#include "af/zb_af.h"
#include "common/zb_common.h"
#include "core/zb_core.h"
#include "osal/zb_osal.h"
#include "device/zb_device.h"
#include "device/zb_device_manager.h"
#include "device/lighting/zb_device_on_off_light.h"
#include "ota/zb_ota_server.h"
#include "device/zb_device_state_cache.h"
#include "network/zb_network_service.h"
#include "zdo/zb_zdo.h"
#include "zcl/zb_zcl.h"
#include "manu/zb_manu.h"
#include "zcl/zb_zcl_ms.h"
#include "zcl/zb_zcl_general.h"
#include "zcl/zb_zcl_lighting.h"
#include "zcl/zb_zcl_ss.h"
#include "znp/zb_znp.h"
#include "znp/zb_znp_mt_af.h"
#include "znp/zb_znp_mt_app.h"
#include "znp/zb_znp_mt_app_cfg.h"
#include "znp/zb_znp_mt_zdo.h"
#include "znp/zb_znp_mt_sys.h"
#include "znp/zb_znp_mt_util.h"
#include "iotdev_config/include/iotdev_config.h"
#include "iotdev_gpio/include/iotdev_gpio.h"
#include "iotdev_uart/include/iotdev_uart.h"
#include "driver/gpio.h"

#define TAG "ZB_CORE"

/* ZNP coprocessor serial link — board wiring (kept at the integration layer,
 * alongside the RESET/SBL GPIO callbacks, so the ZNP transport stays board
 * agnostic). */
#define ZB_ZNP_UART_PORT     IOTDEV_UART1
#define ZB_ZNP_UART_BAUD     IOTDEV_UART_BAUD_921600
#if ZIGBEE_HUB_BOARD_REV == ZIGBEE_HUB_BOARD_REV_A
#define ZB_ZNP_UART_TX_PIN   GPIO_NUM_2
#define ZB_ZNP_UART_RX_PIN   GPIO_NUM_32
#else
#define ZB_ZNP_UART_TX_PIN   GPIO_NUM_32
#define ZB_ZNP_UART_RX_PIN   GPIO_NUM_35
#endif

/******************************************************************************
 * CONSTANTS
 *****************************************************************************/
 #define ZB_HUB_DEVICE_VERSION            0
 #define ZB_HUB_LATENCY                   0
 #define ZB_HUB_FLAGS                     0
 
 #define ZB_HUB_HW_REVISION               1
 #define ZB_HUB_APP_VERSION               1
 #define ZB_HUB_ZCL_VERSION               BASIC_ZCL_VERSION
 #define ZB_HUB_TIME_ZONE                 8
 #define ZB_HUB_UNIX_EPOCH_OFFSET         946684800

/**
 * @brief Zigbee state machine
 */
typedef enum e_zb_state
{
    ZB_STATE_IDLE = 0,
    ZB_STATE_INIT,
    ZB_STATE_RUNNING,
    ZB_STATE_BOOTLOADER_ENTERING,
    ZB_STATE_ZNP_FIRMWARE_UPDATE,
    ZB_STATE_STOPPING,
    ZB_STATE_STOPPED,
} e_zb_state_t;

/**
 * @brief Callback functions for ZNP MT AF
 */
static int zb_znp_mt_af_data_confirm_cb(const s_zb_znp_mt_af_data_confirm_msg_t* rsp);
static int zb_znp_mt_af_incoming_msg_cb(const s_zb_znp_mt_af_incoming_msg_t* rsp);

/**
 * @brief Callback functions for ZNP MT ZDO
 * @note These callbacks are used to handle the responses from the ZNP MT ZDO commands
 */
static int zb_znp_mt_zdo_mgmt_permit_join_rsp_cb(const s_zb_znp_mt_zdo_mgmt_permit_join_rsp_t* rsp);
static int zb_znp_mt_zdo_leave_ind_cb(const s_zb_znp_mt_zdo_leave_ind_t* rsp);
static int zb_znp_mt_zdo_tc_device_ind_cb(const s_zb_znp_mt_zdo_tc_device_ind_t* rsp);
static int zb_znp_mt_zdo_state_change_ind_cb(const e_zb_znp_mt_zdo_state_t state);
static int zb_znp_mt_zdo_nwk_addr_rsp_cb(const s_zb_znp_mt_zdo_nwk_addr_rsp_t *rsp);
static int zb_znp_mt_zdo_ieee_addr_rsp_cb(const s_zb_znp_mt_zdo_ieee_addr_rsp_t *rsp);
static int zb_znp_mt_zdo_node_desc_rsp_cb(const s_zb_znp_mt_zdo_node_desc_rsp_t *rsp);
static int zb_znp_mt_zdo_active_ep_rsp_cb(const s_zb_znp_mt_zdo_active_ep_rsp_t *rsp);
static int zb_znp_mt_zdo_simple_desc_rsp_cb(const s_zb_znp_mt_zdo_simple_desc_rsp_t *rsp);
static int zb_znp_mt_zdo_mgmt_lqi_rsp_cb(const s_zb_znp_mt_zdo_mgmt_lqi_rsp_t *rsp);
static int zb_znp_mt_zdo_end_device_annce_ind_cb(const s_zb_znp_mt_zdo_end_device_annce_ind_t* rsp);
static int zb_znp_mt_zdo_bind_rsp_cb(const s_zb_znp_mt_zdo_bind_unbind_rsp_t *rsp);
static int zb_znp_mt_zdo_unbind_rsp_cb(const s_zb_znp_mt_zdo_bind_unbind_rsp_t *rsp);
static void zb_core_bind_q_reset(void);
static void zb_core_bind_q_pump(uint32_t now);
static void zb_core_bind_q_on_rsp(const s_zb_znp_mt_zdo_bind_unbind_rsp_t *rsp, bool unbind);

/**
 * @brief Callback functions for ZNP MT SYS
 * @note These callbacks are used to handle the responses from the ZNP MT SYS commands
 */
static int zb_znp_mt_sys_reset_ind_cb(const s_zb_znp_mt_sys_reset_ind_t* rsp);

/**
 * @brief Callback functions for ZNP MT APP
 * @note These callbacks are used to handle the responses from the ZNP MT APP commands
 */
static int zb_znp_mt_app_rs485_data_ind_cb(const s_zb_znp_mt_app_rs485_data_ind_t *rsp);
static int zb_znp_mt_app_rs485_error_ind_cb(const uint8_t *rsp);

/* Upper-layer consumer for raw RS485 bytes received from the ZNP. Registered by
 * iotdev_zigbee.c so the driver layer keeps no dependency on the consumer
 * component (e.g. iotdev_modbus). */
static zb_core_rs485_rx_cb_t s_rs485_rx_cb = NULL;

/* ZNP link-state consumers (see zb_core.h). on_down fires before every reset,
 * on_up only once the coprocessor is stable again - never between two resets of
 * the same bring-up. */
static zb_core_znp_link_cb_t s_znp_link_down_cb = NULL;
static zb_core_znp_link_cb_t s_znp_link_up_cb   = NULL;

/* Tracks what we last told the consumers, so a reset storm cannot emit two
 * downs (or a stable state two ups) and so on_up is only sent after an on_down. */
static bool s_znp_link_reported_up = true;

static void
zb_core_znp_link_down(void)
{
    if (!s_znp_link_reported_up)
    {
        return;
    }
    s_znp_link_reported_up = false;
    if (s_znp_link_down_cb != NULL)
    {
        s_znp_link_down_cb();
    }
}

static void
zb_core_znp_link_up(void)
{
    if (s_znp_link_reported_up)
    {
        return;
    }
    s_znp_link_reported_up = true;
    if (s_znp_link_up_cb != NULL)
    {
        s_znp_link_up_cb();
    }
}

/* True from the moment the ZNP announces a reset (SYS_RESET_IND, whoever caused
 * it) until INIT starts configuring it again. While set, the coprocessor is
 * already in exactly the state a RESET pulse would put it in, so IDLE can skip
 * its own reset: a redundant reset costs a full coprocessor boot and wipes the
 * RS485 line config a second time. Cleared on entry to INIT. */
static volatile bool s_znp_fresh = false;

/* Reset the ZNP into normal (non-SBL) mode. Every zb_znp_set_mode_znp() in the
 * state machine goes through here so the link-down notification can never be
 * forgotten at a new call site.
 *
 * The notification is sent BEFORE the pulse: from that moment the coprocessor
 * is unusable, and a consumer that only learned about it afterwards would have
 * kept writing into a chip that was rebooting. The matching link-up is NOT sent
 * here - a bring-up can contain several resets (this one, then the one inside
 * zb_start_network()), so it is sent when the state machine reaches a stable
 * state. */
static int
zb_core_znp_reset_to_znp_mode(uint32_t timeout_ms)
{
    zb_core_znp_link_down();
    return zb_znp_set_mode_znp(timeout_ms);
}

/**
 * @brief Helper functions for Zigbee network initialization
 */
static int zb_has_form_network(uint32_t *channel_mask);
static int zb_set_nv_startup(uint8_t startup_option);
static int zb_set_bdb_commisioning_channel(uint32_t channel_mask);
static int zb_set_nv_pan_id(uint32_t pan_id);
static int zb_set_nv_dev_type(uint8_t dev_type);
static int zb_start_network(s_zb_config_t *config);
static int zb_register_af(s_zb_config_t *config);
static void zb_init_default_config(void);

/**
 * @brief Helper functions for ZCL application initialization
 */
static zb_status_t zb_core_zcl_application_init(void);
static uint8_t zb_core_zcl_unhandled_cmd_handler(s_zb_zcl_incoming_msg_t *msg);
static uint8_t zb_core_zcl_read_write_attr_callback(uint16_t cluster_id, uint16_t attr_id, uint8_t operation, uint8_t *data, uint16_t *data_len);

/**
 * @brief ZNP coprocessor / network health-monitoring helpers
 */
static void zb_core_fill_coord_info(s_zb_coordinator_info_t *info, uint8_t state);
static void zb_core_set_coordinator_state(e_zb_coordinator_state_t state);
static void zb_core_set_network_state(uint8_t network_state);
static void zb_core_health_monitor(void);

/******************************************************************************
* Global Variables
*****************************************************************************/
// Global attributes
static const uint16_t zcl_zigbee_hub_basic_cluster_revision = 0x0002;
static const uint16_t zcl_zigbee_hub_time_cluster_revision = 0x0001;
// Basic Cluster
static const uint8_t zcl_zigbee_hub_hw_revision = ZB_HUB_HW_REVISION;
static const uint8_t zcl_zigbee_hub_app_version = ZB_HUB_APP_VERSION;
static const uint8_t zcl_zigbee_hub_zcl_version = ZB_HUB_ZCL_VERSION;
static const uint8_t zcl_zigbee_hub_manufacturer_name[] = {11, 'B', 'R', 'T', '-', 'S', 'Y', 'S', 'T', 'E', 'M', 'S'};
static const uint8_t zcl_zigbee_hub_model_name[] = {10, 'Z', 'i', 'g', 'b', 'e', 'e', '-', 'H', 'u', 'b'};
static const uint8_t zcl_zigbee_hub_power_source = POWER_SOURCE_DC;

// Time Cluster
static uint32_t zcl_zigbee_hub_time = 0;
static uint32_t zcl_zigbee_hub_standard_time = 0;
static uint32_t zcl_zigbee_hub_local_time = 0;

/******************************************************************************
* Attributes definitions
*****************************************************************************/
const s_zb_zcl_attr_rec_t zcl_zigbee_hub_attrs[] =
{
    // *** General Basic Cluster Attributes ***
    {
        ZCL_CLUSTER_ID_GENERAL_BASIC,
        { // Attribute record
            ATTRID_BASIC_ZCL_VERSION,
            ZCL_DATATYPE_UINT8,
            ACCESS_CONTROL_READ,
            (void *)&zcl_zigbee_hub_zcl_version
        }
    },
    {
        ZCL_CLUSTER_ID_GENERAL_BASIC,
        {
            ATTRID_BASIC_HW_VERSION,
            ZCL_DATATYPE_UINT8,
            ACCESS_CONTROL_READ,
            (void *)&zcl_zigbee_hub_hw_revision
        }
    },
    {
        ZCL_CLUSTER_ID_GENERAL_BASIC,
        {
            ATTRID_BASIC_APPLICATION_VERSION,
            ZCL_DATATYPE_UINT8,
            ACCESS_CONTROL_READ,
            (void *)&zcl_zigbee_hub_app_version
        }
    },
    {
        ZCL_CLUSTER_ID_GENERAL_BASIC,
        {
            ATTRID_BASIC_MANUFACTURER_NAME,
            ZCL_DATATYPE_CHAR_STR,
            ACCESS_CONTROL_READ,
            (void *)zcl_zigbee_hub_manufacturer_name
        }
    },
    {
        ZCL_CLUSTER_ID_GENERAL_BASIC,
        {
            ATTRID_BASIC_MODEL_IDENTIFIER,
            ZCL_DATATYPE_CHAR_STR,
            ACCESS_CONTROL_READ,
            (void *)zcl_zigbee_hub_model_name
        }
    },
    {
        ZCL_CLUSTER_ID_GENERAL_BASIC,
        {
            ATTRID_BASIC_POWER_SOURCE,
            ZCL_DATATYPE_ENUM8,
            ACCESS_CONTROL_READ,
            (void *)&zcl_zigbee_hub_power_source
        }
    },
    {
        ZCL_CLUSTER_ID_GENERAL_BASIC,
        {
            ATTRID_CLUSTER_REVISION,
            ZCL_DATATYPE_UINT16,
            ACCESS_CONTROL_READ,
            (void *)&zcl_zigbee_hub_basic_cluster_revision
        }
    },
    {
        ZCL_CLUSTER_ID_GENERAL_TIME,
        {
            ATTRID_TIME_TIME,
            ZCL_DATATYPE_UTC,
            ACCESS_CONTROL_READ | ACCESS_CONTROL_WRITE,
            (void *)&zcl_zigbee_hub_time
        }
    },
    {
        ZCL_CLUSTER_ID_GENERAL_TIME,
        {
            ATTRID_TIME_STANDARD_TIME,
            ZCL_DATATYPE_UINT32,
            ACCESS_CONTROL_READ,
            (void *)&zcl_zigbee_hub_standard_time
        }
    },
    {
        ZCL_CLUSTER_ID_GENERAL_TIME,
        {
            ATTRID_TIME_LOCAL_TIME,
            ZCL_DATATYPE_UINT32,
            ACCESS_CONTROL_READ,
            (void *)&zcl_zigbee_hub_local_time
        }
    },
    {
        ZCL_CLUSTER_ID_GENERAL_TIME,
        {
            ATTRID_CLUSTER_REVISION,
            ZCL_DATATYPE_UINT16,
            ACCESS_CONTROL_READ,
            (void *)&zcl_zigbee_hub_time_cluster_revision
        }
    }
};

static uint8_t const zcl_zigbee_hub_num_attrs = sizeof(zcl_zigbee_hub_attrs) / sizeof(zcl_zigbee_hub_attrs[0]);

static e_zb_znp_mt_zdo_state_t g_dev_state = ZNP_ZDO_STATE_HOLD;

/* g_zb_state is read from other tasks via zb_core_get_running_status();
 * keep it `volatile` to suppress compiler-reordering of the read.
 * It is written only from the iotdev_zigbee task that runs zb_core_task(). */
#if FCC_TEST
static volatile e_zb_state_t g_zb_state = ZB_STATE_STOPPING;
#else
static volatile e_zb_state_t g_zb_state = ZB_STATE_IDLE;
#endif

/* State-machine bookkeeping for one-shot on-entry actions.
 *   • s_prev_state / s_prev_state_valid let each case know whether
 *     this is the first iteration we entered the state.
 *   • s_idle_retry_not_before throttles the IDLE → INIT attempt so we
 *     don't hammer zb_znp_set_mode_znp() (which itself blocks 5 s on
 *     failure) once every 10 ms when the ZNP is unresponsive. */
#define ZB_IDLE_RETRY_BACKOFF_MS    2000

/* How long to wait for the coprocessor to come back (SYS_RESET_IND) after a
 * RESET pulse. Every reset in the state machine uses this one bound. */
#define ZB_ZNP_RESET_TIMEOUT_MS     6000

static e_zb_state_t s_prev_state         = ZB_STATE_IDLE;
static bool         s_prev_state_valid   = false;
static uint32_t     s_idle_retry_not_before = 0;

/* Policy: whether to auto-open permit-join for 180 s right after the
 * network forms.  Default true to preserve historical behaviour, but
 * production gateways should turn this off via
 * zb_core_set_auto_permit_join_on_form(false). */
static bool s_auto_permit_join_on_form = false;

/* ---- ZNP coprocessor / network health monitoring -------------------- *
 * s_znp_reset_pending / s_znp_fw_version are written from the ZNP task
 * (zb_znp_mt_sys_reset_ind_cb) and read on the driver task; everything
 * else here is touched only on the driver task.
 *
 *   • Heartbeat: while RUNNING the driver task pings the ZNP every
 *     ZB_HEALTH_PING_INTERVAL_MS.  After ZB_HEALTH_PING_FAIL_LIMIT
 *     consecutive misses the coprocessor is declared offline and the
 *     state machine drops to IDLE to re-establish the link + network.
 *   • Unsolicited reset: an SYS_RESET_IND that arrives while RUNNING
 *     means the coordinator silently restarted; we recover the same way. */
#define ZB_HEALTH_PING_INTERVAL_MS  5000
#define ZB_HEALTH_PING_FAIL_LIMIT   3

static volatile bool s_znp_reset_pending = false;
/* Mgmt_Lqi neighbour-table paging. The response callback runs on the ZNP task
 * and MUST NOT issue a follow-up request (that would deadlock the ZNP task), so
 * when a response reports more entries than it returned it records the next
 * page here; zb_core_task() (a requester task) issues it on the next tick. Only
 * one paging sequence is tracked at a time - we page a single target to
 * completion before another query starts. Driver-task services it; the callback
 * (ZNP task) only sets it. */
static volatile bool     s_lqi_query_start = false;   /* a fresh coordinator query was requested */
static volatile bool     s_lqi_page_pending = false;
static volatile uint16_t s_lqi_page_dst = 0;
static volatile uint8_t  s_lqi_page_next_index = 0;
/* Set on the driver task right before we deliberately reset the ZNP (e.g. the
 * HARDWARE reset issued by zb_start_network() while forming a new network).
 * The resulting SYS_RESET_IND is then consumed as expected instead of being
 * mistaken for an unsolicited coordinator restart.  Driver-task only. */
static bool s_znp_reset_expected = false;
static uint32_t s_znp_fw_version = 0;

/* Diagnostic read handler - see zb_core_set_diag_read_handler(). Driver task
 * only, so no locking is warranted. */
static zb_core_diag_read_cb_t s_diag_read_cb = NULL;
static void *s_diag_read_ctx = NULL;

static uint32_t s_health_next_ping = 0;
static uint8_t s_health_ping_fails = 0;
/* ZNP link state reported via ZB_EVENT_NETWORK_INFO.coordinator_state. The
 * network-formation state is derived from g_zb_state at publish time. */
static e_zb_coordinator_state_t s_coordinator_state = ZB_COORDINATOR_STATE_OFFLINE;

static s_zb_config_t g_zb_config;

static zb_os_queue_t zb_event_queue;

static zb_os_event_t zb_event_group;
static uint32_t ZB_EVENT_RUNNING_BIT = (1u << 0);

static char* g_znp_fw_bin_file = NULL;

static const s_zb_zcl_option_rec_t g_zcl_option_list[] = {
    { .cluster_id = ZCL_CLUSTER_ID_GENERAL_BASIC, .option = AF_ACK_REQUEST },
    { .cluster_id = ZCL_CLUSTER_ID_GENERAL_POWER_CONFIG, .option = AF_ACK_REQUEST },
    { .cluster_id = ZCL_CLUSTER_ID_GENERAL_GROUPS, .option = AF_ACK_REQUEST },
    { .cluster_id = ZCL_CLUSTER_ID_GENERAL_ON_OFF, .option = AF_ACK_REQUEST },
    { .cluster_id = ZCL_CLUSTER_ID_GENERAL_LEVEL_CONTROL, .option = AF_ACK_REQUEST },
    { .cluster_id = ZCL_CLUSTER_ID_LIGHTING_COLOR_CONTROL, .option = AF_ACK_REQUEST },
    { .cluster_id = ZCL_CLUSTER_ID_MS_TEMPERATURE_MEASUREMENT, .option = AF_ACK_REQUEST },
    { .cluster_id = ZCL_CLUSTER_ID_MS_RELATIVE_HUMIDITY, .option = AF_ACK_REQUEST },
    { .cluster_id = ZCL_CLUSTER_ID_MS_ILLUMINANCE_MEASUREMENT, .option = AF_ACK_REQUEST },
    { .cluster_id = ZCL_CLUSTER_ID_MS_OCCUPANCY_SENSING, .option = AF_ACK_REQUEST },
    { .cluster_id = ZCL_CLUSTER_ID_MS_CO2_MEASUREMENT, .option = AF_ACK_REQUEST },
    { .cluster_id = ZCL_CLUSTER_ID_MS_FLOW_MEASUREMENT, .option = AF_ACK_REQUEST },
    { .cluster_id = ZCL_CLUSTER_ID_MS_PM25_MEASUREMENT, .option = AF_ACK_REQUEST },
    { .cluster_id = ZCL_CLUSTER_ID_MS_PM10_MEASUREMENT, .option = AF_ACK_REQUEST },
    { .cluster_id = ZCL_CLUSTER_ID_MS_PM1_MEASUREMENT, .option = AF_ACK_REQUEST },
    { .cluster_id = ZCL_CLUSTER_ID_MS_TVOC_MEASUREMENT, .option = AF_ACK_REQUEST },
    { .cluster_id = ZCL_CLUSTER_ID_MS_FORMALDEHYDE_MEASUREMENT, .option = AF_ACK_REQUEST },
    { .cluster_id = ZCL_CLUSTER_ID_MS_IAQ_MEASUREMENT, .option = AF_ACK_REQUEST },
    { .cluster_id = ZCL_CLUSTER_ID_MS_ECO2_MEASUREMENT, .option = AF_ACK_REQUEST },
    { .cluster_id = ZCL_CLUSTER_ID_MS_PRESSURE_MEASUREMENT, .option = AF_ACK_REQUEST },
    { .cluster_id = ZCL_CLUSTER_ID_SS_IAS_ZONE, .option = AF_ACK_REQUEST },
    { .cluster_id = ZCL_CLUSTER_ID_SS_IAS_WD, .option = AF_ACK_REQUEST },
    { .cluster_id = ZCL_CLUSTER_ID_OTA, .option = AF_ACK_REQUEST },
};

/**
 * @brief Zigbee callbacks
 */
static s_zb_znp_mt_af_cb_t zb_znp_mt_af_cb = {
    .pfn_af_data_confirm_cb = zb_znp_mt_af_data_confirm_cb,
    .pfn_af_incoming_msg_cb = zb_znp_mt_af_incoming_msg_cb,
};

static s_zb_znp_mt_app_cb_t zb_znp_mt_app_cb = {
    .pfn_rs485_data_ind_cb = zb_znp_mt_app_rs485_data_ind_cb,
    .pfn_rs485_error_ind_cb = zb_znp_mt_app_rs485_error_ind_cb,
};

static s_zb_znp_mt_sys_cb_t zb_znp_mt_sys_cb = {
    .pfn_sys_reset_ind_cb = zb_znp_mt_sys_reset_ind_cb,
};

static s_zb_znp_mt_zdo_cb_t zb_znp_mt_zdo_cb = {
    .pfn_zdo_state_change_ind_cb = zb_znp_mt_zdo_state_change_ind_cb,
    .pfn_zdo_simple_desc_rsp_cb = zb_znp_mt_zdo_simple_desc_rsp_cb,
    .pfn_zdo_active_ep_rsp_cb = zb_znp_mt_zdo_active_ep_rsp_cb,
    .pfn_zdo_end_device_annce_ind_cb = zb_znp_mt_zdo_end_device_annce_ind_cb,
    .pfn_zdo_mgmt_permit_join_rsp_cb = zb_znp_mt_zdo_mgmt_permit_join_rsp_cb,
    .pfn_zdo_mgmt_lqi_rsp_cb = zb_znp_mt_zdo_mgmt_lqi_rsp_cb,
    .pfn_zdo_leave_ind_cb = zb_znp_mt_zdo_leave_ind_cb,
    .pfn_zdo_tc_device_ind_cb = zb_znp_mt_zdo_tc_device_ind_cb,
    .pfn_zdo_nwk_addr_rsp_cb = zb_znp_mt_zdo_nwk_addr_rsp_cb,
    .pfn_zdo_ieee_addr_rsp_cb = zb_znp_mt_zdo_ieee_addr_rsp_cb,
    .pfn_zdo_node_desc_rsp_cb = zb_znp_mt_zdo_node_desc_rsp_cb,
    .pfn_zdo_bind_rsp_cb = zb_znp_mt_zdo_bind_rsp_cb,
    .pfn_zdo_unbind_rsp_cb = zb_znp_mt_zdo_unbind_rsp_cb,
};

static void zb_init_default_config(void)
{
    memset(&g_zb_config, 0, sizeof(s_zb_config_t));
    g_zb_config.channel_mask = ZB_HUB_PRIMARY_CHANNEL_MASK;
    g_zb_config.channel = ZB_HUB_CHANNEL;
    g_zb_config.tx_power = 5;
    g_zb_config.endpoint = ZB_HUB_ENDPOINT;
    g_zb_config.pan_id = ZB_HUB_PAN_ID;
    g_zb_config.profile_id = ZCL_HA_PROFILE_ID;
    g_zb_config.device_id = ZCL_DEVICEID_HOME_GATEWAY;
    g_zb_config.num_in_cluster = 3;
    g_zb_config.num_out_cluster = 20;
    g_zb_config.in_cluster_id[0] = ZCL_CLUSTER_ID_GENERAL_BASIC;
    g_zb_config.in_cluster_id[1] = ZCL_CLUSTER_ID_GENERAL_IDENTIFY;
    g_zb_config.in_cluster_id[2] = ZCL_CLUSTER_ID_OTA;
    g_zb_config.out_cluster_id[0] = ZCL_CLUSTER_ID_GENERAL_BASIC;
    g_zb_config.out_cluster_id[1] = ZCL_CLUSTER_ID_GENERAL_POWER_CONFIG;
    g_zb_config.out_cluster_id[2] = ZCL_CLUSTER_ID_GENERAL_IDENTIFY;
    g_zb_config.out_cluster_id[3] = ZCL_CLUSTER_ID_GENERAL_ON_OFF;
    g_zb_config.out_cluster_id[4] = ZCL_CLUSTER_ID_GENERAL_LEVEL_CONTROL;
    g_zb_config.out_cluster_id[5] = ZCL_CLUSTER_ID_LIGHTING_COLOR_CONTROL;
    g_zb_config.out_cluster_id[6] = ZCL_CLUSTER_ID_MS_TEMPERATURE_MEASUREMENT;
    g_zb_config.out_cluster_id[7] = ZCL_CLUSTER_ID_MS_RELATIVE_HUMIDITY;
    g_zb_config.out_cluster_id[8] = ZCL_CLUSTER_ID_MS_ILLUMINANCE_MEASUREMENT;
    g_zb_config.out_cluster_id[9] = ZCL_CLUSTER_ID_MS_CO2_MEASUREMENT;
    g_zb_config.out_cluster_id[10] = ZCL_CLUSTER_ID_MS_PM25_MEASUREMENT;
    g_zb_config.out_cluster_id[11] = ZCL_CLUSTER_ID_MS_PRESSURE_MEASUREMENT;
    g_zb_config.out_cluster_id[12] = ZCL_CLUSTER_ID_SS_IAS_ZONE;
    g_zb_config.out_cluster_id[13] = ZCL_CLUSTER_ID_OTA;
    g_zb_config.out_cluster_id[14] = ZCL_CLUSTER_ID_MS_PM10_MEASUREMENT;
    g_zb_config.out_cluster_id[15] = ZCL_CLUSTER_ID_MS_PM1_MEASUREMENT;
    g_zb_config.out_cluster_id[16] = ZCL_CLUSTER_ID_MS_TVOC_MEASUREMENT;
    g_zb_config.out_cluster_id[17] = ZCL_CLUSTER_ID_MS_FORMALDEHYDE_MEASUREMENT;
    g_zb_config.out_cluster_id[18] = ZCL_CLUSTER_ID_MS_IAQ_MEASUREMENT;
    g_zb_config.out_cluster_id[19] = ZCL_CLUSTER_ID_MS_ECO2_MEASUREMENT;
    g_zb_config.form_new_network = false;
}

static int
zb_znp_mt_af_data_confirm_cb(const s_zb_znp_mt_af_data_confirm_msg_t* rsp)
{
    if (rsp->status == ZB_OK)
    {
        ZB_LOGI(TAG, "AF Data Confirm success, endpoint: %d, trans_id: %d", rsp->endpoint, rsp->trans_id);
    }
    else
    {
        ZB_LOGE(TAG, "AF Data Confirm failed, endpoint: %d, trans_id: %d, status: 0x%02X",
                    rsp->endpoint, rsp->trans_id, rsp->status);
    }

    return rsp->status;
}

static s_zb_af_incoming_msg_t *
zb_copy_af_incoming_msg(const s_zb_znp_mt_af_incoming_msg_t* rsp)
{
    s_zb_af_incoming_msg_t *msg = ZB_MEM_MALLOC(sizeof(s_zb_af_incoming_msg_t));
    if (msg == NULL)
    {
        ZB_LOGE(TAG, "%s(): Failed to allocate memory for message", __func__);
        return NULL;
    }
    msg->group_id = rsp->group_id;
    msg->cluster_id = rsp->cluster_id;
    msg->src_endpoint = rsp->src_endpoint;
    msg->src_addr.address_mode = AF_ADDRESS_16BIT;
    msg->src_addr.short_addr = rsp->src_addr;
    msg->src_addr.endpoint = rsp->src_endpoint;
    msg->dst_endpoint = rsp->dst_endpoint;
    msg->was_broadcast = rsp->was_broadcast;
    msg->link_quality = rsp->link_quality;
    msg->security_use = rsp->security_use;
    msg->timestamp = rsp->timestamp;
    msg->command.data_length = rsp->len;
    msg->command.data = ZB_MEM_MALLOC(rsp->len);
    if (msg->command.data == NULL)
    {
        ZB_LOGE(TAG, "%s(): Failed to allocate memory for command data", __func__);
        ZB_MEM_FREE(msg);
        return NULL;
    }
    memcpy(msg->command.data, rsp->data, rsp->len);
    return msg;
}

static int
zb_znp_mt_af_incoming_msg_cb(const s_zb_znp_mt_af_incoming_msg_t* rsp)
{
    ZB_LOGI(TAG, "AF Incoming Message: Cluster: %04X, Endpoint: %d, Address: %04X, length: %d",
        rsp->cluster_id, rsp->src_endpoint, rsp->src_addr, rsp->len);
    ZB_LOG_BUFFER_HEX(TAG, rsp->data, rsp->len);

    s_zb_af_incoming_msg_t *msg = zb_copy_af_incoming_msg(rsp);

    if (msg == NULL)
    {
        ZB_LOGE(TAG, "No memory for new AF message");
        return ZB_FAIL;
    }

    if (rsp->cluster_id == ZCL_CLUSTER_ID_OTA)
    {
        ZB_LOGI(TAG, "OTA Cluster message received");
        s_zb_event_t event = { 0 };
        event.type = ZB_EVENT_NETWORK_AF_INCOMING_MSG;
        event.af_msg = (void *)msg;
        if (zb_ota_server_queue_add(&event) != ZB_OK)
        {
            ZB_LOGE(TAG, "Send AF message to OTA queue failed");
            zb_af_incoming_msg_free(msg);
            return ZB_FAIL;
        }
        return ZB_OK;
    }

    /* Single ZCL pipeline: one copy, one queue, one parse.
     *
     * ZCL read/write responses used to be copied a second time and pushed onto
     * the network-service queue, where nwksrv re-walked the raw payload with its
     * own attribute parsers. That duplicated both the allocation and the parsing
     * logic (and the bugs in it). nwksrv is not a separate task - zb_nwksrv_task()
     * runs inside zb_core_task() - so there was never any concurrency reason for
     * the split. It now receives the message already parsed, via
     * zb_nwksrv_zcl_observe() from zb_core_zcl_unhandled_cmd_handler(). */
    s_zb_event_t event = { 0 };
    event.type = ZB_EVENT_NETWORK_AF_INCOMING_MSG;
    event.af_msg = (void *)msg;
    if (!zb_os_queue_send(zb_event_queue, &event, 1))
    {
        ZB_LOGE(TAG, "Send AF message to application queue failed");
        zb_af_incoming_msg_free(msg);
        return ZB_FAIL;
    }

    return ZB_OK;
}

static int
zb_znp_mt_app_rs485_data_ind_cb(const s_zb_znp_mt_app_rs485_data_ind_t *rsp)
{
    ZB_LOGI(TAG, "RS485 Data Ind: Length: %d", rsp->len);
    ZB_LOG_BUFFER_HEX(TAG, rsp->data, rsp->len);
    /* Surface the received RS485 bytes to whichever upper-layer consumer
     * registered (e.g. the Modbus RTU master). The driver layer stays free of
     * any other-component dependency - the wiring is done in iotdev_zigbee.c. */
    if (s_rs485_rx_cb != NULL)
    {
        s_rs485_rx_cb(rsp->data, rsp->len);
    }
    return ZB_OK;
}

static int
zb_znp_mt_app_rs485_error_ind_cb(const uint8_t *rsp)
{
    ZB_LOGE(TAG, "RS485 Error Ind: Error: %d", *rsp);
    return ZB_OK;
}

static int
zb_znp_mt_sys_reset_ind_cb(const s_zb_znp_mt_sys_reset_ind_t* rsp)
{
    ZB_LOGW(TAG, "ZNP Reset Ind: Version: %d.%d.%d-%d.%d.%d",
                                            rsp->major_rel, rsp->minor_rel, rsp->maint_rel,
                                            rsp->fw_major_rel, rsp->fw_minor_rel, rsp->fw_maint_rel);

    /* Cache the coprocessor FW version so coordinator-info events can
     * report it (packed fw_major.fw_minor.fw_maint). */
    s_znp_fw_version = ((uint32_t)rsp->fw_major_rel << 16) |
                       ((uint32_t)rsp->fw_minor_rel << 8)  |
                       ((uint32_t)rsp->fw_maint_rel);

    /* This runs on the ZNP task: do NOT touch g_zb_state or issue ZNP
     * commands here.  Flag the reset and let zb_core_task() (driver task)
     * react, keeping the recovery confined to a single task. */
    s_znp_reset_pending = true;
    /* The coprocessor is now in its post-boot state, whoever pulsed RESET. */
    s_znp_fresh = true;
    return ZB_OK;
}

static int
zb_znp_mt_zdo_mgmt_permit_join_rsp_cb(const s_zb_znp_mt_zdo_mgmt_permit_join_rsp_t* rsp)
{
    ZB_LOGI(TAG, "Permit join response from %04X: status %d", rsp->src_addr, rsp->status);
    return rsp->status;
}

/* Zigbee neighbour-table relationship codes (ZDO Mgmt_Lqi_rsp). */
#define ZB_NEIGHBOR_REL_PARENT          0x00
#define ZB_NEIGHBOR_REL_CHILD           0x01
#define ZB_NEIGHBOR_REL_SIBLING         0x02
#define ZB_NEIGHBOR_REL_NONE            0x03
#define ZB_NEIGHBOR_REL_PREVIOUS_CHILD  0x04

static const char *
zb_neighbor_relationship_str(uint8_t relationship)
{
    switch (relationship)
    {
        case ZB_NEIGHBOR_REL_PARENT:         return "parent";
        case ZB_NEIGHBOR_REL_CHILD:          return "child";
        case ZB_NEIGHBOR_REL_SIBLING:        return "sibling";
        case ZB_NEIGHBOR_REL_NONE:           return "none";
        case ZB_NEIGHBOR_REL_PREVIOUS_CHILD: return "prev-child";
        default:                             return "unknown";
    }
}

/*
 * Mgmt_Lqi_rsp: the queried node's neighbour table. For each neighbour we log
 * the relationship and the LQI of that radio link, so a request to the
 * coordinator surfaces the link quality of every directly-joined device to its
 * parent. Runs on the ZNP task - only parses/logs, never issues new ZNP
 * requests (that would deadlock the ZNP task). Paging (start_index > 0) must be
 * driven from an application task.
 */
static int
zb_znp_mt_zdo_mgmt_lqi_rsp_cb(const s_zb_znp_mt_zdo_mgmt_lqi_rsp_t *rsp)
{
    ZB_LOGI(TAG, "Mgmt_Lqi_rsp from %04X: status %d, total %u, start %u, count %u",
            rsp->src_addr, rsp->status, rsp->neighbor_table_entries,
            rsp->start_index, rsp->neighbor_lqi_list_count);

    if (rsp->status != ZB_OK)
    {
        return rsp->status;
    }

    /* IEEE of the node that responded (whose table this is), for subscribers
     * that want to credit the responder rather than the listed neighbours. */
    uint64_t src_ieee = 0;
    if (rsp->src_addr == 0x0000)
    {
        src_ieee = g_zb_config.ieee_addr; /* coordinator */
    }
    else
    {
        s_zb_device_t *s = zb_device_manager_find_by_short_addr(rsp->src_addr);
        src_ieee = s ? s->ieee_addr : 0;
    }

    for (uint8_t i = 0; i < rsp->neighbor_lqi_list_count; i++)
    {
        const s_zb_znp_mt_zdo_neighbor_lqi_list_item_t *n = &rsp->neighbor_lqi_list[i];
        ZB_LOGI(TAG, "  [%u] nwk=%04X ieee=%016llX rel=%s depth=%u lqi=%u",
                (unsigned)(rsp->start_index + i), n->network_addr, n->ext_addr,
                zb_neighbor_relationship_str(n->relationship), n->depth, n->lqi);

        /* Surface each neighbour entry to subscribers (e.g. the prodtest network
         * map). Queued by value like the AF-incoming event; small and self-
         * contained, so no framing/lifetime concerns. */
        s_zb_event_t ev = { 0 };
        ev.type = ZB_EVENT_NETWORK_NEIGHBOR_LQI;
        ev.neighbor_lqi.src_addr     = rsp->src_addr;
        ev.neighbor_lqi.src_ieee     = src_ieee;
        ev.neighbor_lqi.nwk_addr     = n->network_addr;
        ev.neighbor_lqi.ieee_addr    = n->ext_addr;
        ev.neighbor_lqi.relationship = n->relationship;
        ev.neighbor_lqi.depth        = n->depth;
        ev.neighbor_lqi.lqi          = n->lqi;
        if (!zb_os_queue_send(zb_event_queue, &ev, 1))
        {
            ZB_LOGW(TAG, "Neighbour LQI event queue full, dropping entry");
        }
    }

    uint16_t reported = (uint16_t)rsp->start_index + rsp->neighbor_lqi_list_count;
    if (reported < rsp->neighbor_table_entries && rsp->neighbor_lqi_list_count > 0)
    {
        /* More entries remain. Cannot request from here (ZNP task); hand the
         * next page to zb_core_task(). reported == entries already returned ==
         * the StartIndex for the next Mgmt_Lqi_req (Zigbee spec). */
        s_lqi_page_dst = rsp->src_addr;
        s_lqi_page_next_index = (uint8_t)reported;
        s_lqi_page_pending = true;
        ZB_LOGD(TAG, "Mgmt_Lqi paging %04X: %u/%u, next start_index=%u",
                rsp->src_addr, reported, rsp->neighbor_table_entries, (unsigned)reported);
    }

    return rsp->status;
}

static int
zb_znp_mt_zdo_leave_ind_cb(const s_zb_znp_mt_zdo_leave_ind_t* rsp)
{
    ZB_LOGI(TAG, "Leave response from %04X: IEEE %016llX, request %d, remove %d, rejoin %d",
                     rsp->src_addr, rsp->ext_addr, rsp->request, rsp->remove, rsp->rejoin);

    if (rsp->request == 0 && rsp->rejoin == 0)
    {
        ZB_LOGI(TAG, "Device left - remove from network: ieee=%016llX", rsp->ext_addr);
        s_zb_znp_mt_zdo_leave_ind_t *leave_ind = ZB_MEM_MALLOC(sizeof(s_zb_znp_mt_zdo_leave_ind_t));
        if (leave_ind == NULL)
        {
            ZB_LOGE(TAG, "%s(): Failed to allocate memory for leave_ind", __func__);
            return ZB_FAIL;
        }
        memcpy(leave_ind, rsp, sizeof(s_zb_znp_mt_zdo_leave_ind_t));
        s_zb_nwksrv_event_element_t element = {};
        element.response = leave_ind;
        element.event_source = ZB_NWKSRV_EVENT_SOURCE_DEVICE_REMOVE;

        if (zb_nwksrv_event_queue_put(&element) != ZB_OK)
        {
            ZB_MEM_FREE(leave_ind);
            return ZB_FAIL;
        }
        return ZB_OK;
    }
    else
    {
        s_zb_device_t *device = zb_device_manager_find_by_ieee(rsp->ext_addr);
        if (device)
        {
            ZB_LOGI(TAG, "Device left - marked as LEFT: ieee=%016llX", rsp->ext_addr);
        }
    }

    return ZB_OK;
}

static int
zb_znp_mt_zdo_tc_device_ind_cb(const s_zb_znp_mt_zdo_tc_device_ind_t* rsp)
{
    ZB_LOGI(TAG, "TC Device Indication from %04X: IEEE %016llX, parent %04X", rsp->src_addr, rsp->src_ieee_addr, rsp->parent_addr);
    zb_nwksrv_ad_process_device_update_signal(rsp);
    return ZB_OK;
}

static int
zb_znp_mt_zdo_state_change_ind_cb(const e_zb_znp_mt_zdo_state_t state)
{
    switch (state)
    {
    case ZNP_ZDO_STATE_HOLD:
        ZB_LOGI(TAG, "Initialized - not started automatically");
        break;
    case ZNP_ZDO_STATE_INIT:
        ZB_LOGI(TAG, "Initialized - not connected to anything");
        break;
    case ZNP_ZDO_STATE_NWK_DISC:
        ZB_LOGI(TAG, "Discovering PAN's to join");
        break;
    case ZNP_ZDO_STATE_NWK_JOINING:
        ZB_LOGI(TAG, "Joining a PAN");
        break;
    case ZNP_ZDO_STATE_END_DEVICE_UNAUTH:
        ZB_LOGI(TAG, "Network Authenticating");
        break;
    case ZNP_ZDO_STATE_END_DEVICE:
        ZB_LOGI(TAG, "Network Joined");
        break;
    case ZNP_ZDO_STATE_ROUTER:
        ZB_LOGI(TAG, "Device joined, authenticated and is a router");
        break;
    case ZNP_ZDO_STATE_COORD_STARTING:
        ZB_LOGI(TAG, "Starting as Zigbee Coordinator");
        break;
    case ZNP_ZDO_STATE_ZB_COORD:
        ZB_LOGI(TAG, "Started as Zigbee Coordinator");
        break;
    case ZNP_ZDO_STATE_NWK_ORPHAN:
        ZB_LOGI(TAG, "Device has lost information about its parent");
        break;
    default:
        ZB_LOGI(TAG, "Unknown state %d", state);
        break;
    }

    g_dev_state = state;
    if (state == ZNP_ZDO_STATE_ZB_COORD)
    {
        zb_os_event_set(zb_event_group, ZB_EVENT_RUNNING_BIT);
    }
    return ZB_OK;
}

static int
zb_znp_mt_zdo_nwk_addr_rsp_cb(const s_zb_znp_mt_zdo_nwk_addr_rsp_t *rsp)
{
    ZB_LOGI(TAG, "NWK Address Response: Status: %d, NWK Address: %04X", rsp->status, rsp->nwk_addr);
    s_zb_znp_mt_zdo_nwk_addr_rsp_t *nwk_addr_rsp = ZB_MEM_MALLOC(sizeof(s_zb_znp_mt_zdo_nwk_addr_rsp_t));
    if (nwk_addr_rsp == NULL)
    {
        ZB_LOGE(TAG, "%s(): Failed to allocate memory for nwk_addr_rsp", __func__);
        return ZB_FAIL;
    }
    // Copy the first 12 bytes of the response
    memcpy(nwk_addr_rsp, rsp, 12);
    s_zb_nwksrv_event_element_t element = {};
    element.response = nwk_addr_rsp;
    element.event_source = ZB_NWKSRV_EVENT_SOURCE_NWK_ADDR;

    if (zb_nwksrv_event_queue_put(&element) != ZB_OK)
    {
        ZB_MEM_FREE(nwk_addr_rsp);
        return ZB_FAIL;
    }

    return rsp->status;
}

static int
zb_znp_mt_zdo_ieee_addr_rsp_cb(const s_zb_znp_mt_zdo_ieee_addr_rsp_t *rsp)
{
    ZB_LOGI(TAG, "IEEE Address Response: Status: %d, NWK Address: %04X, IEEE Address: %016llX", rsp->status, rsp->nwk_addr, rsp->ieee_addr);
    s_zb_znp_mt_zdo_ieee_addr_rsp_t *ieee_addr_rsp = ZB_MEM_MALLOC(sizeof(s_zb_znp_mt_zdo_ieee_addr_rsp_t));
    if (ieee_addr_rsp == NULL)
    {
        ZB_LOGE(TAG, "%s(): Failed to allocate memory for ieee_addr_rsp", __func__);
        return ZB_FAIL;
    }
    // Copy the first 12 bytes of the response
    memcpy(ieee_addr_rsp, rsp, 12);
    s_zb_nwksrv_event_element_t element = {};
    element.response = ieee_addr_rsp;
    element.event_source = ZB_NWKSRV_EVENT_SOURCE_IEEE_ADDR;

    if (zb_nwksrv_event_queue_put(&element) != ZB_OK)
    {
        ZB_MEM_FREE(ieee_addr_rsp);
        return ZB_FAIL;
    }

    return rsp->status;
}

static int
zb_znp_mt_zdo_node_desc_rsp_cb(const s_zb_znp_mt_zdo_node_desc_rsp_t *rsp)
{
    ZB_LOGI(TAG, "Node Descriptor Response: Status: %d, NWK Address: %04X", rsp->status, rsp->nwk_addr);
    s_zb_znp_mt_zdo_node_desc_rsp_t *node_desc_rsp = ZB_MEM_MALLOC(sizeof(s_zb_znp_mt_zdo_node_desc_rsp_t));
    if (node_desc_rsp == NULL)
    {
        ZB_LOGE(TAG, "%s(): Failed to allocate memory for node_desc_rsp", __func__);
        return ZB_FAIL;
    }

    memcpy(node_desc_rsp, rsp, sizeof(s_zb_znp_mt_zdo_node_desc_rsp_t));
    s_zb_nwksrv_event_element_t element = {};
    element.response = node_desc_rsp;
    element.event_source = ZB_NWKSRV_EVENT_SOURCE_NODE_DESC;

    if (zb_nwksrv_event_queue_put(&element) != ZB_OK)
    {
        ZB_MEM_FREE(node_desc_rsp);
        return ZB_FAIL;
    }
    return rsp->status;
}

static int
zb_znp_mt_zdo_active_ep_rsp_cb(const s_zb_znp_mt_zdo_active_ep_rsp_t *rsp)
{
    ZB_LOGI(TAG, "Active Endpoint Response: Status: %d, NWK Address: %04X", rsp->status, rsp->nwk_addr);
    s_zb_znp_mt_zdo_active_ep_rsp_t *active_ep_rsp = ZB_MEM_MALLOC(sizeof(s_zb_znp_mt_zdo_active_ep_rsp_t));
    if (active_ep_rsp == NULL)
    {
        ZB_LOGE(TAG, "%s(): Failed to allocate memory for active_ep_rsp", __func__);
        return ZB_FAIL;
    }
    memcpy(active_ep_rsp, rsp, sizeof(s_zb_znp_mt_zdo_active_ep_rsp_t));
    s_zb_nwksrv_event_element_t element = {};
    element.response = active_ep_rsp;
    element.event_source = ZB_NWKSRV_EVENT_SOURCE_ACTIVE_EP;

    if (zb_nwksrv_event_queue_put(&element) != ZB_OK)
    {
        ZB_MEM_FREE(active_ep_rsp);
        return ZB_FAIL;
    }
    
    return rsp->status;
}

static int
zb_znp_mt_zdo_simple_desc_rsp_cb(const s_zb_znp_mt_zdo_simple_desc_rsp_t *rsp)
{
    ZB_LOGI(TAG, "Simple Descriptor Response: Status: %d, NWK Address: %04X, Endpoint: %d", rsp->status, rsp->nwk_addr, rsp->endpoint);
    s_zb_znp_mt_zdo_simple_desc_rsp_t *simple_desc_rsp = ZB_MEM_MALLOC(sizeof(s_zb_znp_mt_zdo_simple_desc_rsp_t));
    if (simple_desc_rsp == NULL)
    {
        ZB_LOGE(TAG, "%s(): Failed to allocate memory for simple_desc_rsp", __func__);
        return ZB_FAIL;
    }
    memcpy(simple_desc_rsp, rsp, sizeof(s_zb_znp_mt_zdo_simple_desc_rsp_t));
    s_zb_nwksrv_event_element_t element = {};
    element.response = simple_desc_rsp;
    element.event_source = ZB_NWKSRV_EVENT_SOURCE_SIMPLE_DESC;

    if (zb_nwksrv_event_queue_put(&element) != ZB_OK)
    {
        ZB_MEM_FREE(simple_desc_rsp);
        return ZB_FAIL;
    }

    return rsp->status;
}

static int
zb_znp_mt_zdo_end_device_annce_ind_cb(const s_zb_znp_mt_zdo_end_device_annce_ind_t* rsp)
{
    ZB_LOGI(TAG, "New device commissioned or rejoined NwkAddr: 0x%04X IEEE: %016llX", rsp->nwk_addr, rsp->ieee_addr);

    zb_nwksrv_ad_process_device_announce_signal(rsp->ieee_addr);

    return ZB_OK;
}

static int
zb_znp_mt_zdo_bind_rsp_cb(const s_zb_znp_mt_zdo_bind_unbind_rsp_t *rsp)
{
    zb_core_bind_q_on_rsp(rsp, false);
    return ZB_OK;
}

static int
zb_znp_mt_zdo_unbind_rsp_cb(const s_zb_znp_mt_zdo_bind_unbind_rsp_t *rsp)
{
    zb_core_bind_q_on_rsp(rsp, true);
    return ZB_OK;
}

static int
zb_has_form_network(uint32_t *channel_mask)
{
    s_zb_znp_mt_util_get_nv_info_rsp_t nv_info;
    uint32_t channel_mask_from_nv = 0;
    int status = zb_znp_mt_util_get_nv_info(&nv_info);
    if (status != ZB_OK)
    {
        ZB_LOGE(TAG, "Failed to get NV info");
        return status;
    }
    channel_mask_from_nv = BUILD_UINT32(nv_info.scan_channel[3], nv_info.scan_channel[2], nv_info.scan_channel[1], nv_info.scan_channel[0]);
    if (channel_mask_from_nv & ZB_ALL_CHANNEL_MASK)
    {
        *channel_mask = channel_mask_from_nv & ZB_ALL_CHANNEL_MASK;
    }
    ZB_LOGI(TAG, "NV Info: PAN ID: %04X, Channel Mask: %08X", nv_info.pan_id, *channel_mask);
    if (nv_info.pan_id != 0xFFFF && nv_info.pan_id != 0xFFFE)
    {
        return ZB_OK;
    }
    return ZB_FAIL;
}

static int
zb_set_nv_startup(uint8_t startup_option)
{
    s_zb_znp_mt_sys_osal_nv_write_cmd_t nv_write_cmd;
    nv_write_cmd.id = ZNP_ZCD_NV_STARTUP_OPTION;
    nv_write_cmd.offset = 0;
    nv_write_cmd.len = 1;
    nv_write_cmd.value[0] = startup_option;

    int status = zb_znp_mt_sys_osal_nv_write(&nv_write_cmd);
    ZB_LOGI(TAG, "NV Write Startup Option command sent...[%d]", status);
    return status;
}

static int
zb_set_nv_dev_type(uint8_t dev_type)
{
    s_zb_znp_mt_sys_osal_nv_write_cmd_t nv_write_cmd;
    nv_write_cmd.id = ZNP_ZCD_NV_LOGICAL_TYPE;
    nv_write_cmd.offset = 0;
    nv_write_cmd.len = 1;
    nv_write_cmd.value[0] = dev_type;

    int status = zb_znp_mt_sys_osal_nv_write(&nv_write_cmd);
    ZB_LOGI(TAG, "NV Write Device Type command sent...[%d]", status);
    return status;
}

static int
zb_set_nv_pan_id(uint32_t pan_id)
{
    s_zb_znp_mt_sys_osal_nv_write_cmd_t nv_write_cmd;
    nv_write_cmd.id = ZNP_ZCD_NV_PANID;
    nv_write_cmd.offset = 0;
    nv_write_cmd.len = 2;
    nv_write_cmd.value[0] = LO_UINT16(pan_id);
    nv_write_cmd.value[1] = HI_UINT16(pan_id);

    int status = zb_znp_mt_sys_osal_nv_write(&nv_write_cmd);
    ZB_LOGI(TAG, "NV Write PAN ID command sent...[%d]", status);
    return status;
}

static int
zb_bdb_start_commissioning(uint8_t mode)
{
    int status = zb_znp_mt_app_cfg_bdb_start_commissioning(mode, ZNP_MT_SRSP_BDB_FORMING_TIMEOUT_MS);
    ZB_LOGI(TAG, "BDB Start Commissioning command sent...[%d]", status);
    return status;
}

static int
zb_set_bdb_commisioning_channel(uint32_t channel_mask)
{
    // Set the primary channel
    int status = zb_znp_mt_app_cfg_bdb_set_channel(true, channel_mask);
    ZB_LOGI(TAG, "Set BDB Commisioning Primary Channel command sent...[%08X]", channel_mask);
    if (status != ZB_OK)
    {
        ZB_LOGE(TAG, "Set BDB Commisioning Channel failed");
        return status;
    }

    // Set the secondary channels
    status = zb_znp_mt_app_cfg_bdb_set_channel(false, 0);
    if (status != ZB_OK)
    {
        ZB_LOGE(TAG, "Set BDB Commisioning Channel failed");
        return status;
    }
    return status;
}

static int
zb_set_tx_power(int8_t tx_power)
{
    s_zb_znp_mt_sys_set_tx_power_cmd_t tx_power_cmd;
    tx_power_cmd.tx_power = tx_power;

    int status = zb_znp_mt_sys_set_tx_power(&tx_power_cmd);
    ZB_LOGI(TAG, "Set TX Power command sent...[%d] TX Power: %d", status, tx_power);
    return status;
}

static int
zb_start_network(s_zb_config_t *config)
{
    int status;
    uint8_t bdb_commissioning_mode = 0;

    if (config->form_new_network)
    {
        zb_device_manager_remove_all_devices();
        bdb_commissioning_mode = ZNP_APP_CFG_BDB_COMMISIONING_MODE_NWK_FORMATION;
        status = zb_set_nv_startup(ZNP_ZCD_STARTOPT_CLEAR_CONFIG | ZNP_ZCD_STARTOPT_CLEAR_STATE);

        if (status != ZB_OK)
        {
            ZB_LOGE(TAG, "zb_set_nv_startup failed");
            return status;
        }

        ZB_LOGI(TAG, "Resetting ZNP");
        /* This reset is ours: the SYS_RESET_IND it triggers must be ignored by
         * the unsolicited-reset handler (otherwise the state machine recovers
         * to IDLE right after the network finishes forming). */
        s_znp_reset_expected = true;
        if (zb_core_znp_reset_to_znp_mode(ZB_ZNP_RESET_TIMEOUT_MS) != ZB_OK)
        {
            s_znp_reset_expected = false;
            ZB_LOGE(TAG, "ZNP Reset failed");
            return ZB_FAIL;
        }

        // Start new network
        status = zb_set_nv_dev_type(ZNP_ZCD_DEVICETYPE_COORDINATOR);
        if (status != ZB_OK)
        {
            ZB_LOGE(TAG, "zb_set_nv_dev_type failed");
            return status;
        }

        status = zb_set_nv_pan_id(0xFFFF/*config->pan_id*/);
        if (status != ZB_OK)
        {
            ZB_LOGE(TAG, "zb_set_nv_pan_id failed");
            return status;
        }

        status = zb_set_bdb_commisioning_channel(config->channel_mask);
        if (status != ZB_OK)
        {
            ZB_LOGE(TAG, "zb_set_bdb_commisioning_channel failed");
            return status;
        }
    }

    status = zb_set_tx_power(config->tx_power);
    if (status != ZB_OK)
    {
        ZB_LOGE(TAG, "zb_set_tx_power failed");
        return status;
    }

    s_zb_znp_mt_sys_get_tx_power_srsp_t rsp;
    if (zb_znp_mt_sys_get_tx_power(&rsp) == ZB_OK)
    {
        g_zb_config.tx_power = rsp.tx_power;
    }

    status = zb_bdb_start_commissioning(bdb_commissioning_mode);
    if (status != ZB_OK)
    {
        ZB_LOGE(TAG, "zb_bdb_start_commissioning mode: %d failed", bdb_commissioning_mode);
        return status;
    }

    ZB_LOGI(TAG, " Processing ZDO State Change callbacks");

    zb_os_event_wait(zb_event_group, ZB_EVENT_RUNNING_BIT, true, true, 10000);

    if (g_dev_state != ZNP_ZDO_STATE_ZB_COORD)
    {
        ZB_LOGE(TAG, "Network start failed");
        return ZB_FAIL;
    }

    // Enable ZDO direct callback
    s_zb_znp_mt_sys_osal_nv_write_cmd_t nv_write_cmd;
    nv_write_cmd.id = ZNP_ZCD_NV_ZDO_DIRECT_CB;
    nv_write_cmd.offset = 0;
    nv_write_cmd.len = 1;
    nv_write_cmd.value[0] = 1;
    status = zb_znp_mt_sys_osal_nv_write(&nv_write_cmd);

    status = zb_register_af(config);
    if (status != ZB_OK)
    {
        ZB_LOGE(TAG, "zb_register_af failed");
        return status;
    }

    return ZB_OK;
}

static int
zb_register_af(s_zb_config_t *config)
{
    int status;

    // Register the AF
    s_zb_znp_mt_af_register_cmd_t cmd;
    memset(&cmd, 0, sizeof(s_zb_znp_mt_af_register_cmd_t));
    cmd.endpoint = config->endpoint;
    cmd.profile_id = config->profile_id;
    cmd.device_id = config->device_id;
    cmd.device_version = ZB_HUB_DEVICE_VERSION;
    cmd.num_in_clusters = config->num_in_cluster;
    for (int i = 0; i < config->num_in_cluster; i++)
    {
        cmd.in_cluster_list[i] = config->in_cluster_id[i];
    }
    cmd.num_out_clusters = config->num_out_cluster;
    for (int i = 0; i < config->num_out_cluster; i++)
    {
        cmd.out_cluster_list[i] = config->out_cluster_id[i];
    }
    cmd.latency = ZB_HUB_LATENCY;

    status = zb_znp_mt_af_register(&cmd);
    if (status != ZB_OK)
    {
        ZB_LOGE(TAG, "AF Register failed");
        return status;
    }
    ZB_LOGI(TAG, "AF Register successful");
    return status;
}

/**
 * Set the Zigbee network-formation state. Edge-triggered: on a change it emits
 * the legacy ZB_EVENT_NETWORK_STATE_* signal (kept for back-compat) AND
 * re-publishes the combined ZB_EVENT_NETWORK_INFO (the preferred event, which
 * also carries coordinator_state). Driver-task only.
 */
static void
zb_core_set_network_state(uint8_t network_state)
{
    /* Called at discrete state-machine edges, so no de-dup needed. Callers must
     * have g_zb_state reflecting `network_state` already (publish_network_info
     * derives .state from g_zb_state). Emits the legacy ZB_EVENT_NETWORK_STATE_*
     * signal (kept for back-compat) then the combined ZB_EVENT_NETWORK_INFO. */
    s_zb_event_t evt = {0};
    evt.ieee_addr        = g_zb_config.ieee_addr;
    evt.network_info.channel  = g_zb_config.channel;
    evt.network_info.pan_id   = g_zb_config.pan_id;
    evt.network_info.tx_power = g_zb_config.tx_power;
    switch (network_state)
    {
        case ZB_NETWORK_STATE_INIT: evt.type = ZB_EVENT_NETWORK_STATE_INIT; break;
        case ZB_NETWORK_STATE_RUN:  evt.type = ZB_EVENT_NETWORK_STATE_RUN;  break;
        default:                    evt.type = ZB_EVENT_NETWORK_STATE_STOP; break;
    }
    zb_core_publish_event(&evt);   /* @deprecated legacy signal */

    zb_core_publish_network_info();
}

static void
zb_core_fw_update_progress_cb(uint8_t percent, void *ctx)
{
    (void)ctx;
    s_zb_event_t evt = {0};
    evt.type = ZB_EVENT_COORDINATOR_FW_UPDATE_PROGRESS;
    evt.ieee_addr = g_zb_config.ieee_addr;
    evt.ota_progress.percent = percent;
    evt.ota_progress.phase = ZB_OTA_PROGRESS_PHASE_DOWNLOAD;
    zb_core_publish_event(&evt);
}

/**
 * Fill a coordinator-info snapshot from the cached config + last known
 * ZNP firmware version.  @p state is one of ZB_NETWORK_STATE_*.
 */
static void
zb_core_fill_coord_info(s_zb_coordinator_info_t *info, uint8_t state)
{
    info->state     = state;
    info->channel   = g_zb_config.channel;
    info->tx_power  = g_zb_config.tx_power;
    info->pan_id    = g_zb_config.pan_id;
    info->version   = s_znp_fw_version;
    info->ieee_addr = g_zb_config.ieee_addr;
}

/**
 * Edge-triggered coordinator (ZNP) link-state change. OFFLINE = cannot talk to
 * the ZNP module; FW_UPDATE = in bootloader/flashing; READY = reachable. On a
 * change it re-publishes the combined ZB_EVENT_NETWORK_INFO (which now carries
 * coordinator_state). Driver-task only.
 */
static void
zb_core_set_coordinator_state(e_zb_coordinator_state_t state)
{
    if (state == s_coordinator_state)
    {
        return;
    }
    s_coordinator_state = state;
    ZB_LOGW(TAG, "Coordinator state -> %s",
            state == ZB_COORDINATOR_STATE_READY     ? "READY"
          : state == ZB_COORDINATOR_STATE_FW_UPDATE ? "FW_UPDATE"
                                                    : "OFFLINE");
    zb_core_publish_network_info();
}

/**
 * Periodic ZNP liveness heartbeat, run only while RUNNING.  A successful
 * SYS ping clears the fail counter and (re)asserts ONLINE; repeated
 * failures past ZB_HEALTH_PING_FAIL_LIMIT declare the coprocessor offline
 * and drop the state machine to IDLE so it re-establishes the link and
 * restores the network.  Driver-task only (shares the ZNP request path
 * with the rest of zb_core_task, which all runs on one task).
 */
static void
zb_core_health_monitor(void)
{
    if (zb_os_now_ms() < s_health_next_ping)
    {
        return;
    }
    s_health_next_ping = zb_os_now_ms() + ZB_HEALTH_PING_INTERVAL_MS;

    s_zb_znp_mt_sys_ping_srsp_t ping = {0};
    if (zb_znp_mt_sys_ping(&ping) == ZB_OK)
    {
        if (s_health_ping_fails != 0)
        {
            ZB_LOGI(TAG, "ZNP ping recovered after %d miss(es)", s_health_ping_fails);
        }
        s_health_ping_fails = 0;
        zb_core_set_coordinator_state(ZB_COORDINATOR_STATE_READY);
        return;
    }

    if (++s_health_ping_fails >= ZB_HEALTH_PING_FAIL_LIMIT)
    {
        ZB_LOGE(TAG, "ZNP unresponsive after %d pings - recovering", s_health_ping_fails);
        s_health_ping_fails = 0;
        zb_core_set_coordinator_state(ZB_COORDINATOR_STATE_OFFLINE);
        g_zb_state = ZB_STATE_IDLE;      /* re-establish ZNP link + network */
        s_idle_retry_not_before = 0;     /* recover ASAP */
    }
    else
    {
        ZB_LOGW(TAG, "ZNP ping failed (%d/%d)", s_health_ping_fails, ZB_HEALTH_PING_FAIL_LIMIT);
    }
}

static void
zb_zcl_message_handling(void)
{
    s_zb_event_t zigbee_event = { 0 };
    if (zb_os_queue_recv(zb_event_queue, &zigbee_event, 10))
    {
        switch (zigbee_event.type)
        {
            case ZB_EVENT_NETWORK_AF_INCOMING_MSG:
            {
                e_zcl_proc_msg_status_t status = ZCL_PROC_MSG_INVALID;
                s_zb_af_incoming_msg_t *msg = (s_zb_af_incoming_msg_t *)zigbee_event.af_msg;
                // Find ieee address from msg->src_addr
                uint64_t ieee_addr = 0;
                if (msg->src_addr.address_mode == AF_ADDRESS_64BIT)
                {
                    ieee_addr = msg->src_addr.long_addr;
                }
                else if (msg->src_addr.address_mode == AF_ADDRESS_16BIT)
                {
                    s_zb_device_t *device = zb_device_manager_find_by_short_addr(msg->src_addr.short_addr);
                    if (device)
                    {
                        ieee_addr = device->ieee_addr;
                    }
                }

                /* Liveness/link-quality tracking. This is the one place every
                 * incoming AF frame passes through on the driver task, so update
                 * last_seen and LQI here regardless of whether the frame maps to
                 * a function - covers frames the ZCL layer would otherwise drop
                 * (unknown cluster, no device function). LQI is only available
                 * from an incoming frame, which is why it is tracked here. */
                if (ieee_addr != 0)
                {
                    zb_device_manager_note_activity(ieee_addr, msg->link_quality);
                }
#if FCC_TEST || PROD_TEST
                if (ieee_addr != 0)
                {
                    ZB_LOGI(TAG, "ZCL Message from IEEE Address=0x%016llX", ieee_addr);
                    zigbee_event.ieee_addr = ieee_addr;
                    zb_core_publish_event(&zigbee_event);
                }
#endif
                status = zb_zcl_process_message(msg);
                if (status != ZCL_PROC_MSG_SUCCESS)
                {
                    switch (status)
                    {
                        case ZCL_PROC_MSG_MANUFACTURER_SPECIFIC:
                        case ZCL_PROC_MSG_MANUFACTURER_SPECIFIC_DR:
                            ZB_LOGW(TAG, "ZCL Frame not handled: manufacturer specific");
                            break;
                        case ZCL_PROC_MSG_NOT_HANDLED:
                        case ZCL_PROC_MSG_NOT_HANDLED_DR:
                            ZB_LOGW(TAG, "ZCL Frame not handled: no registered handler");
                            break;
                        default:
                            ZB_LOGE(TAG, "ZCL Frame processing failed: %d", status);
                            break;
                    }
                }
                zb_af_incoming_msg_free(msg);
            }
            break;
            case ZB_EVENT_NETWORK_NEIGHBOR_LQI:
            {
                /* A CHILD entry in src's neighbour table is the authoritative
                 * statement "this neighbour's parent is src" - use it to refresh
                 * the device list's parent addresses (covers roaming and stale
                 * join-time data), then fan the event out to the facade
                 * subscribers here, on the zb_core task, like AF_INCOMING. */
                if (zigbee_event.neighbor_lqi.relationship == ZB_NEIGHBOR_REL_CHILD &&
                    zigbee_event.neighbor_lqi.ieee_addr != 0)
                {
                    /* The neighbour did not itself transmit, so its last_seen /
                     * online / rx are NOT touched. We do refresh its parent
                     * address and its link quality (the reporting parent's fresh
                     * measurement of this child). */
                    uint16_t parent_nwk = zigbee_event.neighbor_lqi.src_addr;
                    uint64_t parent_ieee;
                    if (parent_nwk == 0x0000)
                    {
                        parent_ieee = g_zb_config.ieee_addr; /* coordinator */
                    }
                    else
                    {
                        s_zb_device_t *p = zb_device_manager_find_by_short_addr(parent_nwk);
                        parent_ieee = p ? p->ieee_addr : 0;
                    }
                    zb_device_manager_set_parent(zigbee_event.neighbor_lqi.ieee_addr,
                                                 parent_nwk, parent_ieee);
                    zb_device_manager_set_lqi(zigbee_event.neighbor_lqi.ieee_addr,
                                              zigbee_event.neighbor_lqi.lqi);
                }
                else if (zigbee_event.neighbor_lqi.relationship == ZB_NEIGHBOR_REL_PARENT &&
                         zigbee_event.neighbor_lqi.src_ieee != 0)
                {
                    /* The reporting node (src) just answered a Mgmt_Lqi query, so
                     * it is the device that actually transmitted. Credit only it:
                     * refresh its last_seen + LQI (from its own parent-link
                     * measurement). Keeps a router that sends no app data alive. */
                    zb_device_manager_note_activity(zigbee_event.neighbor_lqi.src_ieee,
                                                    zigbee_event.neighbor_lqi.lqi);
                }
                zb_core_publish_event(&zigbee_event);
            }
            break;
            default:
                ZB_LOGE(TAG, "Invalid event type: %d", zigbee_event.type);
                break;
        }
    }
}

int
zb_core_get_core_temperature(int16_t *temperature)
{
    return zb_znp_mt_sys_get_temperature(temperature);
}

int
zb_core_get_heap_statistics(uint32_t *free_heap, uint32_t *total_heap)
{
    if (zb_core_get_running_status() == false)
    {
        return ZB_FAIL; 
    }
    return zb_znp_mt_sys_get_heap_statistics(free_heap, total_heap);
}

bool
zb_core_get_running_status(void)
{
    return g_zb_state == ZB_STATE_RUNNING;
}

static uint8_t
zb_core_zcl_unhandled_cmd_handler(s_zb_zcl_incoming_msg_t *msg)
{
    uint8_t handled = false;

    /* Offer the parsed message to the device-interview state machines FIRST.
     * A device being commissioned is not in the device pool yet, so every
     * lookup below would fail and the frame would be dropped - including the
     * read responses the interview itself is waiting for, and the attribute
     * reports a device sends right after joining. */
    switch (zb_nwksrv_zcl_observe(msg))
    {
        case ZB_ZCL_OBS_CONSUMED:
            /* An interview owned this frame and has advanced its state. */
            return true;
        case ZB_ZCL_OBS_OBSERVED:
        case ZB_ZCL_OBS_IGNORED:
        default:
            break;
    }

    switch (msg->hdr.command_id)
    {
        case ZCL_CMD_READ_RSP:
        {
            ZB_LOGI(TAG, "ZCL Read RSP from device 0x%04x, ep=%d, cluster=0x%04x",
                msg->msg->src_addr.short_addr, msg->msg->src_endpoint, msg->msg->cluster_id);
            /* Offered to the diagnostic handler as well as the device model: a
             * manual read may target an attribute a function owns, and the
             * person who asked for it still wants to see the answer. */
            if (s_diag_read_cb != NULL)
            {
                s_diag_read_cb(msg, s_diag_read_ctx);
            }
            s_zb_device_t *device = zb_device_manager_find_by_short_addr(msg->msg->src_addr.short_addr);
            if (device)
            {
                zb_sensor_id_t sensor_id = ZB_SENSOR_ID(msg->msg->src_endpoint, msg->msg->cluster_id);
                s_zb_function_t *func = zb_device_manager_find_function(device, sensor_id);
                if (func)
                {
                    func->ops->on_read_rsp(device, func, msg);
                }
                else
                {
                    ZB_LOGW(TAG, "ZCL Read RSP from device 0x%04x, ep=%d, cluster=0x%04x, function not found",
                        msg->msg->src_addr.short_addr, msg->msg->src_endpoint, msg->msg->cluster_id);
                }
            }
            else
            {
                ZB_LOGW(TAG, "ZCL Read RSP from device 0x%04x, ep=%d, cluster=0x%04x, not found",
                    msg->msg->src_addr.short_addr, msg->msg->src_endpoint, msg->msg->cluster_id);
            }
            handled = true;
            break;
        }
        case ZCL_CMD_WRITE_RSP:
        {
            ZB_LOGI(TAG, "ZCL Write RSP from device 0x%04x, ep=%d, cluster=0x%04x",
                msg->msg->src_addr.short_addr, msg->msg->dst_endpoint, msg->msg->cluster_id);
            s_zb_zcl_write_attr_rsp_cmd_t *write_attr_rsp_cmd = (s_zb_zcl_write_attr_rsp_cmd_t *)msg->attr_cmd;
            for (uint8_t i = 0; i < write_attr_rsp_cmd->num_attr; i++)
            {
                s_zb_zcl_write_attr_rsp_info_t *write_attr_rsp_info = &write_attr_rsp_cmd->attr_list[i];
                ZB_LOGI(TAG, "ZCL Write RSP attr_id=0x%04x, status=0x%02x", write_attr_rsp_info->attr_id, write_attr_rsp_info->status);
            }
            handled = true;
            break;
        }
        case ZCL_CMD_CONFIG_REPORT_RSP:
        {
            ZB_LOGI(TAG, "ZCL Config Report RSP from device 0x%04x, ep=%d, cluster=0x%04x",
                msg->msg->src_addr.short_addr, msg->msg->dst_endpoint, msg->msg->cluster_id);
            handled = true;
            break;
        }
        case ZCL_CMD_READ_REPORT_CFG_RSP:
        {
            ZB_LOGI(TAG, "ZCL Read Report CFG RSP from device 0x%04x, ep=%d, cluster=0x%04x",
                msg->msg->src_addr.short_addr, msg->msg->dst_endpoint, msg->msg->cluster_id);
            /* Nothing in the device model consumes this - it exists purely so a
             * human can see what reporting a device really has. */
            if (s_diag_read_cb != NULL)
            {
                s_diag_read_cb(msg, s_diag_read_ctx);
            }
            handled = true;
            break;
        }
        case ZCL_CMD_REPORT:
        {
            ZB_LOGI(TAG, "ZCL Report from device 0x%04x, ep=%d, cluster=0x%04x",
                msg->msg->src_addr.short_addr, msg->msg->src_endpoint, msg->msg->cluster_id);
            s_zb_device_t *device = zb_device_manager_find_by_short_addr(msg->msg->src_addr.short_addr);
            if (device)
            {
                zb_sensor_id_t sensor_id = ZB_SENSOR_ID(msg->msg->src_endpoint, msg->msg->cluster_id);
                s_zb_function_t *func = zb_device_manager_find_function(device, sensor_id);
                if (func)
                {
                    func->ops->on_attr_report(device, func, msg);
                }
                else
                {
                    ZB_LOGW(TAG, "ZCL Report from device 0x%04x, ep=%d, cluster=0x%04x, function not found",
                        msg->msg->src_addr.short_addr, msg->msg->src_endpoint, msg->msg->cluster_id);
                }
            }
            else
            {
                ZB_LOGW(TAG, "ZCL Report from device 0x%04x, ep=%d, cluster=0x%04x, device not found",
                    msg->msg->src_addr.short_addr, msg->msg->src_endpoint, msg->msg->cluster_id);
            }
            handled = true;
            break;
        }
        case ZCL_CMD_DEFAULT_RSP:
        {
            s_zb_zcl_default_rsp_cmd_t *default_rsp_cmd = (s_zb_zcl_default_rsp_cmd_t *)msg->attr_cmd;
            ZB_LOGI(TAG, "ZCL Default RSP from device 0x%04x, ep=%d, cluster=0x%04x, command_id=0x%02x, status=0x%02x",
                msg->msg->src_addr.short_addr, msg->msg->dst_endpoint, msg->msg->cluster_id, default_rsp_cmd->command_id, default_rsp_cmd->status_code);
            handled = true;
            break;
        }
        default:
            break;
    }
    return handled;
}

static zb_status_t
zb_core_zcl_manu_spec_profile_wide_cmd_handler(s_zb_zcl_incoming_msg_t *msg)
{
    ZB_LOGI(TAG, "ZCL MSPW cmd (mfr 0x%04x) from device 0x%04x, ep=%d, cluster=0x%04x",
        msg->hdr.manuf_code, msg->msg->src_addr.short_addr, msg->msg->src_endpoint, msg->msg->cluster_id);

    /* The generic parser rejects payloads it cannot walk safely (unknown data
     * types, truncated records) by returning NULL. The common handler below
     * dereferences attr_cmd, so stop here instead of faulting - a vendor whose
     * frames land here needs its own entry in the zb_zcl_manu table. */
    if (msg->attr_cmd == NULL)
    {
        ZB_LOGW(TAG, "ZCL MSPW cmd (mfr 0x%04x) payload not parsable, dropped - "
                     "register a zb_zcl_manu handler for this manufacturer",
            msg->hdr.manuf_code);
        return ZB_SUCCESS;
    }

    /* Manufacturer-specific attribute reports / read responses (e.g. the
     * Develco/frient air-quality VOC cluster 0xFC03) are dispatched to the
     * owning function exactly like standard ones. The parsed command payload
     * is already attached by the ZCL layer, and msg->hdr.manuf_code lets the
     * driver validate the source manufacturer. */
    zb_core_zcl_unhandled_cmd_handler(msg);
    return ZB_SUCCESS;
}

/* See zb_core.h. Routes the synthesised report through
 * zb_core_zcl_unhandled_cmd_handler(), the same entry point a real one takes. */
void
zb_core_zcl_inject_report(const s_zb_zcl_incoming_msg_t *src, uint16_t cluster_id,
    uint16_t attr_id, uint8_t data_type, const uint8_t *value)
{
    union {
        s_zb_zcl_report_attr_cmd_t cmd;
        uint8_t raw[sizeof(s_zb_zcl_report_attr_cmd_t) + sizeof(s_zb_zcl_report_attr_info_t)];
    } report = {0};

    s_zb_af_incoming_msg_t af_msg = *src->msg;  /* keep src_addr / endpoints */
    af_msg.cluster_id = cluster_id;

    report.cmd.num_attr = 1;
    report.cmd.attr_list[0].attr_id = attr_id;
    report.cmd.attr_list[0].data_type = data_type;
    report.cmd.attr_list[0].attr_data = (uint8_t *)value;

    s_zb_zcl_incoming_msg_t out = {0};
    out.msg = &af_msg;
    out.hdr = src->hdr;
    out.hdr.command_id = ZCL_CMD_REPORT;
    out.hdr.fc.manu_specific = 0;   /* normalised into a standard report */
    out.attr_cmd = &report.cmd;

    ZB_LOGI(TAG, "Injecting decoded attr: ep=%d cluster=0x%04x attr=0x%04x",
        af_msg.src_endpoint, cluster_id, attr_id);
    zb_core_zcl_unhandled_cmd_handler(&out);
}

static uint32_t
zb_core_zcl_get_utc_epoch_time(void)
{
    uint32_t utc_epoch_time = (uint32_t)zb_get_utc_epoch_time();
    if (utc_epoch_time < ZB_HUB_UNIX_EPOCH_OFFSET)
    {
        return 0;
    }
    else
    {
        return utc_epoch_time - ZB_HUB_UNIX_EPOCH_OFFSET;
    }
}

static uint8_t
zb_core_zcl_read_write_attr_callback(uint16_t cluster_id, uint16_t attr_id, uint8_t operation, uint8_t *data, uint16_t *data_len)
{
    if (cluster_id == ZCL_CLUSTER_ID_GENERAL_TIME && operation == ZCL_OPER_READ)
    {
        if (attr_id == ATTRID_TIME_TIME)
        {
            zcl_zigbee_hub_time = zb_core_zcl_get_utc_epoch_time();
        }
        else if (attr_id == ATTRID_TIME_STANDARD_TIME)
        {
            zcl_zigbee_hub_time = zb_core_zcl_get_utc_epoch_time();
            zcl_zigbee_hub_standard_time = zcl_zigbee_hub_time + ZB_HUB_TIME_ZONE * 3600;
        }
        else if (attr_id == ATTRID_TIME_LOCAL_TIME)
        {
            zcl_zigbee_hub_time = zb_core_zcl_get_utc_epoch_time();
            zcl_zigbee_hub_standard_time = zcl_zigbee_hub_time + ZB_HUB_TIME_ZONE * 3600;
            zcl_zigbee_hub_local_time = zcl_zigbee_hub_standard_time;
        }
    }
    return ZCL_STATUS_SUCCESS;
}



/**************************************************************************************************
 * ZCL SS Application Initialization
 **************************************************************************************************/
static zb_status_t
zb_zcl_ss_zone_change_noti_callback(s_zb_zcl_ss_zone_change_notification_t *noti, s_zb_af_address_t *src_addr)
{
    ZB_LOGI(TAG, "ZCL SS Zone Status addr=0x%04x, zone_status=%d", src_addr->short_addr, noti->zone_status);
    s_zb_device_t *device = zb_device_manager_find_by_short_addr(src_addr->short_addr);
    uint8_t buf[sizeof(s_zb_zcl_report_attr_cmd_t) + sizeof(s_zb_zcl_report_attr_info_t)];
    s_zb_zcl_incoming_msg_t msg = {0};
    s_zb_af_incoming_msg_t af_msg = {0};
    s_zb_zcl_report_attr_cmd_t *attr_cmd = (s_zb_zcl_report_attr_cmd_t *)buf;
    attr_cmd->num_attr = 1;
    attr_cmd->attr_list[0].attr_id = ATTRID_IAS_ZONE_ZONE_STATUS;
    attr_cmd->attr_list[0].data_type = ZCL_DATATYPE_UINT16;
    attr_cmd->attr_list[0].attr_data = (uint8_t *)&noti->zone_status;

    af_msg.cluster_id = ZCL_CLUSTER_ID_SS_IAS_ZONE;
    msg.msg = &af_msg;

    msg.attr_cmd = attr_cmd;
    if (device)
    {
        zb_sensor_id_t sensor_id = ZB_SENSOR_ID(src_addr->endpoint, ZCL_CLUSTER_ID_SS_IAS_ZONE);
        s_zb_function_t *func = zb_device_manager_find_function(device, sensor_id);
        if (func)
        {
            func->ops->on_attr_report(device, func, &msg);
        }
        else
        {
            ZB_LOGW(TAG, "ZCL SS Zone Status addr=0x%04x, zone_status=%d, function not found", src_addr->short_addr, noti->zone_status);
        }
    }
    else
    {
        ZB_LOGW(TAG, "ZCL SS Zone Status addr=0x%04x, zone_status=%d, device not found", src_addr->short_addr, noti->zone_status);
    }
    return ZB_SUCCESS;
}

static s_zb_zcl_ss_app_callbacks_t g_zcl_ss_app_callbacks = {
    .pfn_zone_change_notification = zb_zcl_ss_zone_change_noti_callback,
};

static zb_status_t
zb_core_zcl_application_init(void)
{
    zb_zcl_register_attr_list(ZB_HUB_ENDPOINT, zcl_zigbee_hub_num_attrs, zcl_zigbee_hub_attrs);
    zb_zcl_register_external_cmd_handler(ZB_HUB_ENDPOINT, zb_core_zcl_unhandled_cmd_handler);
    zb_zcl_register_manu_spec_profile_wide_cmd_handler(zb_core_zcl_manu_spec_profile_wide_cmd_handler);
    /* Per-vendor frame decoders live under zigbee_driver/manu/ and register
     * themselves into the ZCL manufacturer dispatch table. */
    zb_manu_register_all();
    zb_zcl_ss_register_command_callbacks(ZB_HUB_ENDPOINT, &g_zcl_ss_app_callbacks);
    zb_zcl_register_cluster_option_list(ZB_HUB_ENDPOINT, sizeof(g_zcl_option_list) / sizeof(g_zcl_option_list[0]), g_zcl_option_list);
    zb_zcl_register_read_write_callback(ZB_HUB_ENDPOINT, zb_core_zcl_read_write_attr_callback, NULL);
    return ZB_SUCCESS;
}

void
zb_core_init(void)
{
    ZB_LOGI(TAG, "Initializing Zigbee Core");
    // Initialize ZNP UART
    s_zb_znp_config_t znp_cfg =
    {
        .uart_port = ZB_ZNP_UART_PORT,
        .uart_baud = ZB_ZNP_UART_BAUD,
        .uart_tx_pin = ZB_ZNP_UART_TX_PIN,
        .uart_rx_pin = ZB_ZNP_UART_RX_PIN,
        .uart_rts_pin = GPIO_NUM_NC,
        .uart_cts_pin = GPIO_NUM_NC,
        .rx_stream_size = 0,    // use driver default
        .reset_on_init = false, // core state machine drives reset via mode change
        .pfn_set_reset_pin = iotdev_gpio_set_coprocessor_reset_pin,
        .pfn_set_sbl_pin = iotdev_gpio_set_coprocessor_boot_pin,
    };
    if (zb_znp_init(&znp_cfg) != ZB_OK)
    {
        ZB_LOGE(TAG, "ZNP init failed");
        return;
    }
    /* Before the device manager restores its devices: each one's cached values
     * are pushed into its functions as it is built. */
    zb_device_state_cache_init();
    zb_ota_server_init();
    zb_nwksrv_init();
    zb_device_manager_init();
    // Initialize AF transaction ID management
    zb_af_trans_id_init();
    zb_event_queue = zb_os_queue_create(ZB_EVENT_QUEUE_SIZE, sizeof(s_zb_event_t));
    zb_event_group = zb_os_event_create();

    zb_init_default_config();

    zb_znp_mt_af_register_callback(zb_znp_mt_af_cb);
    zb_znp_mt_app_register_callback(zb_znp_mt_app_cb);
    zb_znp_mt_sys_register_callback(zb_znp_mt_sys_cb);
    zb_znp_mt_zdo_register_callback(zb_znp_mt_zdo_cb);
}

void
zb_core_deinit(void)
{
    zb_nwksrv_deinit();
    zb_ota_server_deinit();
    zb_znp_mt_af_unregister_callback();
    zb_znp_mt_sys_unregister_callback();
    zb_znp_mt_zdo_unregister_callback();
}

/******************************************************************************
 * Post-network-up device state sync (per-function poll scheduler)
 *
 * Each function carries its own cadence in func->poll_interval_ms (resolved at
 * build time from a type default + Device Quirk Register override):
 *   ZB_POLL_NEVER - actuator / no readable state: never read.
 *   0             - read once on first sync, then rely on the device's own
 *                   attribute reports.
 *   N (ms)        - poll every N ms (reporting unconfigured/unsupported).
 * Every readable function still gets one initial read, gated by func->synced —
 * which is also where electrical/energy pull their static formatting attrs.
 *
 * The scheduler examines one function per eligible tick (round-robin cursor)
 * and issues at most one read per ZB_SYNC_READ_GAP_MS, so a batch of
 * simultaneously-due functions does not flood the coprocessor/network.
 ******************************************************************************/
#define ZB_SYNC_READ_GAP_MS             200u

/* Cadence of the per-function `tick` sweep (see zb_core_tick_functions). 1 Hz
 * is fine for the only consumer so far - an occupancy timeout measured in tens
 * of seconds - and keeps the pool walk off the hot path. */
#define ZB_FUNC_TICK_PERIOD_MS          1000u
#define ZB_BIND_UNBIND_RSP_TIMEOUT_MS   7500u
#define ZB_BIND_UNBIND_POST_GAP_MS      50u

#define ZB_BIND_UNBIND_Q_SIZE           8u

typedef struct
{
    bool     unbind;
    uint16_t dev_nwk;
    uint64_t src_ieee;
    uint8_t  src_ep;
    uint64_t dst_ieee;
    uint8_t  dst_ep;
    uint16_t cluster;
    bool     for_config_apply;
} s_zb_bind_unbind_q_item_t;

static struct
{
    s_zb_bind_unbind_q_item_t items[ZB_BIND_UNBIND_Q_SIZE];
    uint8_t                   count;
    bool                      in_flight;
    bool                      flight_unbind;
    uint16_t                  flight_nwk;
    uint32_t                  flight_since_ms;
    uint32_t                  next_send_ms;
    volatile bool             rsp_received;
    volatile uint8_t          rsp_status;
} s_bind_q;

static void
zb_core_bind_q_reset(void)
{
    memset(&s_bind_q, 0, sizeof(s_bind_q));
}

static void
zb_core_bind_q_complete_front(uint8_t status)
{
    if (s_bind_q.count == 0)
    {
        return;
    }

    const s_zb_bind_unbind_q_item_t *item = &s_bind_q.items[0];
    if (!item->unbind && item->for_config_apply)
    {
        s_zb_device_t *dev = zb_device_manager_find_by_short_addr(item->dev_nwk);
        if (dev != NULL && dev->config_bind_in_flight)
        {
            dev->config_bind_in_flight = false;
            dev->config_apply_idx++;
            if (dev->config_apply_idx >= dev->desired_config_count)
            {
                dev->config_applied = true;
            }
        }
    }

    if (status == ZB_SUCCESS)
    {
        ZB_LOGI(TAG, "%s complete 0x%04x ep%u cl 0x%04x",
                item->unbind ? "Unbind" : "Bind", item->dev_nwk, item->src_ep, item->cluster);
    }
    else
    {
        ZB_LOGW(TAG, "%s failed 0x%04x ep%u cl 0x%04x (status 0x%02x)",
                item->unbind ? "Unbind" : "Bind", item->dev_nwk, item->src_ep, item->cluster, status);
    }

    for (uint8_t i = 1; i < s_bind_q.count; i++)
    {
        s_bind_q.items[i - 1] = s_bind_q.items[i];
    }
    s_bind_q.count--;
    s_bind_q.in_flight    = false;
    s_bind_q.next_send_ms = zb_os_now_ms() + ZB_BIND_UNBIND_POST_GAP_MS;
}

static void
zb_core_bind_q_on_rsp(const s_zb_znp_mt_zdo_bind_unbind_rsp_t *rsp, bool unbind)
{
    if (!s_bind_q.in_flight || rsp == NULL || unbind != s_bind_q.flight_unbind)
    {
        return;
    }
    if (rsp->src_addr != s_bind_q.flight_nwk)
    {
        ZB_LOGW(TAG, "Unexpected %s rsp from 0x%04x (expected 0x%04x)",
                unbind ? "unbind" : "bind", rsp->src_addr, s_bind_q.flight_nwk);
        return;
    }
    s_bind_q.rsp_status   = rsp->status;
    s_bind_q.rsp_received = true;
}

static void
zb_core_bind_q_pump(uint32_t now)
{
    if (s_bind_q.in_flight)
    {
        if (s_bind_q.rsp_received)
        {
            s_bind_q.rsp_received = false;
            zb_core_bind_q_complete_front(s_bind_q.rsp_status);
            return;
        }
        if ((int32_t)(now - s_bind_q.flight_since_ms) >= (int32_t)ZB_BIND_UNBIND_RSP_TIMEOUT_MS)
        {
            ZB_LOGW(TAG, "%s rsp timeout for 0x%04x",
                    s_bind_q.flight_unbind ? "Unbind" : "Bind", s_bind_q.flight_nwk);
            zb_core_bind_q_complete_front(ZB_FAILURE);
        }
        return;
    }

    if (s_bind_q.count == 0 || (int32_t)(now - s_bind_q.next_send_ms) < 0)
    {
        return;
    }

    const s_zb_bind_unbind_q_item_t *item = &s_bind_q.items[0];
    s_zb_af_address_t dst = {0};
    dst.address_mode = AF_ADDRESS_64BIT;
    dst.long_addr    = item->dst_ieee;
    dst.endpoint     = item->dst_ep;

    ZB_LOGI(TAG, "%s queue send 0x%04x ep%u cl 0x%04x -> 0x%llx ep%u",
            item->unbind ? "Unbind" : "Bind", item->dev_nwk, item->src_ep, item->cluster,
            (unsigned long long)item->dst_ieee, item->dst_ep);

    int rc;
    if (item->unbind)
    {
        rc = zb_zdo_unbind_request(item->dev_nwk, item->src_ieee, item->src_ep, &dst, item->cluster);
    }
    else
    {
        rc = zb_zdo_bind_request(item->dev_nwk, item->src_ieee, item->src_ep, &dst, item->cluster);
    }
    if (rc != ZB_OK)
    {
        ZB_LOGW(TAG, "%s queue SRSP failed for 0x%04x (%d)",
                item->unbind ? "Unbind" : "Bind", item->dev_nwk, rc);
        zb_core_bind_q_complete_front(ZB_FAILURE);
        return;
    }

    s_bind_q.in_flight       = true;
    s_bind_q.flight_unbind   = item->unbind;
    s_bind_q.flight_nwk      = item->dev_nwk;
    s_bind_q.flight_since_ms = now;
    s_bind_q.rsp_received    = false;
}

static zb_status_t
zb_core_bind_unbind_enqueue(bool unbind, uint16_t dev_nwk_addr, uint64_t src_ieee,
        uint8_t src_endpoint, uint64_t dst_ieee, uint8_t dst_endpoint,
        uint16_t cluster_id, bool for_config_apply)
{
    if (s_bind_q.count >= ZB_BIND_UNBIND_Q_SIZE)
    {
        return ZB_BUFFER_FULL;
    }

    s_zb_bind_unbind_q_item_t *item = &s_bind_q.items[s_bind_q.count++];
    item->unbind            = unbind;
    item->dev_nwk           = dev_nwk_addr;
    item->src_ieee          = src_ieee;
    item->src_ep            = src_endpoint;
    item->dst_ieee          = dst_ieee;
    item->dst_ep            = dst_endpoint;
    item->cluster           = cluster_id;
    item->for_config_apply  = for_config_apply;
    return ZB_OK;
}

zb_status_t
zb_core_bind_enqueue(uint16_t dev_nwk_addr, uint64_t src_ieee, uint8_t src_endpoint,
                     uint64_t dst_ieee, uint8_t dst_endpoint, uint16_t cluster_id,
                     bool for_config_apply)
{
    return zb_core_bind_unbind_enqueue(false, dev_nwk_addr, src_ieee, src_endpoint,
            dst_ieee, dst_endpoint, cluster_id, for_config_apply);
}

zb_status_t
zb_core_unbind_enqueue(uint16_t dev_nwk_addr, uint64_t src_ieee, uint8_t src_endpoint,
                       uint64_t dst_ieee, uint8_t dst_endpoint, uint16_t cluster_id)
{
    return zb_core_bind_unbind_enqueue(true, dev_nwk_addr, src_ieee, src_endpoint,
            dst_ieee, dst_endpoint, cluster_id, false);
}

static uint16_t s_sched_dev_idx  = 0;   /* pool slot cursor              */
static uint8_t  s_sched_func_idx = 0;   /* function cursor within device */
static uint32_t s_sched_next_ms  = 0;   /* read-gap pacing gate          */

/* Advance the round-robin cursor and return the function it now points at,
 * with its owning device. Skips empty pool slots and wraps to the start.
 * Bounded so an empty / zero-function pool cannot spin. Returns NULL only when
 * no readable device exists. */
static s_zb_function_t *
zb_core_sched_advance(s_zb_device_t **out_device)
{
    for (uint32_t steps = 0; steps <= (uint32_t)ZB_MAX_DEVICE + 1; steps++)
    {
        s_zb_device_t *device = zb_device_manager_find_device_from_start_index(&s_sched_dev_idx);
        if (!device)
        {
            if (s_sched_dev_idx == 0)
                return NULL;            /* pool empty */
            s_sched_dev_idx  = 0;       /* wrap to start */
            s_sched_func_idx = 0;
            continue;
        }
        if (s_sched_func_idx < device->function_count)
        {
            s_zb_function_t *func = &device->functions[s_sched_func_idx++];
            *out_device = device;
            return func;
        }
        s_sched_dev_idx++;              /* exhausted this device → next slot */
        s_sched_func_idx = 0;
    }
    return NULL;
}

static bool
zb_core_func_due(const s_zb_function_t *func, uint32_t now)
{
    if (func->poll_interval_ms == ZB_POLL_NEVER)
        return false;
    if (!func->synced)
        return true;                                   /* initial read pending */
    if (func->poll_interval_ms == 0)
        return false;                                  /* read-once, rely on reports */
    return (int32_t)(now - func->last_read_ms) >= (int32_t)func->poll_interval_ms;
}

/* On entering RUNNING: make every poll-capable function due now, so live state
 * refreshes promptly after a (re)form. Does NOT clear func->synced, so static
 * attributes are not re-fetched across a coprocessor recovery. */
static void
zb_core_sync_arm(void)
{
    uint32_t now = zb_os_now_ms();
    uint16_t idx = 0;
    s_zb_device_t *device;
    while ((device = zb_device_manager_find_device_from_start_index(&idx)) != NULL)
    {
        device->config_applied = true;
        device->config_apply_idx = 0;
        device->config_bind_in_flight = false;
        for (uint8_t i = 0; i < device->function_count; i++)
        {
            s_zb_function_t *func = &device->functions[i];
            if (func->poll_interval_ms != ZB_POLL_NEVER && func->poll_interval_ms != 0)
                func->last_read_ms = now - func->poll_interval_ms; /* due now */
        }
        idx++;
    }
    s_sched_dev_idx  = 0;
    s_sched_func_idx = 0;
    s_sched_next_ms  = now;
    zb_core_bind_q_reset();
    ZB_LOGI(TAG, "Device state sync armed");
}

/* Drive every function's `tick` op once a second.
 *
 * Deliberately separate from the polling scheduler below: that one walks a
 * single function per call and returns early for end devices, whereas the
 * functions that need ticking (occupancy timing out an Aqara motion report)
 * are exactly the sleepy end-device ones it skips. The sweep is bounded by the
 * device pool and runs at 1 Hz, so the cost is negligible next to the ZNP
 * traffic the same task handles. */
static void
zb_core_tick_functions(uint32_t now)
{
    static uint32_t s_tick_next_ms = 0;

    if ((int32_t)(now - s_tick_next_ms) < 0)
        return;
    s_tick_next_ms = now + ZB_FUNC_TICK_PERIOD_MS;

    uint16_t idx = 0;
    for (s_zb_device_t *device = zb_device_manager_find_device_from_start_index(&idx);
         device != NULL;
         idx++, device = zb_device_manager_find_device_from_start_index(&idx))
    {
        for (uint8_t f = 0; f < device->function_count && f < ZB_MAX_FUNCTIONS; f++)
        {
            s_zb_function_t *func = &device->functions[f];
            if (func->ops && func->ops->tick)
            {
                func->ops->tick(device, func, now);
            }
        }
    }
}

void
zb_core_query_device_task()
{
    uint32_t now = zb_os_now_ms();

    zb_core_bind_q_pump(now);
    zb_core_tick_functions(now);

    /* Pace reads — at most one per gap (signed compare tolerates wrap). */
    if ((int32_t)(now - s_sched_next_ms) < 0)
        return;

    s_zb_device_t *device = NULL;
    s_zb_function_t *func = zb_core_sched_advance(&device);
    if (!func)
        return;                       /* no devices yet */

    /* Re-apply persisted bind/report intent once per session, before polling
     * (best-effort; sleepy devices receive it via their parent's indirect
     * queue). The device holds the live config — this restores it after a
     * factory-reset/rejoin or a wiped coprocessor NVM. */
    if (!device->config_applied)
    {
        if (device->desired_config_count > 0
                && device->config_apply_idx < device->desired_config_count)
        {
            const s_zb_desired_config_t *cfg =
                &device->desired_config[device->config_apply_idx];

            if (cfg->kind == ZB_DESIRED_CFG_BIND)
            {
                if (!device->config_bind_in_flight)
                {
                    zb_status_t st = zb_device_manager_apply_desired_config_entry(device, cfg, true);
                    if (st == ZB_OK)
                    {
                        device->config_bind_in_flight = true;
                    }
                    else
                    {
                        device->config_apply_idx++;
                        if (device->config_apply_idx >= device->desired_config_count)
                        {
                            device->config_applied = true;
                        }
                    }
                }
                return;
            }

            zb_device_manager_apply_desired_config_entry(device, cfg, true);
            device->config_apply_idx++;
            s_sched_next_ms = now + ZB_SYNC_READ_GAP_MS;
            if (device->config_apply_idx >= device->desired_config_count)
            {
                device->config_applied = true;
            }
            return;
        }
        device->config_applied = true;
        s_sched_next_ms = now + ZB_SYNC_READ_GAP_MS;
        return;
    }

    /* Sleepy/end devices will not answer an immediate read; they surface state
     * through their own reports/check-ins. */
    if (device->device_type == ZB_DEVICE_TYPE_END_DEVICE)
        return;

    if (!zb_core_func_due(func, now))
        return;                       /* examined one; try the next on the following tick */

    if (func->ops && func->ops->on_command)
    {
        s_zb_cmd_t cmd = { .type = ZB_CMD_READ_STATE };
        zb_status_t status = func->ops->on_command(device, func, &cmd);
        ZB_LOGI(TAG, "poll 0x%llx %s (every %u ms) -> %d", device->ieee_addr, func->name, func->poll_interval_ms, status);
    }
    func->synced = true;
    func->last_read_ms = now;
    s_sched_next_ms = now + ZB_SYNC_READ_GAP_MS;
}

void
zb_core_task()
{

    /* "Did we just enter this state?"  Used to make per-state actions
     * one-shot (logging, ZB_EVENT_NETWORK_STATE_* publishing, etc.). */
    e_zb_state_t state         = g_zb_state;
    bool         entering      = (!s_prev_state_valid) || (state != s_prev_state);
    bool         next_state_set = false; /* set true whenever a case re-assigns g_zb_state */

    /* React to an unsolicited ZNP coprocessor reset detected on the ZNP
     * task.  While RUNNING this means the coordinator silently restarted
     * (watchdog / brown-out) so the formed network is gone: declare it
     * offline and drop to IDLE to re-establish the link and restore the
     * network.  In any other state a reset is expected (we issue our own
     * during bring-up / mode switches), so we just consume the flag. */
    if (s_znp_reset_pending)
    {
        s_znp_reset_pending = false;

        /* The coprocessor rebooted, so everything the host pushed into it is
         * gone - notably the RS485 line config. An unsolicited reset never goes
         * through zb_core_znp_reset_to_znp_mode(), so mark the link down here.
         * (For a solicited one this is a no-op: it is already down.) The link
         * comes back up when the state machine reaches a stable state. */
        zb_core_znp_link_down();

        if (s_znp_reset_expected)
        {
            /* We issued this reset ourselves (e.g. forming a new network).
             * It may only surface here a tick later, by which time the state
             * machine has already advanced to RUNNING — so consume it instead
             * of treating it as a coordinator failure. */
            s_znp_reset_expected = false;
            ZB_LOGI(TAG, "Solicited ZNP reset consumed");
        }
        else if (g_zb_state == ZB_STATE_RUNNING)
        {
            ZB_LOGE(TAG, "Unsolicited ZNP reset while running - recovering");
            zb_core_set_coordinator_state(ZB_COORDINATOR_STATE_OFFLINE);
            g_zb_state = ZB_STATE_IDLE;
            s_idle_retry_not_before = 0;
            /* Run the IDLE branch this tick. */
            state    = ZB_STATE_IDLE;
            entering = true;
        }
    }

    /* Drive Mgmt_Lqi queries here (a requester task): the fresh-query request
     * (from the facade) and each paging continuation (deferred from the ZNP-task
     * response callback, which can't issue requests itself). Gated so LQI - a
     * low-priority diagnostic - never competes with a device interview: while
     * commissioning we leave the flags set and retry on a later tick, so an
     * in-progress table read pauses across a join and resumes after. Dropped
     * entirely if the network isn't RUNNING. */
    if (s_lqi_query_start || s_lqi_page_pending)
    {
        if (state != ZB_STATE_RUNNING)
        {
            s_lqi_query_start = false;
            s_lqi_page_pending = false;
        }
        else if (!zb_nwksrv_is_commissioning())
        {
            if (s_lqi_query_start)
            {
                /* A fresh query supersedes any half-finished paging sequence. */
                s_lqi_query_start = false;
                s_lqi_page_pending = false;
                if (zb_zdo_send_mgmt_lqi_req(0, 0) != ZB_OK)
                {
                    ZB_LOGW(TAG, "Mgmt_Lqi query request failed");
                }
            }
            else
            {
                s_lqi_page_pending = false;
                if (zb_zdo_send_mgmt_lqi_req(s_lqi_page_dst, s_lqi_page_next_index) != ZB_OK)
                {
                    ZB_LOGW(TAG, "Mgmt_Lqi page request (dst=%04X, idx=%u) failed",
                            s_lqi_page_dst, (unsigned)s_lqi_page_next_index);
                }
            }
        }
        /* else: commissioning in progress - defer (flags stay set). */
    }

    switch (state)
    {
        case ZB_STATE_IDLE:
        {
            if (entering)
            {
                ZB_LOGI(TAG, "Zigbee State Idle");
                /* Retry immediately on first entry. */
                s_idle_retry_not_before = 0;
            }

            /* Back-off so we don't hammer zb_znp_set_mode_znp() (which
             * blocks for up to 5 s when the ZNP is unresponsive) once
             * every 10 ms.  After a failure we stay in IDLE and retry
             * after ZB_IDLE_RETRY_BACKOFF_MS. */
            if (zb_os_now_ms() < s_idle_retry_not_before)
            {
                break;
            }

            /* The coprocessor announced a reset and nothing has configured it
             * since, so it is already in the post-boot state this branch would
             * pulse RESET to reach. Skip the redundant reset: it would cost
             * another full coprocessor boot and wipe the RS485 line config
             * again. Covers both the unsolicited-reset recovery above and the
             * stop -> start cycle (STOPPING already reset the ZNP). */
            if (s_znp_fresh)
            {
                ZB_LOGI(TAG, "ZNP already freshly reset; skipping redundant reset");
                g_zb_state = ZB_STATE_INIT;
                next_state_set = true;
                break;
            }

            if (zb_core_znp_reset_to_znp_mode(ZB_ZNP_RESET_TIMEOUT_MS) != ZB_OK)
            {
                ZB_LOGE(TAG, "ZNP Set Mode ZNP failed; backing off %d ms",
                        ZB_IDLE_RETRY_BACKOFF_MS);
                s_idle_retry_not_before =
                    zb_os_now_ms() + ZB_IDLE_RETRY_BACKOFF_MS;
                break;
            }
            g_zb_state = ZB_STATE_INIT;
            next_state_set = true;
            break;
        }

        case ZB_STATE_INIT:
        {
            if (entering)
            {
                ZB_LOGI(TAG, "Zigbee State Init");
                zb_core_set_network_state(ZB_NETWORK_STATE_INIT);
            }

            g_dev_state = ZNP_ZDO_STATE_HOLD;
            if (g_zb_config.is_factory_reset)
            {
                g_zb_config.form_new_network = true;
                g_zb_config.is_factory_reset = false;
            }
            else
            {
                g_zb_config.form_new_network =
                    (zb_has_form_network(&g_zb_config.channel_mask) == ZB_OK) ? false : true;
            }

            int status = zb_start_network(&g_zb_config);

            /* Configuration has now been pushed into the coprocessor (including
             * the factory-reset path, which does its own SYS_RESET_REQ midway to
             * make the NV startup-option clear take effect). Either way the chip
             * is no longer in its pristine post-boot state, so a later drop to
             * IDLE must reset it for real. Clearing here rather than on entry to
             * INIT matters: zb_start_network()'s own reset re-arms s_znp_fresh,
             * and leaving it set would let a subsequent IDLE skip a reset the
             * configured, network-running chip actually needs. */
            s_znp_fresh = false;

            if (status != ZB_OK)
            {
                ZB_LOGE(TAG, "Network Error: status=%d", status);
                g_zb_state = ZB_STATE_IDLE;
                next_state_set = true;
                /* Throttle the next IDLE retry so we don't loop hot. */
                s_idle_retry_not_before =
                    zb_os_now_ms() + ZB_IDLE_RETRY_BACKOFF_MS;
                break;
            }
            ZB_LOGI(TAG, "Network up");

            zb_core_zcl_application_init();

            /* Cache fresh coordinator info into g_zb_config. */
            s_zb_coordinator_info_t coord_info = {};
            zb_zdo_get_coordinator_info(&coord_info);
            ZB_LOGI(TAG, "ExtAddr: %016llX", coord_info.ieee_addr);
            g_zb_config.ieee_addr    = coord_info.ieee_addr;
            g_zb_config.channel      = coord_info.channel;
            g_zb_config.pan_id       = coord_info.pan_id;

            g_zb_state = ZB_STATE_RUNNING;
            next_state_set = true;
            break;
        }

        case ZB_STATE_RUNNING:
            if (entering)
            {
                ZB_LOGI(TAG, "Zigbee State Running");
                /* Bring-up is complete: no further reset is coming, so the
                 * coprocessor is usable again by the other consumers of its
                 * peripherals (RS485/Modbus). */
                zb_core_znp_link_up();
                zb_core_save_network_config();
                /* Pull current state from every persisted device now the
                 * network is up (staggered; see zb_core_query_device_task). */
                zb_core_sync_arm();
                /* Network is up and the coordinator just answered through
                 * forming, so it is READY. Setting either state re-publishes
                 * the combined ZB_EVENT_NETWORK_INFO. (re)arm the heartbeat. */
                zb_core_set_coordinator_state(ZB_COORDINATOR_STATE_READY);
                zb_core_set_network_state(ZB_NETWORK_STATE_RUN);
                s_health_ping_fails = 0;
                s_health_next_ping  = zb_os_now_ms() + ZB_HEALTH_PING_INTERVAL_MS;

                /* Optional: open permit-join for 180 s after a fresh
                 * form.  Off by default in production; kept on by
                 * default for compat with the previous behaviour. */
                if (s_auto_permit_join_on_form)
                {
                    zb_zdo_permit_join(180);
                }
            }
            zb_zcl_message_handling();
            zb_nwksrv_task();
            zb_core_query_device_task();
            zb_core_health_monitor();
            /* Age out devices that have gone quiet. Nothing else ever writes
             * ZB_DEVICE_STATUS_OFFLINE, so without this a device that leaves
             * the network stays "online" until the hub reboots. */
            zb_device_manager_availability_tick(zb_os_now_ms());
            /* Persist last-known device state when it has changed and the
             * interval has elapsed. Cheap when clean: one flag test. */
            zb_device_state_cache_tick(zb_os_now_ms());
            break;

        case ZB_STATE_BOOTLOADER_ENTERING:
        {
            if (entering)
            {
                ZB_LOGI(TAG, "Zigbee State Bootloader Entering");
                zb_core_set_network_state(ZB_NETWORK_STATE_STOP);
                zb_core_set_coordinator_state(ZB_COORDINATOR_STATE_FW_UPDATE);
            }
            if (zb_znp_set_mode_sbl(2000) != ZB_OK)
            {
                ZB_LOGE(TAG, "Zigbee Set Mode SBL failed");
                s_zb_event_t failed = {0};
                failed.type = ZB_EVENT_COORDINATOR_BOOTLOADER_ENTER_FAILED;
                failed.ieee_addr = g_zb_config.ieee_addr;
                zb_core_publish_event(&failed);
            }
            else
            {
                ZB_LOGI(TAG, "Zigbee Set Mode SBL success");
                s_zb_event_t success = {0};
                success.type = ZB_EVENT_COORDINATOR_BOOTLOADER_ENTER_SUCCESS;
                success.ieee_addr = g_zb_config.ieee_addr;
                zb_core_publish_event(&success);
            }
            g_zb_state = ZB_STATE_IDLE;
            next_state_set = true;
            break;
        }

        case ZB_STATE_ZNP_FIRMWARE_UPDATE:
        {
            bool fw_ok = false;
            if (entering)
            {
                ZB_LOGI(TAG, "Zigbee State ZNP Firmware Update");
                /* Coordinator is going down for flashing. */
                zb_core_set_network_state(ZB_NETWORK_STATE_STOP);
                zb_core_set_coordinator_state(ZB_COORDINATOR_STATE_FW_UPDATE);
                s_zb_event_t started = {0};
                /* The coprocessor is about to be flashed and the hub will
                 * reset afterwards - save now rather than lose up to a whole
                 * flush interval of readings. */
                zb_device_state_cache_flush();
                started.type      = ZB_EVENT_COORDINATOR_FW_UPDATE_STARTED;
                started.ieee_addr = g_zb_config.ieee_addr;
                zb_core_publish_event(&started);
            }

            if (!g_znp_fw_bin_file)
            {
                ZB_LOGE(TAG, "ZNP FW bin file path not set");
            }
            else if (zb_znp_set_mode_sbl(2000) != ZB_OK)
            {
                ZB_LOGE(TAG, "Zigbee Set Mode SBL failed");
            }
            else if (zb_znp_sbl_upgrade_firmware(g_znp_fw_bin_file, true,
                                                  zb_core_fw_update_progress_cb,
                                                  NULL) != ZB_OK)
            {
                ZB_LOGE(TAG, "Zigbee CC26XX Firmware Update failed");
            }
            else
            {
                fw_ok = true;
            }

            s_zb_event_t done = {0};
            done.type      = fw_ok ? ZB_EVENT_COORDINATOR_FW_UPDATE_COMPLETED
                                   : ZB_EVENT_COORDINATOR_FW_UPDATE_FAILED;
            done.ieee_addr = g_zb_config.ieee_addr;
            zb_core_publish_event(&done);

            g_zb_state = ZB_STATE_IDLE;
            next_state_set = true;
            break;
        }

        case ZB_STATE_STOPPING:
            if (entering)
            {
                ZB_LOGI(TAG, "Zigbee State Stopping");
                /* Operator-requested stop: the network goes down but the ZNP
                 * module is still reachable, so coordinator_state stays READY;
                 * network_state -> STOP is published from ZB_STATE_STOPPED. */
                zb_core_bind_q_reset();
            }
            zb_core_znp_reset_to_znp_mode(ZB_ZNP_RESET_TIMEOUT_MS);
            g_zb_state = ZB_STATE_STOPPED;
            next_state_set = true;
            break;

        case ZB_STATE_STOPPED:
            if (entering)
            {
                ZB_LOGI(TAG, "Zigbee State Stopped");
                /* The Zigbee network is down but the coprocessor itself is up
                 * and answering, so its RS485 peripheral is usable again. */
                zb_core_znp_link_up();
                zb_core_set_network_state(ZB_NETWORK_STATE_STOP);
            }
            break;

        default:
            ZB_LOGE(TAG, "Unknown state: %d", (int)state);
            break;
    }

    /* Track the state we ran THIS iteration; if a case re-assigned
     * g_zb_state we still record the at-entry value so the next tick
     * sees `entering=true` for the new state. */
    s_prev_state       = state;
    s_prev_state_valid = true;
    (void)next_state_set;
}

zb_status_t
zb_core_send_event(s_zb_event_t *event)
{
    if (zb_os_queue_send(zb_event_queue, event, 10))
    {
        return ZB_SUCCESS;
    }
    ZB_LOGE(TAG, "Failed to send event to queue");
    return ZB_FAIL;
}

/* ====================================================================== *
 *  Upper-layer event callback hook
 *  ----------------------------------------------------------------------
 *  zb_core stores a single callback that the iotdev_zigbee facade
 *  registers on init.  Driver-layer code calls zb_core_publish_event()
 *  at the points where network / lifecycle events naturally happen, and
 *  the facade fans them out to its subscribers.  Sensor events go
 *  through zb_device_manager_notify_event() and reach the facade via
 *  zb_device_manager_register_event_notify_callback().
 * ====================================================================== */
static zb_core_event_cb_t s_zb_core_event_cb = NULL;

void
zb_core_set_event_callback(zb_core_event_cb_t cb)
{
    s_zb_core_event_cb = cb;
}

void
zb_core_set_rs485_rx_callback(zb_core_rs485_rx_cb_t cb)
{
    s_rs485_rx_cb = cb;
}

void
zb_core_set_znp_link_callbacks(zb_core_znp_link_cb_t on_down, zb_core_znp_link_cb_t on_up)
{
    s_znp_link_down_cb = on_down;
    s_znp_link_up_cb   = on_up;
}

void
zb_core_publish_event(const s_zb_event_t *event)
{
    zb_core_event_cb_t cb = s_zb_core_event_cb;
    if (cb != NULL && event != NULL)
    {
        cb(event);
    }
}

/* ====================================================================== *
 *  Network-level command entry points
 *  ----------------------------------------------------------------------
 *  Invoked by the iotdev_zigbee façade from the same OS task that runs
 *  zb_core_task(); see zb_core.h for the contract.
 * ====================================================================== */

zb_status_t
zb_core_apply_network_config(const s_zb_network_config_t *cfg, e_zb_cmd_type_t cmd_type)
{
    if (cfg == NULL)
    {
        return ZB_FAIL;
    }

    if (cfg->fields & ZB_NETWORK_CONFIG_F_TX_POWER)
    {
        if (cfg->tx_power != g_zb_config.tx_power)
        {
            if (zb_set_tx_power(cfg->tx_power) != ZB_OK)
            {
                ZB_LOGE(TAG, "Apply network config: set_tx_power failed");
                return ZB_FAIL;
            }
            ZB_LOGI(TAG, "Apply network config: tx_power changed from %d to %d",
                g_zb_config.tx_power, cfg->tx_power);
            g_zb_config.tx_power = cfg->tx_power;
            s_zb_event_t evt = {0};
            evt.type = ZB_EVENT_NETWORK_TX_POWER_CHANGED;
            evt.ieee_addr = g_zb_config.ieee_addr;
            evt.network_info.tx_power = cfg->tx_power;
            zb_core_publish_event(&evt);
        }
    }

    if (cfg->fields & ZB_NETWORK_CONFIG_F_CHANNEL_MASK)
    {
        if ((cfg->channel_mask & ZB_ALL_CHANNEL_MASK) != (1 << g_zb_config.channel)
            || (cfg->channel_mask & ZB_ALL_CHANNEL_MASK) != g_zb_config.channel_mask)
        {
            const bool network_was_up = zb_core_get_running_status();

            ZB_LOGI(TAG, "Apply network config: channel mask changed from 0x%08X to 0x%08X",
                g_zb_config.channel_mask, cfg->channel_mask);

            g_zb_config.channel_mask = cfg->channel_mask & ZB_ALL_CHANNEL_MASK;
            g_zb_config.is_factory_reset = true;
            g_zb_state = ZB_STATE_IDLE;   /* set before publish so state reads STOP */
            if (network_was_up)
            {
                zb_core_set_network_state(ZB_NETWORK_STATE_STOP);
            }
            s_idle_retry_not_before = 0;
            s_zb_event_t evt = {0};
            evt.type = ZB_EVENT_NETWORK_CHANNEL_MASK_CHANGED;
            evt.ieee_addr = g_zb_config.ieee_addr;
            evt.network_info.channel_mask = g_zb_config.channel_mask;
            zb_core_publish_event(&evt);
        }
    }
    return ZB_OK;
}

zb_status_t
zb_core_apply_default_network_config(uint8_t channel, uint32_t channel_mask, int8_t tx_power)
{
    channel_mask &= ZB_ALL_CHANNEL_MASK;
    if (channel_mask == 0)
    {
        if (channel < 11 || channel > 26)
        {
            ZB_LOGE(TAG, "Default network config: invalid channel %u with empty mask", channel);
            return ZB_FAIL;
        }
        channel_mask = (1u << channel);
    }

    g_zb_config.channel = channel;
    g_zb_config.channel_mask = channel_mask;
    g_zb_config.tx_power = tx_power;

    ZB_LOGI(TAG, "Default network config applied: channel=%u, mask=0x%08X, tx_power=%d",
        channel, channel_mask, tx_power);
    return ZB_OK;
}

zb_status_t
zb_core_save_network_config(void)
{
    bool dirty = false;

    if (!iotdev_compare_config_int(CFG_ZIGBEE_CHANNEL, g_zb_config.channel))
    {
        if (iotdev_config_set_int(CFG_ZIGBEE_CHANNEL, g_zb_config.channel) < 0)
        {
            ZB_LOGE(TAG, "Save network config: set channel failed");
            return ZB_FAIL;
        }
        dirty = true;
    }
    if (!iotdev_compare_config_int(CFG_ZIGBEE_CHANNEL_MASK, g_zb_config.channel_mask))
    {
        if (iotdev_config_set_int(CFG_ZIGBEE_CHANNEL_MASK, g_zb_config.channel_mask) < 0)
        {
            ZB_LOGE(TAG, "Save network config: set channel mask failed");
            return ZB_FAIL;
        }
        dirty = true;
    }
    if (!iotdev_compare_config_int(CFG_ZIGBEE_TX_POWER, (uint32_t)g_zb_config.tx_power))
    {
        if (iotdev_config_set_int(CFG_ZIGBEE_TX_POWER, g_zb_config.tx_power) < 0)
        {
            ZB_LOGE(TAG, "Save network config: set tx power failed");
            return ZB_FAIL;
        }
        dirty = true;
    }

    if (!dirty)
    {
        ZB_LOGD(TAG, "Save network config: unchanged, nothing to write");
        return ZB_OK;
    }

    if (!iotdev_config_save())
    {
        ZB_LOGE(TAG, "Save network config: flash write failed");
        return ZB_FAIL;
    }

    ZB_LOGI(TAG, "Network config saved: channel=%u, mask=0x%08X, tx_power=%d",
        g_zb_config.channel, g_zb_config.channel_mask, g_zb_config.tx_power);
    return ZB_OK;
}

zb_status_t
zb_core_request_znp_fw_update(const char *fw_bin_file)
{
    /* Only allowed from idle / running, never mid-init or mid-stop. */
    if (g_zb_state != ZB_STATE_IDLE && g_zb_state != ZB_STATE_RUNNING)
    {
        ZB_LOGW(TAG, "ZNP FW update rejected: state=%d", (int)g_zb_state);
        return ZB_FAIL;
    }
    if (g_znp_fw_bin_file)
    {
        free(g_znp_fw_bin_file);
    }
    g_znp_fw_bin_file = ZB_MEM_CALLOC(1, strlen(fw_bin_file) + 1);
    if (!g_znp_fw_bin_file)
    {
        ZB_LOGE(TAG, "Failed to allocate memory for ZNP FW bin file path");
        return ZB_FAIL;
    }
    strcpy(g_znp_fw_bin_file, fw_bin_file);
    g_zb_state = ZB_STATE_ZNP_FIRMWARE_UPDATE;
    return ZB_OK;
}

zb_status_t
zb_core_request_znp_bootloader_enter(void)
{
    g_zb_state = ZB_STATE_BOOTLOADER_ENTERING;
    return ZB_OK;
}

zb_status_t
zb_core_request_start(void)
{
    if (g_zb_state != ZB_STATE_STOPPED && g_zb_state != ZB_STATE_STOPPING)
    {
        return ZB_OK;
    }
    g_zb_state = ZB_STATE_IDLE;
    return ZB_OK;
}

zb_status_t
zb_core_request_stop(void)
{
    if (g_zb_state == ZB_STATE_STOPPED || g_zb_state == ZB_STATE_STOPPING)
    {
        return ZB_OK;
    }
    g_zb_state = ZB_STATE_STOPPING;
    return ZB_OK;
}

zb_status_t
zb_core_request_factory_reset(void)
{
    ZB_LOGI(TAG, "Factory reset requested");
    g_zb_config.is_factory_reset = true;
    g_zb_config.form_new_network = true;

    if (g_zb_state == ZB_STATE_RUNNING || g_zb_state == ZB_STATE_INIT)
    {
        g_zb_state = ZB_STATE_IDLE;   /* set before publish so state reads STOP */
        zb_core_set_network_state(ZB_NETWORK_STATE_STOP);
        s_idle_retry_not_before = 0;
    }
    else if (g_zb_state == ZB_STATE_STOPPED)
    {
        g_zb_state = ZB_STATE_IDLE;
        s_idle_retry_not_before = 0;
    }
    else if (g_zb_state == ZB_STATE_IDLE)
    {
        s_idle_retry_not_before = 0;
    }

    return ZB_OK;
}

zb_status_t
zb_core_request_lqi_query(void)
{
    /* Just arm the request; zb_core_task() issues it on its next tick, when it
     * is running and no interview is in progress. Safe to call from any task. */
    s_lqi_query_start = true;
    return ZB_OK;
}

void
zb_core_set_auto_permit_join_on_form(bool enable)
{
    s_auto_permit_join_on_form = enable;
    ZB_LOGI(TAG, "Auto permit-join on form: %s", enable ? "ON" : "OFF");
}

void
zb_core_set_diag_read_handler(zb_core_diag_read_cb_t cb, void *ctx)
{
    s_diag_read_cb = cb;
    s_diag_read_ctx = ctx;
}

zb_status_t
zb_core_diag_read_attributes(uint16_t nwk_addr, uint8_t endpoint, uint16_t cluster_id,
                             uint16_t manuf_code, const uint16_t *attr_ids, uint8_t attr_count)
{
    if ((attr_ids == NULL) || (attr_count == 0) || (attr_count > ZB_MAX_ATTRS))
    {
        return ZB_FAIL;
    }

    uint8_t buf[sizeof(s_zb_zcl_read_attr_cmd_t) + ZB_MAX_ATTRS * sizeof(uint16_t)] = {0};
    s_zb_zcl_read_attr_cmd_t *cmd = (s_zb_zcl_read_attr_cmd_t *)&buf[0];

    cmd->num_attr = attr_count;
    for (uint8_t i = 0; i < attr_count; i++)
    {
        cmd->attr_id[i] = attr_ids[i];
    }

    s_zb_af_address_t addr = {
        .short_addr = nwk_addr,
        .address_mode = AF_ADDRESS_16BIT,
        .endpoint = endpoint,
    };
    return zb_zcl_send_read_manu(ZB_HUB_ENDPOINT, &addr, cluster_id, cmd,
                                 ZCL_FRAME_CLIENT_SERVER_DIR, true, manuf_code,
                                 zb_zcl_next_seq_num());
}

zb_status_t
zb_core_diag_read_reporting_config(uint16_t nwk_addr, uint8_t endpoint, uint16_t cluster_id,
                                   const uint16_t *attr_ids, uint8_t attr_count)
{
    if ((attr_ids == NULL) || (attr_count == 0) || (attr_count > ZB_MAX_ATTRS))
    {
        return ZB_FAIL;
    }

    uint8_t buf[sizeof(s_zb_zcl_read_report_cfg_cmd_t) +
                ZB_MAX_ATTRS * sizeof(s_zb_zcl_read_report_cfg_info_t)] = {0};
    s_zb_zcl_read_report_cfg_cmd_t *cmd = (s_zb_zcl_read_report_cfg_cmd_t *)&buf[0];

    cmd->num_attr = attr_count;
    for (uint8_t i = 0; i < attr_count; i++)
    {
        /* 0x00 = "the reports this device sends us", which is the direction
         * every Configure Reporting in this driver uses. */
        cmd->attr_list[i].direction = 0;
        cmd->attr_list[i].attr_id = attr_ids[i];
    }

    s_zb_af_address_t addr = {
        .short_addr = nwk_addr,
        .address_mode = AF_ADDRESS_16BIT,
        .endpoint = endpoint,
    };
    return zb_zcl_send_read_report_cfg_cmd(ZB_HUB_ENDPOINT, &addr, cluster_id, cmd,
                                           ZCL_FRAME_CLIENT_SERVER_DIR, true,
                                           zb_zcl_next_seq_num());
}

void
zb_core_publish_network_info(void)
{
    s_zb_event_t evt = {0};
    evt.type             = ZB_EVENT_NETWORK_INFO;
    evt.ieee_addr        = g_zb_config.ieee_addr;
    evt.network_info.ieee_addr = g_zb_config.ieee_addr;
    evt.network_info.channel  = g_zb_config.channel;
    evt.network_info.channel_mask = g_zb_config.channel_mask;
    evt.network_info.pan_id   = g_zb_config.pan_id;
    evt.network_info.tx_power = g_zb_config.tx_power;
    evt.network_info.version  = s_znp_fw_version;

    /* Two orthogonal dimensions: Zigbee network formation (from g_zb_state) and
     * ZNP link state. When the coordinator is OFFLINE the network fields are
     * last-known/stale — coordinator_state is the flag that says whether to
     * trust them. */
    switch (g_zb_state)
    {
        case ZB_STATE_INIT:    evt.network_info.state = ZB_NETWORK_STATE_INIT; break;
        case ZB_STATE_RUNNING: evt.network_info.state = ZB_NETWORK_STATE_RUN;  break;
        default:               evt.network_info.state = ZB_NETWORK_STATE_STOP; break;
    }
    evt.network_info.coordinator_state = (uint8_t)s_coordinator_state;
    zb_core_publish_event(&evt);
}

zb_status_t
zb_core_get_coordinator_info(s_zb_coordinator_info_t *info)
{
    if (info == NULL)
    {
        return ZB_FAIL;
    }

    uint8_t net_state;
    switch (g_zb_state)
    {
        case ZB_STATE_INIT:
            net_state = ZB_NETWORK_STATE_INIT;
            break;
        case ZB_STATE_RUNNING:
            net_state = ZB_NETWORK_STATE_RUN;
            break;
        default:
            net_state = ZB_NETWORK_STATE_STOP;
            break;
    }

    zb_core_fill_coord_info(info, net_state);
    if (info->ieee_addr == 0)
    {
        return ZB_FAIL;
    }
    return ZB_OK;
}
 