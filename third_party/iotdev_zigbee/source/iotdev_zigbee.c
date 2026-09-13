/*
 * iotdev_zigbee.c
 *
 *  Implementation of the Zigbee middleware façade.
 *
 *  Two design patterns are at play:
 *
 *      • Command Pattern  (outbound).  iotdev_zigbee_send_command()
 *        validates the envelope and copies it onto an internal FreeRTOS
 *        queue (s_cmd_queue).  iotdev_zigbee_task() later dequeues and
 *        routes by command class:
 *           – function-level (SET_ONOFF, SET_BRIGHTNESS, READ_STATE, …)
 *             → looked up via {ieee_addr, sensor_id} and forwarded to
 *               the matching device function's `on_command` op;
 *           – network-level  (NETWORK_OPEN/CLOSE, NETWORK_CONFIG,
 *             DEVICE_REMOVE, DEVICE_OTA_START, COPROCESSOR_FW_UPDATE_*,
 *             NETWORK_INFO, …)
 *             → forwarded to the matching zb_core / device-manager
 *               entry point.
 *        Either way, every command runs on the iotdev_zigbee task, so
 *        no driver state ever sees concurrent access.
 *
 *      • Observer Pattern (inbound).  Components register a callback
 *        with iotdev_zigbee_subscribe(); events arrive from two driver
 *        sources:
 *           – sensor / actuator events from device functions, via
 *             zb_device_manager_register_event_notify_callback();
 *           – network / lifecycle events from zb_core, via
 *             zb_core_set_event_callback().
 *        Both are funneled into a single broker that delivers
 *        `s_zb_event_t` to subscribers under a recursive mutex.
 *
 *  Threading model
 *  ---------------
 *      • iotdev_zigbee_send_command() may be called from any FreeRTOS
 *        task; it never executes driver code in the caller's context.
 *      • iotdev_zigbee_task() drains s_cmd_queue and then calls
 *        zb_core_task() on every loop iteration.  All command dispatch
 *        and all driver event handling therefore run on a single OS
 *        task.
 *
 *  Author: Vo Van Buong (BRT-SG)
 */

#include <string.h>

#include "iotdev_common/include/iotdev_common.h"
#include "iotdev_zigbee/include/iotdev_zigbee.h"
#include "iotdev_zigbee/zigbee_driver/core/zb_core.h"
#include "iotdev_zigbee/zigbee_driver/device/zb_device_manager.h"
#include "iotdev_zigbee/zigbee_driver/device/zb_device.h"
#include "iotdev_zigbee/zigbee_driver/znp/zb_znp.h"
#include "iotdev_zigbee/zigbee_driver/znp/zb_znp_mt_app.h"
#include "iotdev_zigbee/zigbee_driver/ota/zb_ota_server.h"
#include "iotdev_zigbee/zigbee_driver/zdo/zb_zdo.h"
#include "iotdev_zigbee/zigbee_driver/zcl/zb_zcl_ss.h"
#include "iotdev_zigbee/zigbee_driver/zcl/zb_zcl_general.h"
#include "iotdev_zigbee/zigbee_driver/zcl/zb_zcl_lighting.h"
#include "iotdev_zigbee/zigbee_driver/device/lighting/zb_device_color_light.h"
#include "iotdev_zigbee/zigbee_driver/device/lighting/zb_device_extended_color_light.h"

#include "esp_err.h"
#include "iotdev_modbus/include/iotdev_modbus.h"
#include "iotdev_config/include/iotdev_config.h"

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"
#include "freertos/semphr.h"
#include "freertos/timers.h"

#define TAG "IOTDEV_ZB"

/* ====================================================================== *
 *  Configuration
 * ====================================================================== */
#ifndef IOTDEV_ZIGBEE_CMD_QUEUE_DEPTH
#define IOTDEV_ZIGBEE_CMD_QUEUE_DEPTH       16
#endif

#ifndef IOTDEV_ZIGBEE_CMD_ENQUEUE_TIMEOUT_MS
#define IOTDEV_ZIGBEE_CMD_ENQUEUE_TIMEOUT_MS 50
#endif

/* ====================================================================== *
 *  Internal types
 * ====================================================================== */
typedef struct subscriber_slot
{
    bool                            in_use;
    e_zb_event_type_t               event_type; /* IOTDEV_ZIGBEE_EVENT_ANY = wildcard */
    iotdev_zigbee_event_callback_t  cb;
    void                           *ctx;
} subscriber_slot_t;

/* ====================================================================== *
 *  Internal state
 * ====================================================================== */
static SemaphoreHandle_t s_broker_mutex = NULL;
static subscriber_slot_t s_subscribers[IOTDEV_ZIGBEE_MAX_SUBSCRIBERS];
static QueueHandle_t     s_cmd_queue    = NULL;   /* function-level commands */
static volatile bool     s_initialised  = false;
static TimerHandle_t     s_permit_join_timer = NULL; /* one-shot: auto-close permit-join on expiry */

/* Forward declarations — used by iotdev_zigbee_task. */
static iotdev_zigbee_status_t
dispatch_function_command(const s_iotdev_zigbee_command_info_t *cmd);

static iotdev_zigbee_status_t
dispatch_network_command(const s_iotdev_zigbee_command_info_t *cmd);

static bool
is_network_level_command(e_zb_cmd_type_t type);

static bool
command_requires_running_network(e_zb_cmd_type_t type);

static void
cmd_release_owned_strings(s_iotdev_zigbee_command_info_t *cmd);

static void
arm_permit_join_timer(uint8_t duration_s);

static void
cancel_permit_join_timer(void);

static void
track_ota_event(const s_zb_event_t *event);

static void
drain_command_queue(void)
{
    if (s_cmd_queue == NULL)
    {
        return;
    }
    s_iotdev_zigbee_command_info_t pending;
    while (xQueueReceive(s_cmd_queue, &pending, 0) == pdTRUE)
    {
        /*
         * Authoritative network-readiness gate.
         * The submit-time check in iotdev_zigbee_send_command() is a
         * fast-path; the network could have gone down between then and
         * now (e.g. user issued a leave / factory-reset).  Re-validate
         * here and drop the command if the gate fails — the caller
         * already received OK, so we just log; correctness depends on
         * the caller observing the relevant event for completion
         * (state report, attribute read response, …).
         */
        if (command_requires_running_network(pending.cmd.type) &&
            !zb_core_get_running_status())
        {
            IOTDEV_LOGW(TAG,
                "Drop queued cmd type=%d: network went down before dispatch",
                (int)pending.cmd.type);
            cmd_release_owned_strings(&pending);
            continue;
        }

        if (is_network_level_command(pending.cmd.type))
        {
            (void)dispatch_network_command(&pending);
        }
        else
        {
            (void)dispatch_function_command(&pending);
        }
        cmd_release_owned_strings(&pending);
    }
}

/* ====================================================================== *
 *  Helpers
 * ====================================================================== */
static inline bool
broker_lock(void)
{
    if (s_broker_mutex == NULL)
    {
        return false;
    }
    return xSemaphoreTakeRecursive(s_broker_mutex, portMAX_DELAY) == pdTRUE;
}

static inline void
broker_unlock(void)
{
    if (s_broker_mutex != NULL)
    {
        xSemaphoreGiveRecursive(s_broker_mutex);
    }
}

/* ====================================================================== *
 *  Bridges from driver layer → broker
 *  Both have the same shape (s_zb_event_t *) so a single forwarder works.
 * ====================================================================== */
static void
on_driver_event(const s_zb_event_t *event)
{
    (void)iotdev_zigbee_publish(event);
}

/* ====================================================================== *
 *  RS485 / Modbus transport binding
 *  ----------------------------------------------------------------------
 *  The Modbus RTU master (iotdev_modbus) runs its RS485 traffic over the
 *  ZNP coprocessor. These hooks are called directly from the Modbus/RS485
 *  task; concurrency with the Zigbee stack's own ZNP traffic is handled at
 *  the ZNP layer (zb_znp.c req_mutex serializes every SREQ/mode request),
 *  so we don't route them through the command queue:
 *      TX     -> zb_znp_mt_app_rs485_write_req()  (ZNP_APP_RS485_WRITE_REQ)
 *      config -> zb_znp_mt_app_rs485_config_req() (ZNP_APP_RS485_CONFIG_REQ)
 *      RX     <- zb_core RS485 rx callback -> iotdev_modbus_rx_feed()
 * ====================================================================== */
static int
rs485_modbus_config(uint32_t baud_rate, uint8_t stop_bits, uint8_t parity, uint16_t idle_gap_ms)
{
    const s_zb_znp_mt_app_config_t cfg = {
        .baud_rate   = baud_rate,
        .stop_bits   = stop_bits,
        .parity      = parity,
        .idle_gap_ms = idle_gap_ms,
    };
    return zb_znp_mt_app_rs485_config_req(&cfg);
}

static void
bind_modbus_transport(void)
{
    const iotdev_modbus_transport_t mb_transport = {
        .write  = zb_znp_mt_app_rs485_write_req,
        .config = rs485_modbus_config,
    };
    iotdev_modbus_set_transport(&mb_transport);
    zb_core_set_rs485_rx_callback(iotdev_modbus_rx_feed);

    /* A ZNP reset reboots the coprocessor that owns the RS485 UART, wiping the
     * line config we pushed. Bracket the whole reset window for the Modbus
     * master: it stops transmitting while the ZNP is down, and re-applies the
     * line config on the first transaction after it comes back up. */
    zb_core_set_znp_link_callbacks(iotdev_modbus_transport_down,
                                   iotdev_modbus_transport_up);
}

/* ====================================================================== *
 *  Public API — lifecycle
 * ====================================================================== */
void
iotdev_zigbee_init(void)
{
    if (s_initialised)
    {
        IOTDEV_LOGW(TAG, "Already initialised");
        return;
    }

    IOTDEV_LOGI(TAG, "Init Zigbee middleware, free heap: %lu",
                (unsigned long)esp_get_free_heap_size());

    memset(s_subscribers, 0, sizeof(s_subscribers));

    s_broker_mutex = xSemaphoreCreateRecursiveMutex();
    if (s_broker_mutex == NULL)
    {
        IOTDEV_LOGE(TAG, "Failed to create broker mutex");
        return;
    }

    s_cmd_queue = xQueueCreate(IOTDEV_ZIGBEE_CMD_QUEUE_DEPTH,
                               sizeof(s_iotdev_zigbee_command_info_t));
    if (s_cmd_queue == NULL)
    {
        IOTDEV_LOGE(TAG, "Failed to create command queue");
        vSemaphoreDelete(s_broker_mutex);
        s_broker_mutex = NULL;
        return;
    }

    zb_core_init();

    /* Override the built-in network defaults with the values persisted in the
     * config file (channel / channel mask / tx power) before the network forms.
     * Missing keys fall back to the config module's own defaults; a fully
     * unreadable set leaves zb_core's built-in defaults untouched. */
    {
        uint32_t channel = 0, channel_mask = 0, tx_power = 0;
        iotdev_config_get_int(CFG_ZIGBEE_CHANNEL, &channel);
        iotdev_config_get_int(CFG_ZIGBEE_CHANNEL_MASK, &channel_mask);
        iotdev_config_get_int(CFG_ZIGBEE_TX_POWER, &tx_power);
        zb_core_apply_default_network_config((uint8_t)channel, channel_mask, (int8_t)tx_power);
    }

    /* Two upstream event sources, one downstream broker. */
    zb_core_set_event_callback(on_driver_event);
    zb_device_manager_register_event_notify_callback(on_driver_event);

    /* Bind the Modbus RTU master onto the ZNP RS485 transport. */
    bind_modbus_transport();

    s_initialised = true;
}

void
iotdev_zigbee_deinit(void)
{
    iotdev_common_set_module_run_status(ZIGBEE_APP_TASK, false);

    if (s_broker_mutex != NULL)
    {
        if (broker_lock())
        {
            memset(s_subscribers, 0, sizeof(s_subscribers));
            broker_unlock();
        }
        vSemaphoreDelete(s_broker_mutex);
        s_broker_mutex = NULL;
    }
    if (s_permit_join_timer != NULL)
    {
        xTimerStop(s_permit_join_timer, portMAX_DELAY);
        xTimerDelete(s_permit_join_timer, portMAX_DELAY);
        s_permit_join_timer = NULL;
    }
    if (s_cmd_queue != NULL)
    {
        vQueueDelete(s_cmd_queue);
        s_cmd_queue = NULL;
    }
    s_initialised = false;
}

/* ====================================================================== *
 *  Public API — tasks
 * ====================================================================== */
void
iotdev_zigbee_task(void *param)
{
    (void)param;
    IOTDEV_LOGI(TAG, "Starting Zigbee Task, free heap: %lu", esp_get_free_heap_size());
    iotdev_common_set_module_run_status(ZIGBEE_APP_TASK, true);
    while (iotdev_common_get_module_run_status(ZIGBEE_APP_TASK))
    {
        /* 1) Drain any function-level commands queued by other tasks.
         *    Running them here serialises every ZCL / ZNP touch onto a
         *    single OS task. */
        drain_command_queue();
        /* 2) Step the driver core (drains zb_event_queue). */
        zb_core_task();
        IOTDEV_TASK_DELAY_MS(10);
    }
    zb_core_deinit();
    IOTDEV_LOGI(TAG, "Task exited!");
    vTaskDelete(NULL);
}

void
iotdev_zigbee_znp_task(void *param)
{
    (void)param;
    IOTDEV_LOGI(TAG, "Starting ZNP Task, free heap: %lu", esp_get_free_heap_size());
    iotdev_common_set_module_run_status(ZIGBEE_ZNP_TASK, true);
    while (iotdev_common_get_module_run_status(ZIGBEE_ZNP_TASK))
    {
        zb_znp_task();
    }
    IOTDEV_LOGI(TAG, "ZNP Task exited!");
    vTaskDelete(NULL);
}

void
iotdev_zigbee_ota_task(void *param)
{
    (void)param;
    IOTDEV_LOGI(TAG, "Starting Zigbee OTA Task, free heap: %lu", esp_get_free_heap_size());
    iotdev_common_set_module_run_status(ZIGBEE_OTA_TASK, true);
    while (iotdev_common_get_module_run_status(ZIGBEE_OTA_TASK))
    {
        zb_ota_task();
    }
    IOTDEV_LOGI(TAG, "Zigbee OTA Task exited!");
    vTaskDelete(NULL);
}

bool iotdev_zigbee_is_app_task_running(void)
{
    return iotdev_common_get_module_run_status(ZIGBEE_APP_TASK);
}

bool iotdev_zigbee_is_znp_task_running(void)
{
    return iotdev_common_get_module_run_status(ZIGBEE_ZNP_TASK);
}

bool iotdev_zigbee_is_ota_task_running(void)
{
    return iotdev_common_get_module_run_status(ZIGBEE_OTA_TASK);
}

bool iotdev_zigbee_get_running_status(void)
{
    return zb_core_get_running_status();
}

int
iotdev_zigbee_get_core_temperature(int16_t *temperature)
{
    return zb_core_get_core_temperature(temperature);
}

int
iotdev_zigbee_get_heap_statistics(uint32_t *free_heap, uint32_t *total_heap)
{
    if (zb_core_get_running_status() == false)
    {
        return IOTDEV_FAIL;
    }
    return zb_core_get_heap_statistics(free_heap, total_heap);
}

/* ====================================================================== *
 *  Observer Pattern — inbound event broker
 * ====================================================================== */
iotdev_zigbee_sub_handle_t
iotdev_zigbee_subscribe(e_zb_event_type_t              event_type,
                        iotdev_zigbee_event_callback_t cb,
                        void                          *ctx)
{
    if (!s_initialised || cb == NULL)
    {
        return IOTDEV_ZIGBEE_INVALID_SUB_HANDLE;
    }
    /* event_type is validated only loosely: anything in the driver enum
     * range, or the wildcard, is acceptable. */
    if (event_type != IOTDEV_ZIGBEE_EVENT_ANY &&
        (unsigned)event_type >= ZB_EVENT_TYPE_MAX)
    {
        return IOTDEV_ZIGBEE_INVALID_SUB_HANDLE;
    }

    iotdev_zigbee_sub_handle_t handle = IOTDEV_ZIGBEE_INVALID_SUB_HANDLE;
    if (!broker_lock())
    {
        return handle;
    }
    for (uint16_t i = 0; i < IOTDEV_ZIGBEE_MAX_SUBSCRIBERS; ++i)
    {
        if (!s_subscribers[i].in_use)
        {
            s_subscribers[i].in_use     = true;
            s_subscribers[i].event_type = event_type;
            s_subscribers[i].cb         = cb;
            s_subscribers[i].ctx        = ctx;
            handle = (iotdev_zigbee_sub_handle_t)i;
            break;
        }
    }
    broker_unlock();

    if (handle == IOTDEV_ZIGBEE_INVALID_SUB_HANDLE)
    {
        IOTDEV_LOGE(TAG, "Subscriber pool full (max=%d)", IOTDEV_ZIGBEE_MAX_SUBSCRIBERS);
    }
    return handle;
}

iotdev_zigbee_status_t
iotdev_zigbee_unsubscribe(iotdev_zigbee_sub_handle_t handle)
{
    if (!s_initialised)
    {
        return IOTDEV_ZIGBEE_ERR_NOT_INIT;
    }
    if (handle < 0 || handle >= IOTDEV_ZIGBEE_MAX_SUBSCRIBERS)
    {
        return IOTDEV_ZIGBEE_ERR_PARAM;
    }

    iotdev_zigbee_status_t st = IOTDEV_ZIGBEE_ERR_NOT_FOUND;
    if (!broker_lock())
    {
        return IOTDEV_ZIGBEE_ERR;
    }
    if (s_subscribers[handle].in_use)
    {
        memset(&s_subscribers[handle], 0, sizeof(s_subscribers[handle]));
        st = IOTDEV_ZIGBEE_OK;
    }
    broker_unlock();
    return st;
}

int8_t
iotdev_zigbee_lqi_to_rssi(uint8_t lqi)
{
    /* TI Z-Stack linear approximation: LQI 0 -> -90 dBm, 255 -> -20 dBm. */
    return (int8_t)((int)lqi * 70 / 255 - 90);
}

iotdev_zigbee_status_t
iotdev_zigbee_publish(const s_zb_event_t *event)
{
    if (!s_initialised)
    {
        return IOTDEV_ZIGBEE_ERR_NOT_INIT;
    }
    if (event == NULL)
    {
        return IOTDEV_ZIGBEE_ERR_PARAM;
    }

    /* Every event passes through here, so this is the one place that sees a
     * whole firmware update without spending a subscriber slot. */
    track_ota_event(event);

    /*
     * Snapshot the matching subscribers under the mutex, then invoke
     * callbacks outside the mutex.  Holding the lock across user code
     * would let a callback that subscribes / unsubscribes / publishes
     * deadlock on itself.
     */
    subscriber_slot_t snapshot[IOTDEV_ZIGBEE_MAX_SUBSCRIBERS];
    uint16_t          snapshot_count = 0;

    if (!broker_lock())
    {
        return IOTDEV_ZIGBEE_ERR;
    }
    for (uint16_t i = 0; i < IOTDEV_ZIGBEE_MAX_SUBSCRIBERS; ++i)
    {
        if (!s_subscribers[i].in_use)
        {
            continue;
        }
        if (s_subscribers[i].event_type != IOTDEV_ZIGBEE_EVENT_ANY &&
            s_subscribers[i].event_type != event->type)
        {
            continue;
        }
        snapshot[snapshot_count++] = s_subscribers[i];
    }
    broker_unlock();

    for (uint16_t i = 0; i < snapshot_count; ++i)
    {
        snapshot[i].cb(event, snapshot[i].ctx);
    }
    return IOTDEV_ZIGBEE_OK;
}

/* ====================================================================== *
 *  Command Pattern — outbound dispatch
 *  ----------------------------------------------------------------------
 *  All commands ride a single FreeRTOS queue (s_cmd_queue).  The
 *  iotdev_zigbee task dequeues each envelope and routes it:
 *      • function-level: targeted at a (device, sensor_id) pair.  The
 *        device is found via zb_device_manager_find_by_ieee(), the
 *        function via zb_device_manager_find_function(), and the
 *        semantic command is handed to func->ops->on_command(), which
 *        owns the cluster / attribute / ZCL frame details.
 *      • network-level:  targeted at the coordinator / Z-Stack itself.
 *        These call directly into zb_core / zb_zdo / zb_ota_server
 *        entry points (no separate event queue).  They are still safe
 *        because they run on the same OS task as zb_core_task().
 * ====================================================================== */

static iotdev_zigbee_status_t
dispatch_function_command(const s_iotdev_zigbee_command_info_t *cmd)
{
    s_zb_device_t *device = zb_device_manager_find_by_ieee(cmd->ieee_addr);
    if (device == NULL)
    {
        IOTDEV_LOGE(TAG, "Device not found: ieee=%016llx",
                    (unsigned long long)cmd->ieee_addr);
        return IOTDEV_ZIGBEE_ERR_NOT_FOUND;
    }

    s_zb_function_t *func = zb_device_manager_find_function(device, cmd->sensor_id);
    if (func == NULL)
    {
        IOTDEV_LOGE(TAG, "Function not found: ieee=%016llx sensor_id=0x%08lx",
                    (unsigned long long)cmd->ieee_addr,
                    (unsigned long)cmd->sensor_id);
        return IOTDEV_ZIGBEE_ERR_NOT_FOUND;
    }
    /* Runtime poll-interval override — local config, no ZCL frame, so it is
     * handled centrally and works regardless of whether the function defines
     * on_command. The poll scheduler (zb_core_query_device_task) applies the
     * new cadence on its next pass. */
    if (cmd->cmd.type == ZB_CMD_SET_POLL_INTERVAL)
    {
        func->poll_interval_ms = cmd->cmd.poll.interval_ms;
        IOTDEV_LOGI(TAG, "Set poll interval for %s -> %lu ms",
                    func->name, (unsigned long)func->poll_interval_ms);
        /* Persist the override so it survives reboot / re-interview (re-applied
         * over the build-time resolved cadence on load, FR-6.8/FR-4.7). */
        s_zb_desired_config_t e = {0};
        e.kind             = ZB_DESIRED_CFG_POLL;
        e.src_endpoint     = ZB_SENSOR_EP(cmd->sensor_id);
        e.cluster_id       = ZB_SENSOR_CLUSTER(cmd->sensor_id);
        e.poll_interval_ms = func->poll_interval_ms;
        zb_device_manager_record_desired_config(device, &e);
        return IOTDEV_ZIGBEE_OK;
    }

    /* Bind / unbind and reporting-config are generic ZDO/ZCL operations handled
     * centrally (no per-function on_command needed). The intent is persisted so
     * it can be re-applied when the device re-joins (FR-5.10). */
    if (cmd->cmd.type == ZB_CMD_DEVICE_BIND || cmd->cmd.type == ZB_CMD_DEVICE_UNBIND)
    {
        bool bind = (cmd->cmd.type == ZB_CMD_DEVICE_BIND);
        uint64_t dst_ieee = cmd->cmd.device_bind.ieee_addr;
        zb_sensor_id_t dst_sensor = cmd->cmd.device_bind.sensor_id;

        s_zb_desired_config_t e = {0};
        e.kind         = ZB_DESIRED_CFG_BIND;
        e.src_endpoint = ZB_SENSOR_EP(cmd->sensor_id);
        e.cluster_id   = ZB_SENSOR_CLUSTER(cmd->sensor_id);
        e.dst_ieee     = dst_ieee;
        e.dst_endpoint = ZB_SENSOR_EP(dst_sensor);

        if (bind)
        {
            zb_status_t st = zb_device_manager_apply_desired_config_entry(device, &e, false);
            zb_device_manager_record_desired_config(device, &e);     /* persist intent */
            return (st == ZB_OK) ? IOTDEV_ZIGBEE_OK : IOTDEV_ZIGBEE_ERR;
        }
        zb_status_t rc = zb_core_unbind_enqueue(device->nwk_addr, device->ieee_addr,
                e.src_endpoint, dst_ieee, e.dst_endpoint, e.cluster_id);
        zb_device_manager_remove_desired_config(device, &e);         /* forget intent */
        return (rc == ZB_OK) ? IOTDEV_ZIGBEE_OK : IOTDEV_ZIGBEE_ERR;
    }

    if (cmd->cmd.type == ZB_CMD_DEVICE_CONFIG_REPORT)
    {
        s_zb_desired_config_t e = {0};
        e.kind         = ZB_DESIRED_CFG_REPORT;
        e.src_endpoint = ZB_SENSOR_EP(cmd->sensor_id);
        e.cluster_id   = ZB_SENSOR_CLUSTER(cmd->sensor_id);
        e.attr_id      = cmd->cmd.device_config_report.attr_id;
        e.data_type    = cmd->cmd.device_config_report.data_type;
        e.min_interval = cmd->cmd.device_config_report.min_interval;
        e.max_interval = cmd->cmd.device_config_report.max_interval;
        e.change_len   = cmd->cmd.device_config_report.change_len;
        memcpy(e.change, cmd->cmd.device_config_report.change, sizeof(e.change));

        zb_status_t st = zb_device_manager_apply_desired_config_entry(device, &e, false);
        if (e.max_interval == 0xFFFF)   /* disabling reporting → forget intent */
            zb_device_manager_remove_desired_config(device, &e);
        else
            zb_device_manager_record_desired_config(device, &e);
        return (st == ZB_OK) ? IOTDEV_ZIGBEE_OK : IOTDEV_ZIGBEE_ERR;
    }

    /* A plain attribute write is generic ZCL, like the two above - no
     * per-function on_command needed. Deliberately NOT persisted as desired
     * config: the value lives in the device's own NVM and survives a rejoin,
     * unlike bindings and reporting config which a factory reset wipes. */
    if (cmd->cmd.type == ZB_CMD_DEVICE_WRITE_ATTR)
    {
        const uint8_t value_len = cmd->cmd.device_write_attr.value_len;
        if (value_len == 0 || value_len > sizeof(cmd->cmd.device_write_attr.value))
        {
            IOTDEV_LOGE(TAG, "WRITE_ATTR: bad value_len %u", value_len);
            return IOTDEV_ZIGBEE_ERR_PARAM;
        }

        /* 0 = the cluster the addressed function lives on; an explicit cluster
         * lets a setting on a manufacturer cluster be written while still
         * addressing the function it configures. */
        uint16_t cluster_id = cmd->cmd.device_write_attr.cluster_id;
        if (cluster_id == 0)
        {
            cluster_id = ZB_SENSOR_CLUSTER(cmd->sensor_id);
        }

        uint8_t buf[sizeof(s_zb_zcl_write_attr_cmd_t) + sizeof(s_zb_zcl_write_attr_info_t)] = {0};
        s_zb_zcl_write_attr_cmd_t *wr = (s_zb_zcl_write_attr_cmd_t *)buf;
        wr->num_attr = 1;
        wr->attr_list[0].attr_id   = cmd->cmd.device_write_attr.attr_id;
        wr->attr_list[0].data_type = cmd->cmd.device_write_attr.data_type;
        wr->attr_list[0].attr_data = (uint8_t *)cmd->cmd.device_write_attr.value;

        s_zb_af_address_t addr = {0};
        addr.address_mode = AF_ADDRESS_16BIT;
        addr.short_addr   = device->nwk_addr;
        addr.endpoint     = ZB_SENSOR_EP(cmd->sensor_id);

        zb_status_t st = zb_zcl_send_write_manu(ZB_HUB_ENDPOINT, &addr, cluster_id, wr,
                                               ZCL_CMD_WRITE, ZCL_FRAME_CLIENT_SERVER_DIR,
                                               false, cmd->cmd.device_write_attr.manuf_code,
                                               zb_zcl_next_seq_num());
        IOTDEV_LOGI(TAG, "WRITE_ATTR ieee=%016llx ep=%u cl=0x%04x attr=0x%04x mfr=0x%04x (%d)",
                    (unsigned long long)cmd->ieee_addr, ZB_SENSOR_EP(cmd->sensor_id),
                    cluster_id, cmd->cmd.device_write_attr.attr_id,
                    cmd->cmd.device_write_attr.manuf_code, st);
        return (st == ZB_OK) ? IOTDEV_ZIGBEE_OK : IOTDEV_ZIGBEE_ERR;
    }

    if (func->ops == NULL || func->ops->on_command == NULL)
    {
        IOTDEV_LOGE(TAG, "Function %s does not handle commands", func->name);
        return IOTDEV_ZIGBEE_ERR_NOT_SUPPORTED;
    }

    IOTDEV_LOGI(TAG, "Dispatch [%s] -> %s on ieee=%016llx ep=%u cluster=0x%04x",
                func->name,
                "function command",
                (unsigned long long)cmd->ieee_addr,
                ZB_SENSOR_EP(cmd->sensor_id),
                ZB_SENSOR_CLUSTER(cmd->sensor_id));

    zb_status_t st = func->ops->on_command(device, func, &cmd->cmd);
    return (st == ZB_OK) ? IOTDEV_ZIGBEE_OK : IOTDEV_ZIGBEE_ERR;
}

/**
 * Pick the endpoint an Identify command should be addressed to.
 *
 * Identify targets the physical device, not one of its logical functions, so
 * the caller does not choose the endpoint. We prefer one that actually
 * advertises the Identify cluster, since multi-endpoint devices commonly
 * implement it on only one of them, and fall back to the first endpoint so a
 * device with an incomplete interview still gets a chance to respond.
 *
 * @return the endpoint id, or 0 if the device has no endpoints at all.
 */
static uint8_t
resolve_identify_endpoint(const s_zb_device_t *device)
{
    for (uint8_t i = 0; i < device->endpoint_count; ++i)
    {
        const s_zb_device_endpoint_t *ep = &device->endpoints[i];
        for (uint8_t c = 0; c < ep->in_cluster_count; ++c)
        {
            if (ep->in_clusters[c] == ZCL_CLUSTER_ID_GENERAL_IDENTIFY)
            {
                return ep->endpoint_id;
            }
        }
    }

    return (device->endpoint_count > 0) ? device->endpoints[0].endpoint_id : 0;
}

static iotdev_zigbee_status_t
dispatch_network_command(const s_iotdev_zigbee_command_info_t *cmd)
{
    IOTDEV_LOGI(TAG, "Dispatch network command type=%d ieee=%016llx",
                (int)cmd->cmd.type, (unsigned long long)cmd->ieee_addr);

    switch (cmd->cmd.type)
    {
        case ZB_CMD_NETWORK_OPEN:
        {
            uint8_t duration = cmd->cmd.network_open.duration;
            if (duration == 0xFF)
            {
                duration = 0xFE;
            }
            int rc = zb_zdo_permit_join(duration);
            if (rc == ZB_OK)
            {
                /*
                 * Arm a host-side guard timer so the network is explicitly
                 * closed (permit-join 0) once the requested window elapses.
                 * duration 0 already closes the network, so neither arms an auto-close timer.
                 */
                if (duration != 0)
                {
                    arm_permit_join_timer(duration);
                    s_zb_event_t event = {0};
                    event.type = ZB_EVENT_NETWORK_OPEN;
                    zb_core_publish_event(&event);
                }
                else
                {
                    cancel_permit_join_timer();
                    s_zb_event_t event = {0};
                    event.type = ZB_EVENT_NETWORK_CLOSE;
                    zb_core_publish_event(&event);
                }
            }
            return (rc == ZB_OK) ? IOTDEV_ZIGBEE_OK : IOTDEV_ZIGBEE_ERR;
        }

        case ZB_CMD_NETWORK_CLOSE:
        {
            /* Explicit close - drop any pending auto-close guard timer. */
            int rc = zb_zdo_permit_join(0);
            if (rc == ZB_OK)
            {
                cancel_permit_join_timer();
                s_zb_event_t event = {0};
                event.type = ZB_EVENT_NETWORK_CLOSE;
                zb_core_publish_event(&event);
            }
            return (rc == ZB_OK) ? IOTDEV_ZIGBEE_OK : IOTDEV_ZIGBEE_ERR;
        }

        case ZB_CMD_NETWORK_START:
        {
            zb_status_t rc = zb_core_request_start();
            return (rc == ZB_OK) ? IOTDEV_ZIGBEE_OK : IOTDEV_ZIGBEE_ERR;
        }

        case ZB_CMD_NETWORK_STOP:
        {
            zb_status_t rc = zb_core_request_stop();
            return (rc == ZB_OK) ? IOTDEV_ZIGBEE_OK : IOTDEV_ZIGBEE_ERR;
        }

        case ZB_CMD_NETWORK_CONFIG:
        {
            zb_status_t rc =
                zb_core_apply_network_config(&cmd->cmd.network_config, cmd->cmd.type);
            return (rc == ZB_OK) ? IOTDEV_ZIGBEE_OK : IOTDEV_ZIGBEE_ERR;
        }

        case ZB_CMD_NETWORK_FACTORY_RESET:
        {
            zb_status_t rc = zb_core_request_factory_reset();
            return (rc == ZB_OK) ? IOTDEV_ZIGBEE_OK : IOTDEV_ZIGBEE_ERR;
        }

        case ZB_CMD_DEVICE_OTA_ABORT:
        {
            zb_ota_server_abort();
            return IOTDEV_ZIGBEE_OK;
        }

        case ZB_CMD_DEVICE_OTA_PROGRESS:
        {
            uint8_t percent = 0;
            if (!zb_ota_server_get_progress(&percent))
            {
                return IOTDEV_ZIGBEE_ERR_NOT_READY;
            }
            s_zb_event_t event = {0};
            event.type = ZB_EVENT_DEVICE_OTA_PROGRESS;
            event.ieee_addr = zb_ota_server_get_client_ieee();
            event.ota_progress.percent = percent;
            event.ota_progress.phase = ZB_OTA_PROGRESS_PHASE_DOWNLOAD;
            zb_core_publish_event(&event);
            return IOTDEV_ZIGBEE_OK;
        }

        case ZB_CMD_NETWORK_INFO:
        {
            zb_core_publish_network_info();
            return IOTDEV_ZIGBEE_OK;
        }

        case ZB_CMD_DEVICE_LIST:
        {
            /* Enumerate the device pool on this (driver) task and stream the
             * snapshot back as a DEVICE_LIST_BEGIN/ITEM/END burst. */
            zb_device_manager_publish_device_list(cmd->cmd.device_list.txn_id);
            return IOTDEV_ZIGBEE_OK;
        }

        case ZB_CMD_DEVICE_STATE:
        {
            /* Re-emit each function's cached value for the target device (or all
             * devices when ieee_addr==0), bracketed by DEVICE_STATE_BEGIN/END. */
            zb_device_manager_publish_device_state(cmd->ieee_addr,
                                                   cmd->cmd.device_state.txn_id);
            return IOTDEV_ZIGBEE_OK;
        }

        case ZB_CMD_DEVICE_STATE_ARRAY:
        {
            /* Same cached values, but one array event per device
             * (ZB_EVENT_DEVICE_STATE) instead of a per-function burst. */
            zb_device_manager_publish_device_state_array(cmd->ieee_addr,
                                                         cmd->cmd.device_state.txn_id);
            return IOTDEV_ZIGBEE_OK;
        }

        case ZB_CMD_DEVICE_REMOVE:
        {
            return (zb_device_manager_remove_device(cmd->ieee_addr) == ZB_OK)
                    ? IOTDEV_ZIGBEE_OK : IOTDEV_ZIGBEE_ERR;
        }

        case ZB_CMD_DEVICE_IDENTIFY:
        {
            s_zb_device_t *device = zb_device_manager_find_by_ieee(cmd->ieee_addr);
            if (device == NULL)
            {
                IOTDEV_LOGE(TAG, "IDENTIFY: device not found: ieee=%016llx",
                            (unsigned long long)cmd->ieee_addr);
                return IOTDEV_ZIGBEE_ERR_NOT_FOUND;
            }

            uint8_t endpoint = resolve_identify_endpoint(device);
            if (endpoint == 0)
            {
                IOTDEV_LOGE(TAG, "IDENTIFY: no endpoint on ieee=%016llx",
                            (unsigned long long)cmd->ieee_addr);
                return IOTDEV_ZIGBEE_ERR_NOT_SUPPORTED;
            }

            s_zb_af_address_t addr = {0};
            addr.address_mode = AF_ADDRESS_16BIT;
            addr.short_addr   = device->nwk_addr;
            addr.endpoint     = endpoint;

            zb_status_t st = zb_zcl_general_send_identify(
                                 ZB_HUB_ENDPOINT, &addr,
                                 cmd->cmd.device_identify.identify_time,
                                 0, zb_zcl_next_seq_num());
            IOTDEV_LOGI(TAG, "IDENTIFY ieee=%016llx ep=%u time=%us (%d)",
                        (unsigned long long)cmd->ieee_addr,
                        (unsigned)endpoint,
                        (unsigned)cmd->cmd.device_identify.identify_time,
                        (int)st);
            return (st == ZB_OK) ? IOTDEV_ZIGBEE_OK : IOTDEV_ZIGBEE_ERR;
        }

        case ZB_CMD_DEVICE_OTA_START:
        {
            const char *path = cmd->cmd.device_ota_start.path;
            if (path == NULL)
            {
                return IOTDEV_ZIGBEE_ERR_PARAM;
            }
            zb_ota_server_add_file_entry((char *)path);
            return IOTDEV_ZIGBEE_OK;
        }

        case ZB_CMD_COPROC_BOOTLOADER_CHECK:
        {
            cancel_permit_join_timer();
            zb_status_t rc = zb_core_request_znp_bootloader_enter();
            return (rc == ZB_OK) ? IOTDEV_ZIGBEE_OK : IOTDEV_ZIGBEE_ERR;
        }

        case ZB_CMD_COPROC_FW_UPDATE_START:
        {
            const char *path = cmd->cmd.coprocessor_fw_update_start.path;
            if (path == NULL)
            {
                return IOTDEV_ZIGBEE_ERR_PARAM;
            }
            zb_status_t rc = zb_core_request_znp_fw_update(path);
            return (rc == ZB_OK) ? IOTDEV_ZIGBEE_OK : IOTDEV_ZIGBEE_ERR_BUSY;
        }

        default:
            return IOTDEV_ZIGBEE_ERR_NOT_SUPPORTED;
    }
}

/**
 * Returns true if the command is handled by zb_core / network layer
 * rather than by an individual device function.
 */
static bool
is_network_level_command(e_zb_cmd_type_t type)
{
    switch (type)
    {
        case ZB_CMD_NETWORK_INFO:
        case ZB_CMD_NETWORK_CONFIG:
        case ZB_CMD_NETWORK_OPEN:
        case ZB_CMD_NETWORK_CLOSE:
        case ZB_CMD_NETWORK_START:
        case ZB_CMD_NETWORK_STOP:
        case ZB_CMD_NETWORK_FACTORY_RESET:
        case ZB_CMD_NETWORK_QUERY_LQI:
        case ZB_CMD_DEVICE_LIST:
        case ZB_CMD_DEVICE_STATE:
        case ZB_CMD_DEVICE_STATE_ARRAY:
        case ZB_CMD_DEVICE_REMOVE:
        case ZB_CMD_DEVICE_OTA_START:
        case ZB_CMD_DEVICE_OTA_ABORT:
        case ZB_CMD_DEVICE_OTA_PROGRESS:
        case ZB_CMD_COPROC_FW_UPDATE_START:
        case ZB_CMD_COPROC_BOOTLOADER_CHECK:
        case ZB_CMD_DEVICE_IDENTIFY:
            return true;
        default:
            return false;
    }
}

/**
 * Returns true if the command requires the Zigbee network to be running
 * to be meaningful.
 *
 * The rule is split by command class:
 *   • Function-level commands ALWAYS require a running network — every
 *     one of them is translated by `func->ops->on_command()` into an
 *     over-the-air ZCL frame.  No exceptions.
 *   • Network-level commands mostly require the network too (open /
 *     close permit-join, bind / unbind, device-remove, OTA, …) but a
 *     small allow-list is exempt:
 *        - NETWORK_INFO     : metadata read; used precisely to discover
 *                             whether the network is up.
 *        - NETWORK_CONFIG   : channel / PAN-ID / extPanID applied
 *                             *before* forming.
 *        - COPROCESSOR_FW_* : ZNP firmware update runs in serial
 *                             bootloader, with the Zigbee stack down.
 */
static bool
command_requires_running_network(e_zb_cmd_type_t type)
{
    /* Local config — sets func->poll_interval_ms only, emits no ZCL frame, so
     * it is valid even while the network is down (despite being function-targeted). */
    if (type == ZB_CMD_SET_POLL_INTERVAL)
    {
        return false;
    }

    /* Rule 1 — function-level: always require network.
     *          (Any cluster cmd ends up as a ZCL frame on-air.) */
    if (!is_network_level_command(type))
    {
        return true;
    }

    /* Rule 2 — network-level: check the small allow-list. */
    switch (type)
    {
        case ZB_CMD_NETWORK_START:
        case ZB_CMD_NETWORK_STOP:
        case ZB_CMD_NETWORK_INFO:
        case ZB_CMD_NETWORK_CONFIG:
        case ZB_CMD_NETWORK_FACTORY_RESET:
        case ZB_CMD_DEVICE_LIST:  /* read the known devices even when the network is down */
        case ZB_CMD_DEVICE_STATE: /* cached values — no OTA read, valid while down */
        case ZB_CMD_DEVICE_STATE_ARRAY:
        case ZB_CMD_COPROC_FW_UPDATE_START:
        case ZB_CMD_COPROC_BOOTLOADER_CHECK:
            return false;
        default:
            return true;
    }
}

/* ====================================================================== *
 *  Owned-string lifecycle for queued commands
 *  ----------------------------------------------------------------------
 *  Most s_zb_cmd_t members are POD — a shallow copy through the queue
 *  is enough.  Two members carry a `const char *path` whose buffer is
 *  owned by the caller and may be freed before iotdev_zigbee_task gets
 *  to run the command.  Deep-copy those strings on enqueue and free
 *  them after dispatch.
 * ====================================================================== */
static iotdev_zigbee_status_t
cmd_take_ownership_of_strings(s_iotdev_zigbee_command_info_t *cmd)
{
    const char *src = NULL;
    switch (cmd->cmd.type)
    {
        case ZB_CMD_DEVICE_OTA_START:
            src = cmd->cmd.device_ota_start.path;
            break;
        case ZB_CMD_COPROC_FW_UPDATE_START:
            src = cmd->cmd.coprocessor_fw_update_start.path;
            break;
        default:
            return IOTDEV_ZIGBEE_OK;
    }
    if (src == NULL)
    {
        return IOTDEV_ZIGBEE_OK;
    }

    size_t len = strlen(src) + 1;
    char  *dup = IOTDEV_MEM_MALLOC(len);
    if (dup == NULL)
    {
        return IOTDEV_ZIGBEE_ERR_NO_MEM;
    }
    memcpy(dup, src, len);

    switch (cmd->cmd.type)
    {
        case ZB_CMD_DEVICE_OTA_START:
            cmd->cmd.device_ota_start.path = dup;
            break;
        case ZB_CMD_COPROC_FW_UPDATE_START:
            cmd->cmd.coprocessor_fw_update_start.path = dup;
            break;
        default:
            break; /* unreachable */
    }
    return IOTDEV_ZIGBEE_OK;
}

static void
cmd_release_owned_strings(s_iotdev_zigbee_command_info_t *cmd)
{
    char *p = NULL;
    switch (cmd->cmd.type)
    {
        case ZB_CMD_DEVICE_OTA_START:
            p = (char *)cmd->cmd.device_ota_start.path;
            cmd->cmd.device_ota_start.path = NULL;
            break;
        case ZB_CMD_COPROC_FW_UPDATE_START:
            p = (char *)cmd->cmd.coprocessor_fw_update_start.path;
            cmd->cmd.coprocessor_fw_update_start.path = NULL;
            break;
        default:
            return;
    }
    if (p != NULL)
    {
        IOTDEV_MEM_FREE(p);
    }
}

/**
 * Queue a command (function-level or network-level) for the
 * iotdev_zigbee task to pick up.  Returns IOTDEV_ZIGBEE_ERR_BUSY if the
 * queue is full after the configured timeout.
 */
static iotdev_zigbee_status_t
enqueue_command(const s_iotdev_zigbee_command_info_t *cmd)
{
    if (s_cmd_queue == NULL)
    {
        return IOTDEV_ZIGBEE_ERR_NOT_INIT;
    }

    s_iotdev_zigbee_command_info_t copy = *cmd;
    iotdev_zigbee_status_t st = cmd_take_ownership_of_strings(&copy);
    if (st != IOTDEV_ZIGBEE_OK)
    {
        return st;
    }

    bool ok = xQueueSend(s_cmd_queue, &copy,
                         pdMS_TO_TICKS(IOTDEV_ZIGBEE_CMD_ENQUEUE_TIMEOUT_MS)) == pdTRUE;
    if (!ok)
    {
        IOTDEV_LOGE(TAG, "Command queue full (depth=%d), dropping cmd type=%d",
                    IOTDEV_ZIGBEE_CMD_QUEUE_DEPTH, (int)copy.cmd.type);
        cmd_release_owned_strings(&copy);
        return IOTDEV_ZIGBEE_ERR_BUSY;
    }
    return IOTDEV_ZIGBEE_OK;
}

/* ====================================================================== *
 *  Permit-join guard timer
 *  ----------------------------------------------------------------------
 *  ZB_CMD_NETWORK_OPEN opens the network for a bounded duration.  This
 *  one-shot FreeRTOS timer fires when that window elapses and closes the
 *  network again.  The callback runs on the timer-service (daemon) task,
 *  where it is NOT safe to touch the ZNP serial transport (all driver /
 *  ZCL / ZNP work is single-threaded on iotdev_zigbee_task).  So it only
 *  enqueues a ZB_CMD_NETWORK_CLOSE; the task dispatches it and calls
 *  zb_zdo_permit_join(0).
 * ====================================================================== */
static void
permit_join_timeout_cb(TimerHandle_t timer)
{
    (void)timer;
    IOTDEV_LOGI(TAG, "Permit-join window elapsed; closing network");
    s_iotdev_zigbee_command_info_t close_cmd = {0};
    close_cmd.cmd.type = ZB_CMD_NETWORK_CLOSE;
    (void)enqueue_command(&close_cmd);
}

static void
arm_permit_join_timer(uint8_t duration_s)
{
    const TickType_t period = pdMS_TO_TICKS((uint32_t)duration_s * 1000u);

    if (s_permit_join_timer == NULL)
    {
        s_permit_join_timer = xTimerCreate("zb_permit_join", period,
                                           pdFALSE /* one-shot */, NULL,
                                           permit_join_timeout_cb);
        if (s_permit_join_timer == NULL)
        {
            IOTDEV_LOGE(TAG, "Permit-join guard timer create failed");
            return;
        }
        if (xTimerStart(s_permit_join_timer, portMAX_DELAY) != pdPASS)
        {
            IOTDEV_LOGE(TAG, "Permit-join guard timer start failed");
            return;
        }
    }
    else
    {
        /* Re-open while already open: restart with the new window.
         * xTimerChangePeriod also (re)starts the timer. */
        xTimerStop(s_permit_join_timer, portMAX_DELAY);
        if (xTimerChangePeriod(s_permit_join_timer, period, portMAX_DELAY) != pdPASS)
        {
            IOTDEV_LOGE(TAG, "Permit-join guard timer rearm failed");
            return;
        }
    }
    IOTDEV_LOGI(TAG, "Network open for %u s; auto-close armed", (unsigned)duration_s);
}

static void
cancel_permit_join_timer(void)
{
    if (s_permit_join_timer != NULL)
    {
        xTimerStop(s_permit_join_timer, portMAX_DELAY);
    }
}

/* Latest firmware-update progress, updated on the publish path below. Single
 * writer (the driver task publishes), readers only copy the struct out, and
 * the fields are small - so no lock is warranted here. */
static s_iotdev_zigbee_ota_status_t s_coproc_ota_status = { .state = IOTDEV_ZIGBEE_OTA_IDLE };
static s_iotdev_zigbee_ota_status_t s_device_ota_status = { .state = IOTDEV_ZIGBEE_OTA_IDLE };

/* Fold one coordinator or device OTA event into the matching snapshot. */
static void
track_ota_event(const s_zb_event_t *event)
{
    switch (event->type)
    {
        /* ---- Coprocessor (the hub's own radio) ---- */
        case ZB_EVENT_COORDINATOR_FW_UPDATE_STARTED:
            s_coproc_ota_status.state = IOTDEV_ZIGBEE_OTA_RUNNING;
            s_coproc_ota_status.percent = 0;
            s_coproc_ota_status.phase = 0;
            break;

        case ZB_EVENT_COORDINATOR_FW_UPDATE_PROGRESS:
            s_coproc_ota_status.state = IOTDEV_ZIGBEE_OTA_RUNNING;
            s_coproc_ota_status.percent = event->ota_progress.percent;
            s_coproc_ota_status.phase = event->ota_progress.phase;
            break;

        case ZB_EVENT_COORDINATOR_FW_UPDATE_COMPLETED:
            s_coproc_ota_status.state = IOTDEV_ZIGBEE_OTA_DONE;
            s_coproc_ota_status.percent = 100;
            break;

        case ZB_EVENT_COORDINATOR_FW_UPDATE_FAILED:
        case ZB_EVENT_COORDINATOR_FW_UPDATE_ABORTED:
            s_coproc_ota_status.state = IOTDEV_ZIGBEE_OTA_FAILED;
            break;

        /* ---- One joined device, flashed over the air ---- */
        case ZB_EVENT_DEVICE_OTA_STARTED:
            s_device_ota_status.state = IOTDEV_ZIGBEE_OTA_RUNNING;
            s_device_ota_status.percent = 0;
            s_device_ota_status.phase = 0;
            s_device_ota_status.ieee_addr = event->ieee_addr;
            break;

        case ZB_EVENT_DEVICE_OTA_PROGRESS:
            s_device_ota_status.state = IOTDEV_ZIGBEE_OTA_RUNNING;
            s_device_ota_status.percent = event->ota_progress.percent;
            s_device_ota_status.phase = event->ota_progress.phase;
            s_device_ota_status.ieee_addr = event->ieee_addr;
            break;

        case ZB_EVENT_DEVICE_OTA_COMPLETED:
            s_device_ota_status.state = IOTDEV_ZIGBEE_OTA_DONE;
            s_device_ota_status.percent = 100;
            s_device_ota_status.ieee_addr = event->ieee_addr;
            break;

        case ZB_EVENT_DEVICE_OTA_FAILED:
        case ZB_EVENT_DEVICE_OTA_ABORTED:
            s_device_ota_status.state = IOTDEV_ZIGBEE_OTA_FAILED;
            s_device_ota_status.ieee_addr = event->ieee_addr;
            break;

        default:
            break;
    }
}

void
iotdev_zigbee_get_coprocessor_ota_status(s_iotdev_zigbee_ota_status_t *out)
{
    if (out != NULL)
    {
        *out = s_coproc_ota_status;
    }
}

void
iotdev_zigbee_get_device_ota_status(s_iotdev_zigbee_ota_status_t *out)
{
    if (out != NULL)
    {
        *out = s_device_ota_status;
    }
}

bool
iotdev_zigbee_get_coprocessor_version(char *out, size_t out_len)
{
    s_zb_coordinator_info_t info = {0};

    if ((out == NULL) || (out_len == 0))
    {
        return false;
    }
    out[0] = '\0';

    /* A zero version means the ZNP has never answered a SYS_RESET_IND, so
     * there is nothing to report - do not print it as "0.0.0". The call can
     * also fail outright before the coordinator info is cached. */
    if ((zb_core_get_coordinator_info(&info) != ZB_OK) || (info.version == 0))
    {
        return false;
    }

    /* Packed fw_major<<16 | fw_minor<<8 | fw_maint by zb_core's
     * SYS_RESET_IND handler. */
    snprintf(out, out_len, "%u.%u.%u",
             (unsigned)((info.version >> 16) & 0xFF),
             (unsigned)((info.version >> 8) & 0xFF),
             (unsigned)(info.version & 0xFF));
    return true;
}

iotdev_zigbee_status_t
iotdev_zigbee_send_command(const s_iotdev_zigbee_command_info_t *cmd)
{
    if (!s_initialised)
    {
        return IOTDEV_ZIGBEE_ERR_NOT_INIT;
    }
    if (cmd == NULL)
    {
        return IOTDEV_ZIGBEE_ERR_PARAM;
    }

    /*
     * Network-readiness gate (sender-side, fast reject).
     *
     * Commands that need an over-the-air ZCL frame (all function-level,
     * plus most network-level) are pointless when the Zigbee network
     * is down.  Bail out early to give the caller an accurate answer
     * and to keep the command queue free of doomed work.
     *
     * The dispatch loop re-checks before actually executing, because
     * the network can flip between submit and execution.
     */
    if (command_requires_running_network(cmd->cmd.type) &&
        !zb_core_get_running_status())
    {
        IOTDEV_LOGW(TAG, "Command rejected: network not running, type=%d",
                    (int)cmd->cmd.type);
        return IOTDEV_ZIGBEE_ERR_NOT_READY;
    }

    /*
     * All commands — function-level AND network-level — ride the same
     * internal queue and run on iotdev_zigbee_task, which is the same
     * OS task that runs zb_core_task().  This keeps every driver /
     * ZCL / ZNP touch single-threaded with no extra locks.
     */
    return enqueue_command(cmd);
}

iotdev_zigbee_status_t
iotdev_zigbee_request_device_list(uint32_t txn_id)
{
    s_iotdev_zigbee_command_info_t cmd;
    memset(&cmd, 0, sizeof(cmd));
    cmd.cmd.type = ZB_CMD_DEVICE_LIST;
    cmd.cmd.device_list.txn_id = txn_id;
    return iotdev_zigbee_send_command(&cmd);
}

iotdev_zigbee_status_t
iotdev_zigbee_get_device_info(uint64_t ieee_addr, s_iotdev_zigbee_device_info_t *out)
{
    if (!s_initialised)
    {
        return IOTDEV_ZIGBEE_ERR_NOT_INIT;
    }
    if (out == NULL)
    {
        return IOTDEV_ZIGBEE_ERR_PARAM;
    }

    iotdev_zigbee_status_t rc = IOTDEV_ZIGBEE_ERR_NOT_FOUND;

    /* Hold the driver lock across the whole copy: device add/remove take the
     * same lock, so the record cannot be freed while we read it. Copy only the
     * fields the facade exposes - never the internal function ctx/ops pointers. */
    const s_zb_device_t *d = zb_device_manager_acquire_device(ieee_addr);
    if (d != NULL)
    {
        memset(out, 0, sizeof(*out));

        out->ieee_addr       = d->ieee_addr;
        out->nwk_addr        = d->nwk_addr;
        out->parent_ieee     = d->parent_ieee;
        out->parent_nwk_addr = d->parent_nwk_addr;

        out->online          = d->status == ZB_DEVICE_STATUS_ONLINE;
        out->lqi             = d->lqi;
        out->last_seen       = d->last_seen;

        strncpy(out->manufacturer,  d->manufacturer,  sizeof(out->manufacturer)  - 1);
        strncpy(out->model,         d->model,         sizeof(out->model)         - 1);
        strncpy(out->product_label, d->product_label, sizeof(out->product_label) - 1);
        strncpy(out->serial_number, d->serial_number, sizeof(out->serial_number) - 1);
        strncpy(out->sw_build_id,   d->sw_build_id,   sizeof(out->sw_build_id)   - 1);
        strncpy(out->date_code,     d->date_code,     sizeof(out->date_code)     - 1);

        out->product_code_len = d->product_code_len <= sizeof(out->product_code)
                                    ? d->product_code_len : sizeof(out->product_code);
        memcpy(out->product_code, d->product_code, out->product_code_len);

        out->manu_id              = d->manu_id;
        out->device_type          = d->device_type;
        out->power_source         = d->power_source;
        out->app_version          = d->app_version;
        out->hw_version           = d->hw_version;
        out->physical_environment = d->physical_environment;
        out->battery_powered      = d->battery_powered;
        out->sleepy_enabled       = d->sleepy_enabled;
        out->ota_supported        = d->ota_supported;

        /* Publish only real sensor/actuator functions - ZB_FUNC_BASIC_INFO is
         * device metadata already surfaced in the fields above (matches the
         * filtering used for the DEVICE_LIST_ITEM event). */
        uint8_t n = 0;
        for (int i = 0; i < d->function_count && n < ZB_MAX_FUNCTIONS; i++)
        {
            if (d->functions[i].type == ZB_FUNC_BASIC_INFO)
            {
                continue;
            }
            out->functions[n].sensor_id = d->functions[i].sensor_id;
            out->functions[n].type      = d->functions[i].type;
            strncpy(out->functions[n].name, d->functions[i].name, sizeof(out->functions[n].name) - 1);
            n++;
        }
        out->function_count = n;

        rc = IOTDEV_ZIGBEE_OK;
    }
    zb_device_manager_release_device();
    return rc;
}

iotdev_zigbee_status_t
iotdev_zigbee_request_device_state(uint64_t ieee_addr, uint32_t txn_id)
{
    s_iotdev_zigbee_command_info_t cmd;
    memset(&cmd, 0, sizeof(cmd));
    cmd.ieee_addr = ieee_addr;   /* 0 → snapshot every device */
    cmd.cmd.type = ZB_CMD_DEVICE_STATE;
    cmd.cmd.device_state.txn_id = txn_id;
    return iotdev_zigbee_send_command(&cmd);
}

iotdev_zigbee_status_t
iotdev_zigbee_request_device_state_array(uint64_t ieee_addr, uint32_t txn_id)
{
    s_iotdev_zigbee_command_info_t cmd;
    memset(&cmd, 0, sizeof(cmd));
    cmd.ieee_addr = ieee_addr;   /* 0 → snapshot every device */
    cmd.cmd.type = ZB_CMD_DEVICE_STATE_ARRAY;
    cmd.cmd.device_state.txn_id = txn_id;
    return iotdev_zigbee_send_command(&cmd);
}

iotdev_zigbee_status_t
iotdev_zigbee_identify_device(uint64_t ieee_addr, uint16_t identify_time)
{
    if (ieee_addr == 0)
    {
        return IOTDEV_ZIGBEE_ERR_PARAM;
    }

    s_iotdev_zigbee_command_info_t cmd;
    memset(&cmd, 0, sizeof(cmd));
    cmd.ieee_addr = ieee_addr;
    cmd.cmd.type  = ZB_CMD_DEVICE_IDENTIFY;
    cmd.cmd.device_identify.identify_time = identify_time;
    return iotdev_zigbee_send_command(&cmd);
}

/* Function types that can be given a colour. Matches the set the colour
 * drivers register for; the capability picker in the ZCL layer then decides
 * which on-air command each device actually gets. */
static bool
zigbee_func_is_color(e_zb_function_type_t type)
{
    return (type == ZB_FUNC_COLOR_LIGHT) || (type == ZB_FUNC_EXTENDED_COLOR_LIGHT);
}

/* Locate a device's first colour-capable function. */
static bool
zigbee_find_color_func(uint64_t ieee_addr, zb_sensor_id_t *sensor_id_out,
                       e_zb_function_type_t *type_out)
{
    /* Large struct: keep it off the caller's stack. The facade is not
     * re-entrant per call site, and this is only read here before use. */
    const s_zb_device_t *d = zb_device_manager_acquire_device(ieee_addr);

    if (d != NULL)
    {
        for (uint8_t i = 0; (i < d->function_count) && (i < ZB_MAX_FUNCTIONS); i++)
        {
            if (zigbee_func_is_color(d->functions[i].type))
            {
                if (sensor_id_out != NULL)
                {
                    *sensor_id_out = d->functions[i].sensor_id;
                }
                if (type_out != NULL)
                {
                    *type_out = d->functions[i].type;
                }
                return true;
            }
        }
    }
    zb_device_manager_release_device();
    return false;
}

iotdev_zigbee_status_t
iotdev_zigbee_set_device_color_rgb(uint64_t ieee_addr,
                                   uint8_t r, uint8_t g, uint8_t b,
                                   uint16_t trans_ms)
{
    if (ieee_addr == 0)
    {
        return IOTDEV_ZIGBEE_ERR_PARAM;
    }

    s_iotdev_zigbee_command_info_t cmd;
    memset(&cmd, 0, sizeof(cmd));

    if (!zigbee_find_color_func(ieee_addr, &cmd.sensor_id, NULL))
    {
        return IOTDEV_ZIGBEE_ERR_NOT_FOUND;
    }

    /* Zigbee carries no RGB colour mode - convert once here so every caller
     * (and the serial console) goes through the same maths. */
    uint8_t hue = 0, saturation = 0;
    zb_zcl_lighting_rgb_to_hue_sat(r, g, b, &hue, &saturation);

    cmd.ieee_addr                = ieee_addr;
    cmd.cmd.type                 = ZB_CMD_SET_HUE_SAT;
    cmd.cmd.hue_sat.hue          = hue;
    cmd.cmd.hue_sat.saturation   = saturation;
    cmd.cmd.hue_sat.trans_ms     = trans_ms;
    return iotdev_zigbee_send_command(&cmd);
}

bool
iotdev_zigbee_get_device_color_rgb(uint64_t ieee_addr, uint8_t *r, uint8_t *g, uint8_t *b)
{
    if ((r == NULL) || (g == NULL) || (b == NULL) || (ieee_addr == 0))
    {
        return false;
    }

    zb_sensor_id_t sensor_id = 0;
    e_zb_function_type_t type = ZB_FUNC_BASIC_INFO;
    if (!zigbee_find_color_func(ieee_addr, &sensor_id, &type))
    {
        return false;
    }

    s_zb_device_t *device = zb_device_manager_find_by_ieee(ieee_addr);
    if (device == NULL)
    {
        return false;
    }
    s_zb_function_t *func = zb_device_manager_find_function(device, sensor_id);
    if ((func == NULL) || (func->ctx == NULL))
    {
        return false;
    }

    /* The two colour drivers keep hue/saturation at the same offsets, but read
     * through the right type rather than assuming that stays true. */
    uint8_t hue, saturation;
    if (type == ZB_FUNC_EXTENDED_COLOR_LIGHT)
    {
        const s_zb_device_extended_color_light_ctx_t *ctx =
            (const s_zb_device_extended_color_light_ctx_t *)func->ctx;
        hue = ctx->hue;
        saturation = ctx->saturation;
    }
    else
    {
        const s_zb_device_color_light_ctx_t *ctx =
            (const s_zb_device_color_light_ctx_t *)func->ctx;
        hue = ctx->hue;
        saturation = ctx->saturation;
    }

    zb_zcl_lighting_hue_sat_to_rgb(hue, saturation, r, g, b);
    return true;
}

/* ----------------------------------------------------------------------
 *  DEVICE SNAPSHOT — blocking reads
 * --------------------------------------------------------------------*/

/* Pack an RGB triple into the single float the state view carries per reading.
 * 0xRRGGBB is exact in a float: the largest value is 0xFFFFFF, and a float
 * represents every integer up to 2^24. */
static float
device_state_pack_rgb(uint8_t r, uint8_t g, uint8_t b)
{
    return (float)(((uint32_t)r << 16) | ((uint32_t)g << 8) | (uint32_t)b);
}

void
iotdev_zigbee_app_color_to_rgb(float value, uint8_t *r, uint8_t *g, uint8_t *b)
{
    if ((r == NULL) || (g == NULL) || (b == NULL))
    {
        return;
    }
    uint32_t packed = (value > 0.0f) ? (uint32_t)(value + 0.5f) : 0u;

    *r = (uint8_t)((packed >> 16) & 0xFFu);
    *g = (uint8_t)((packed >> 8) & 0xFFu);
    *b = (uint8_t)(packed & 0xFFu);
}

/* Append one (class, value) pair to the flattened state view. */
static void
device_state_put(s_iotdev_zigbee_device_state_t *state, e_zb_function_type_t func, float value)
{
    if (state->value_count >= ZB_MAX_FUNCTIONS)
    {
        return;
    }
    state->function[state->value_count]   = (uint16_t)func;
    state->value[state->value_count] = value;
    state->value_count++;
}

/* Flatten one function's cached value into the state view. Most functions
 * produce a single entry; the ones that measure several distinct quantities
 * produce one entry per quantity, each under its own class.
 *
 * Switched on the function type rather than the value type: the class registry
 * is keyed by function type, and one value type serves several function types
 * (ZB_EVENT_LIGHT_ONOFF_STATE covers lights, relays, plugs and outlets), so
 * the value type cannot pick a class on its own. */
static void
device_state_append_value(s_iotdev_zigbee_device_state_t *state, const s_zb_func_value_t *fv)
{
    switch (fv->type)
    {
    /* --- multi-quantity ---------------------------------------------------
     * Electrical measurement carries AC phase A plus the device-wide
     * frequency. Only what the device reports is non-zero; the driver leaves
     * unsupported quantities at 0. */
    case ZB_FUNC_ELECTRICAL:
        device_state_put(state, ZB_FUNC_ELEC_VOLTAGE,        fv->v.electrical.voltage);
        device_state_put(state, ZB_FUNC_ELEC_CURRENT,        fv->v.electrical.current);
        device_state_put(state, ZB_FUNC_ELEC_ACTIVE_POWER,   fv->v.electrical.power);
        device_state_put(state, ZB_FUNC_ELEC_REACTIVE_POWER, fv->v.electrical.reactive_power);
        device_state_put(state, ZB_FUNC_ELEC_APPARENT_POWER, fv->v.electrical.apparent_power);
        device_state_put(state, ZB_FUNC_ELEC_POWER_FACTOR,   fv->v.electrical.power_factor);
        device_state_put(state, ZB_FUNC_ELEC_FREQUENCY,      fv->v.electrical.frequency);
        return;

    /* --- measurement: the `sensor` member ---------------------------------- */
    case ZB_FUNC_TEMPERATURE:
    case ZB_FUNC_HUMIDITY:
    case ZB_FUNC_PRESSURE:
    case ZB_FUNC_ILLUMINANCE:
    case ZB_FUNC_FLOW:
    case ZB_FUNC_PM25:
    case ZB_FUNC_PM10:
    case ZB_FUNC_PM1:
    case ZB_FUNC_CO2:
    case ZB_FUNC_ECO2:
    case ZB_FUNC_TVOC:
    case ZB_FUNC_FORMALDEHYDE:
    case ZB_FUNC_IAQ:
        device_state_put(state, fv->type, fv->v.sensor.value);
        return;

    case ZB_FUNC_ENERGY:
        device_state_put(state, fv->type, fv->v.energy);
        return;

    /* --- binary state ------------------------------------------------------ */
    case ZB_FUNC_OCCUPANCY:
        device_state_put(state, fv->type, fv->v.binary.active ? 1.0f : 0.0f);
        return;

    case ZB_FUNC_IAS_MOTION_SENSOR:
    case ZB_FUNC_IAS_CONTACT_SWITCH:
    case ZB_FUNC_IAS_DOOR_WINDOW_HANDLE:
    case ZB_FUNC_IAS_FIRE_SENSOR:
    case ZB_FUNC_IAS_WATER_SENSOR:
    case ZB_FUNC_IAS_CO_SENSOR:
    case ZB_FUNC_IAS_PERSONAL_EMERGENCY:
    case ZB_FUNC_IAS_VIBRATION_SENSOR:
    case ZB_FUNC_IAS_GENERIC_SENSOR:
        /* Alarm1/Alarm2 are the zone's actual trip bits; the rest of
         * ZoneStatus (tamper, low battery, trouble) is not part of
         * "is it triggered". */
        device_state_put(state, fv->type,
                         (fv->v.ias.zone_status & (SS_IAS_ZONE_STATUS_ALARM1_ALARMED |
                                                   SS_IAS_ZONE_STATUS_ALARM2_ALARMED))
                             ? 1.0f : 0.0f);
        return;

    /* --- on/off actuators --------------------------------------------------- */
    case ZB_FUNC_ONOFF_LIGHT:
    case ZB_FUNC_ONOFF_PLUGIN_UNIT:
    case ZB_FUNC_ONOFF_SWITCH:
    case ZB_FUNC_ONOFF_SMART_PLUG:
    case ZB_FUNC_RELAY:
    case ZB_FUNC_MAINS_POWER_OUTLET:
        device_state_put(state, fv->type, fv->v.onoff.on ? 1.0f : 0.0f);
        return;

    /* --- level / colour actuators -------------------------------------------- */
    case ZB_FUNC_DIMMABLE_LIGHT:
    case ZB_FUNC_DIMMABLE_PLUGIN_UNIT:
    case ZB_FUNC_DIMMER_SWITCH:
        device_state_put(state, fv->type, (float)fv->v.level.level);
        return;

    case ZB_FUNC_COLOR_TEMP_LIGHT:
        device_state_put(state, fv->type, (float)fv->v.color_temp.color_temp);
        return;

    /* Reported as RGB, not as the raw hue/saturation the cluster uses: the
     * state view carries one float per reading, so a bare hue silently dropped
     * saturation and a grey light was indistinguishable from a red one.
     * Brightness is not part of it - that is IOTDEV_APP_LAMP_BRIGHTNESS, from
     * the Level Control function. */
    case ZB_FUNC_COLOR_LIGHT:
    {
        uint8_t r = 0, g = 0, b = 0;
        zb_zcl_lighting_hue_sat_to_rgb(fv->v.hue_sat.hue, fv->v.hue_sat.saturation,
                                       &r, &g, &b);
        device_state_put(state, fv->type, device_state_pack_rgb(r, g, b));
        return;
    }

    /* An extended colour light is both a colour light and a tunable-white one,
     * and only the device knows which mode is live - so report the last known
     * value of each rather than guessing. Two entries, like ZB_FUNC_ELECTRICAL
     * above; the classes are named explicitly because the mapper can only
     * return the headline one. */
    case ZB_FUNC_EXTENDED_COLOR_LIGHT:
    {
        uint8_t r = 0, g = 0, b = 0;
        zb_zcl_lighting_hue_sat_to_rgb(fv->v.ext_color.hue, fv->v.ext_color.saturation,
                                       &r, &g, &b);
        device_state_put(state, ZB_FUNC_COLOR_LIGHT, device_state_pack_rgb(r, g, b));
        device_state_put(state, ZB_FUNC_COLOR_TEMP_LIGHT, (float)fv->v.ext_color.color_temp);
        return;
    }

    case ZB_FUNC_BUTTON:
        device_state_put(state, fv->type, (float)fv->v.button.action);
        return;

    default:
        /* IAS warning, basic info, and anything new: no reading the registry
         * can name. device_state_put() also drops IOTDEV_APP_NONE, so a type
         * added to the mapper but not here still cannot produce a bogus entry. */
        return;
    }
}

/* Mirror the driver's own e_zb_device_status_t onto the facade's distinct
 * (but value-for-value equivalent) e_iotdev_zigbee_device_state_t — kept as
 * an explicit switch rather than a cast so the two enums can drift safely. */
static e_iotdev_zigbee_device_state_t
device_status_to_state(e_zb_device_status_t status)
{
    switch (status)
    {
    case ZB_DEVICE_STATUS_OFFLINE:       return IOTDEV_ZIGBEE_DEVICE_STATE_OFFLINE;
    case ZB_DEVICE_STATUS_ONLINE:        return IOTDEV_ZIGBEE_DEVICE_STATE_ONLINE;
    case ZB_DEVICE_STATUS_UNKNOWN:
    default:                             return IOTDEV_ZIGBEE_DEVICE_STATE_UNKNOWN;
    }
}

/*
 * Friendly product names, keyed by {manufacturer, model}.
 *
 * Most Zigbee devices leave Basic ProductLabel (0x000E) empty - it is an
 * optional attribute and the cheap vendors simply never populate it - so the
 * only identity a device reliably gives up is ManufacturerName (0x0004) and
 * ModelIdentifier (0x0005). Those are part numbers, not names: "TS0601" tells
 * an operator nothing, "_TZE284_vvmbj46n" less than nothing. This table is the
 * same job zigbee2mqtt's converter definitions do with their `description`
 * field, kept deliberately small and local rather than pulled in wholesale.
 *
 * Matching rules, first match wins:
 *   - NULL manufacturer or NULL model is a wildcard for that field, so a
 *     vendor-wide fallback can sit below its specific models.
 *   - Comparison is exact and case-sensitive: these strings come straight off
 *     the wire and vendors are consistent about their own spelling.
 *
 * Keep specific entries above wildcards - the scan does not rank matches.
 */
typedef struct s_zb_device_name_map
{
    const char *manufacturer;   /* Basic 0x0004, NULL = any                  */
    const char *model;          /* Basic 0x0005, NULL = any from this vendor */
    const char *name;           /* what an operator should see               */
} s_zb_device_name_map_t;

static const s_zb_device_name_map_t s_device_name_map[] = {
    /* --- Lumi / Aqara ---------------------------------------------------- */
    { "LUMI", "lumi.sensor_motion.aq2", "Aqara Motion Sensor"          },
    { "LUMI", "lumi.motion.ac02",       "Aqara Motion Sensor P1"       },
    { "LUMI", "lumi.sensor_wleak.aq1",  "Aqara Water Leak Sensor"      },
    { "LUMI", "lumi.remote.b1acn01",    "Aqara Wireless Mini Switch"   },
    { "LUMI", "lumi.light.acn014",      "Aqara Ceiling Light"          },

    /* --- Tuya -------------------------------------------------------------
     * Tuya white-labels aggressively: the model is shared across dozens of
     * resellers and only the _TZxxxx manufacturer prefix distinguishes the
     * actual hardware, so these have to be matched on both fields. */
    { "_TZ3000_wzmuk9ai", "TS011F", "Smart Plug"                       },
    { "_TZ3210_bfwvfyx1", "TS0505B", "RGB+CCT Light"                   },
    { "_TZE284_vvmbj46n", "TS0601", "Temperature & Humidity Sensor"    },

    { "eWeLink", "CK-BL702-AL-01(7009_Z102LG03-1)", "RGB+CCT Light"    },
    { "eWeLink", "CK-BL702-AL-01(7008_Z102LG01-2)", "Color Temperature Light"},

    { "ZG-204ZE", "CK-BL702-MWS-01(7016)", "Tuya Presence Sensor"      },

    { "BRTSystems", "ZTS-V1", "BRT 4in1 Sensor"                        },
    { "BRTSystems", "ZAQ-V1", "BRT 9in1 Sensor"                        },
};

/*
 * Resolve a friendly name for a device, or NULL when the table has no entry.
 *
 * NULL rather than a fabricated string on purpose: the caller decides what an
 * unknown device should be called, and silently inventing a name here would
 * hide the fact that the device is unrecognised.
 */
static const char *
device_name_from_map(const char *manufacturer, const char *model)
{
    for (uint16_t i = 0; i < (uint16_t)ARRAY_SIZE(s_device_name_map); i++)
    {
        const s_zb_device_name_map_t *entry = &s_device_name_map[i];

        if (entry->manufacturer &&
            ((manufacturer == NULL) || (strcmp(entry->manufacturer, manufacturer) != 0)))
        {
            continue;
        }
        if (entry->model &&
            ((model == NULL) || (strcmp(entry->model, model) != 0)))
        {
            continue;
        }
        return entry->name;
    }
    return NULL;
}

static void
descriptor_put_function(s_iotdev_zigbee_device_descriptor_t *desc, uint8_t *func_count, e_zb_function_type_t func)
{
    if (*func_count >= ZB_MAX_FUNCTIONS)
    {
        return;
    }
    desc->function[(*func_count)++] = (uint16_t)func;
}

bool
iotdev_zigbee_get_device_descriptor(uint64_t ieee_addr, s_iotdev_zigbee_device_descriptor_t *desc)
{
    if (desc == NULL)
    {
        return false;
    }

    memset(desc, 0, sizeof(*desc));

    const s_zb_device_t *device = zb_device_manager_acquire_device(ieee_addr);
    if (device == NULL)
    {
        zb_device_manager_release_device();
        return false;
    }
    desc->ieee_addr = device->ieee_addr;

    strlcpy(desc->uid, (char *)device->product_code, sizeof(desc->uid));

    /* ProductLabel when the device bothered to publish one; otherwise fall back
     * to the {manufacturer, model} lookup. Left empty for a device that is in
     * neither - the caller can then show the model, which is at least true. */
    strlcpy(desc->name, device->product_label, sizeof(desc->name));
    if (desc->name[0] == '\0')
    {
        const char *mapped = device_name_from_map(device->manufacturer, device->model);
        if (mapped != NULL)
        {
            strlcpy(desc->name, mapped, sizeof(desc->name));
        }
    }

    strlcpy(desc->model, device->model, sizeof(desc->model));

    strlcpy(desc->manufacturer, device->manufacturer, sizeof(desc->manufacturer));

    strlcpy(desc->serial_no, device->serial_number, sizeof(desc->serial_no));

    strlcpy(desc->fw_version, device->sw_build_id, sizeof(desc->fw_version));

    snprintf(desc->product_rev, sizeof(desc->product_rev), "%u", device->hw_version);

    strlcpy(desc->manufacture_date, device->date_code, sizeof(desc->manufacture_date));

    snprintf(desc->device_type, sizeof(desc->device_type), "%s",
        device->device_type == ZB_DEVICE_TYPE_END_DEVICE ? "EndDevice" : "Router");

    uint8_t count = device->function_count;
    if (count > ZB_MAX_FUNCTIONS)
    {
        count = ZB_MAX_FUNCTIONS;
    }
    uint8_t func_count = 0;
    for (uint8_t i = 0; i < count; i++)
    {
        e_zb_function_type_t type = device->functions[i].type;

        /* Basic-info (identity) and battery are not app-class readings - the
         * former is metadata, the latter is device health - so they are omitted
         * from the descriptor's app[] list. */
        if (type == ZB_FUNC_BASIC_INFO || type == ZB_FUNC_BATTERY)
        {
            continue;
        }

        if (type == ZB_FUNC_ELECTRICAL)
        {
            descriptor_put_function(desc, &func_count, ZB_FUNC_ELEC_VOLTAGE);
            descriptor_put_function(desc, &func_count, ZB_FUNC_ELEC_CURRENT);
            descriptor_put_function(desc, &func_count, ZB_FUNC_ELEC_ACTIVE_POWER);
            descriptor_put_function(desc, &func_count, ZB_FUNC_ELEC_REACTIVE_POWER);
            descriptor_put_function(desc, &func_count, ZB_FUNC_ELEC_APPARENT_POWER);
            descriptor_put_function(desc, &func_count, ZB_FUNC_ELEC_POWER_FACTOR);
            descriptor_put_function(desc, &func_count, ZB_FUNC_ELEC_FREQUENCY);
        }
        else if (type == ZB_FUNC_EXTENDED_COLOR_LIGHT)
        {
            descriptor_put_function(desc, &func_count, ZB_FUNC_COLOR_LIGHT);
            descriptor_put_function(desc, &func_count, ZB_FUNC_COLOR_TEMP_LIGHT);
        }
        else
        {
            descriptor_put_function(desc, &func_count, type);
        }
    }
    desc->function_count = func_count;

    desc->battery_powered = device->battery_powered;
    desc->sleepy_device    = device->sleepy_enabled;
    desc->ota_supported    = device->ota_supported;

    zb_device_manager_release_device();
    return true;
}

bool
iotdev_zigbee_get_device_health(uint64_t ieee_addr, s_iotdev_zigbee_device_health_t *health)
{
    if (health == NULL)
    {
        return false;
    }

    memset(health, 0, sizeof(*health));

    const s_zb_device_t *device = zb_device_manager_acquire_device(ieee_addr);
    if (device == NULL)
    {
        zb_device_manager_release_device();
        return false;
    }

    bool battery_powered = device->battery_powered;

    health->state     = device_status_to_state(device->status);
    health->power_source = device->power_source == POWER_SOURCE_BATTERY ? IOTDEV_ZIGBEE_DEVICE_POWER_BATTERY
                            : device->power_source == POWER_SOURCE_UNKNOWN ? IOTDEV_ZIGBEE_DEVICE_POWER_UNKNOWN
                            : IOTDEV_ZIGBEE_DEVICE_POWER_MAINS;
    health->rssi      = iotdev_zigbee_lqi_to_rssi(device->lqi);
    health->lqi       = device->lqi;
    health->last_seen = device->last_seen;

    zb_device_manager_release_device();

    /* Battery percent lives on the battery function's cached value, not on
     * the device record itself — a second, short lock/unlock. Read directly
     * rather than via emit_state(), so polling health never runs the event
     * path. */
    if (battery_powered)
    {
        s_zb_func_value_t values[ZB_MAX_FUNCTIONS];
        uint8_t value_count = zb_device_manager_read_function_values(ieee_addr, values);
        for (uint8_t i = 0; i < value_count; i++)
        {
            if (values[i].type == ZB_FUNC_BATTERY)
            {
                health->battery = (uint8_t)values[i].v.battery.percent;
                break;
            }
        }
    }

    return true;
}

bool
iotdev_zigbee_get_device_state(uint64_t ieee_addr, s_iotdev_zigbee_device_state_t *state)
{
    if (state == NULL)
    {
        return false;
    }

    memset(state, 0, sizeof(*state));

    const s_zb_device_t *device = zb_device_manager_acquire_device(ieee_addr);
    if (device == NULL)
    {
        zb_device_manager_release_device();
        return false;
    }
    state->timestamp = device->last_seen;
    zb_device_manager_release_device();

    /* Straight read of the function list and their cached values - no
     * emit_state(), so taking a snapshot cannot produce an event. */
    s_zb_func_value_t values[ZB_MAX_FUNCTIONS];
    uint8_t count = zb_device_manager_read_function_values(ieee_addr, values);
    if (count > ZB_MAX_FUNCTIONS)
    {
        count = ZB_MAX_FUNCTIONS;   /* defensive; reader never exceeds this */
    }

    for (uint8_t i = 0; i < count; i++)
    {
        device_state_append_value(state, &values[i]);
    }

    return true;
}

iotdev_zigbee_status_t
iotdev_zigbee_find_function(uint64_t ieee_addr, e_zb_function_type_t function_type, zb_sensor_id_t *sensor_id)
{
    const s_zb_device_t *device;
    uint8_t i;

    if ((ieee_addr == 0) || (sensor_id == NULL))
    {
        return IOTDEV_ZIGBEE_ERR_PARAM;
    }

    device = zb_device_manager_acquire_device(ieee_addr);

    if (device == NULL)
    {
        zb_device_manager_release_device();
        return IOTDEV_ZIGBEE_ERR_NOT_FOUND;
    }

    for (i = 0; i < device->function_count && i < ZB_MAX_FUNCTIONS; i++)
    {
        if (device->functions[i].type == function_type ||
            (device->functions[i].type == ZB_FUNC_EXTENDED_COLOR_LIGHT &&
            (function_type == ZB_FUNC_COLOR_LIGHT || function_type == ZB_FUNC_COLOR_TEMP_LIGHT)))
        {
            *sensor_id = device->functions[i].sensor_id;
            zb_device_manager_release_device();
            return IOTDEV_ZIGBEE_OK;
        }
    }
    zb_device_manager_release_device();
    return IOTDEV_ZIGBEE_ERR_NOT_FOUND;
}

bool
iotdev_zigbee_rgb_to_hue_sat(uint8_t r, uint8_t g, uint8_t b, uint8_t *hue, uint8_t *saturation)
{
    if ((hue == NULL) || (saturation == NULL))
    {
        return false;
    }

    zb_zcl_lighting_rgb_to_hue_sat(r, g, b, hue, saturation);
    return true;
}
