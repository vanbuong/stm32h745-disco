#include "device/zb_device_manager.h"

#include "common/zb_common.h"
#include "device/zb_device_db.h"
#include "device/zb_device.h"
#include "device/zb_device_schema.h"
#include "device/zb_device_state_cache.h"
#include "device/generic/zb_device_battery_sensor.h"
#include "device/generic/zb_device_button.h"
#include "device/generic/zb_device_dimmer_switch.h"
#include "device/generic/zb_device_mains_power_outlet.h"
#include "device/generic/zb_device_on_off_smart_plug.h"
#include "device/generic/zb_device_on_off_switch.h"
#include "device/generic/zb_device_relay.h"
#include "device/intruder/zb_device_ias_warning.h"
#include "device/intruder/zb_device_ias_zone.h"
#include "device/lighting/zb_device_color_light.h"
#include "device/lighting/zb_device_color_temp_light.h"
#include "device/lighting/zb_device_dimmable_light.h"
#include "device/lighting/zb_device_extended_color_light.h"
#include "device/lighting/zb_device_on_off_light.h"
#include "device/measurement/zb_device_co2_sensor.h"
#include "device/measurement/zb_device_develco_voc_sensor.h"
#include "device/measurement/zb_device_eco2_sensor.h"
#include "device/measurement/zb_device_electrical_measurement.h"
#include "device/measurement/zb_device_energy_meter.h"
#include "device/measurement/zb_device_flow_sensor.h"
#include "device/measurement/zb_device_formaldehyde_sensor.h"
#include "device/measurement/zb_device_humidity_sensor.h"
#include "device/measurement/zb_device_iaq_sensor.h"
#include "device/measurement/zb_device_illuminance_sensor.h"
#include "device/measurement/zb_device_occupancy_sensor.h"
#include "device/measurement/zb_device_pm10_sensor.h"
#include "device/measurement/zb_device_pm1_sensor.h"
#include "device/measurement/zb_device_pm25_sensor.h"
#include "device/measurement/zb_device_pressure_sensor.h"
#include "device/measurement/zb_device_temperature_sensor.h"
#include "device/measurement/zb_device_tvoc_sensor.h"
#include "osal/zb_osal.h"
#include "zdo/zb_zdo.h"
#include "zcl/zb_zcl.h"
#include "common/zb_sensor_units.h"
#include "core/zb_core.h"

#include <stddef.h>
#include <dirent.h>
#include <sys/stat.h>

#define TAG "ZB_DEV_MGR"

static bool desired_config_match(const s_zb_desired_config_t *a, const s_zb_desired_config_t *b);

static s_zb_device_manager_t s_device_manager;
static zb_event_notify_callback_t s_event_notify_callback = NULL;
static zb_os_mutex_t s_device_manager_mutex;

/* Availability bookkeeping per pool slot; see the Availability section below.
 * Deliberately not fields of s_zb_device_t - this is pure runtime state and
 * that struct is persisted. */
typedef struct s_zb_avail_slot
{
    uint32_t next_check_ms;     /* monotonic; before this, skip the device    */
    uint8_t  backoff_step;      /* index into s_avail_backoff_eighths         */
    uint8_t  ping_pending;      /* a ping is out, waiting for the grace window*/
    uint8_t  paused;            /* backoff passed the cutoff; wait for a frame*/
} s_zb_avail_slot_t;

static s_zb_avail_slot_t s_avail[ZB_MAX_DEVICE];
static void availability_slot_reset(uint8_t pool_idx);

/* When non-NULL, zb_device_manager_notify_event() captures the event here (and
 * sets *s_capture_hit) instead of publishing it. Used by the array-form state
 * snapshot to harvest each function's cached event without emitting it. Only
 * ever touched on the driver task under the (recursive) device-manager mutex. */
static s_zb_event_t *s_capture_slot = NULL;
static bool          s_capture_hit  = false;

static void
device_manager_lock(void)
{
    if (s_device_manager_mutex != NULL)
    {
        zb_os_mutex_lock(s_device_manager_mutex, ZB_OSAL_WAIT_FOREVER);
    }
}

static void
device_manager_unlock(void)
{
    if (s_device_manager_mutex != NULL)
    {
        zb_os_mutex_unlock(s_device_manager_mutex);
    }
}

/******************************************************************************
 * IEEE Hash Table
 ******************************************************************************/

static uint32_t
ieee_hash(const uint64_t ieee_addr)
{
    uint32_t h = 2166136261u;
    uint8_t *p = (uint8_t *)&ieee_addr;
    for (int i = 0; i < 8; i++)
    {
        h ^= p[i];
        h *= 16777619u;
    }
    return h;
}

static void
ieee_hash_table_set(const uint64_t ieee_addr, s_zb_device_t *device)
{
    uint32_t idx = ieee_hash(ieee_addr) & (ZB_IEEE_HASH_SIZE - 1);
    for (int i = 0; i < ZB_IEEE_HASH_SIZE; i++)
    {
        uint32_t slot = (idx + i) & (ZB_IEEE_HASH_SIZE - 1);
        if (!s_device_manager.ieee_index.entries[slot].used ||
            s_device_manager.ieee_index.entries[slot].ieee_addr == ieee_addr)
        {
            s_device_manager.ieee_index.entries[slot].ieee_addr = ieee_addr;
            s_device_manager.ieee_index.entries[slot].device = device;
            s_device_manager.ieee_index.entries[slot].used = (device != NULL);
            return;
        }
    }
    ZB_LOGE(TAG, "IEEE hash table full!");
}

static s_zb_device_t *
ieee_hash_table_get(const uint64_t ieee_addr)
{
    uint32_t idx = ieee_hash(ieee_addr) & (ZB_IEEE_HASH_SIZE - 1);
    ZB_LOGD(TAG, "Looking up IEEE address 0x%llx in hash table (idx: %d)", ieee_addr, idx);
    for (int i = 0; i < ZB_IEEE_HASH_SIZE; i++)
    {
        uint32_t slot = (idx + i) & (ZB_IEEE_HASH_SIZE - 1);
        if (!s_device_manager.ieee_index.entries[slot].used)
            return NULL;

        if (s_device_manager.ieee_index.entries[slot].ieee_addr == ieee_addr)
        {
            ZB_LOGD(TAG, "Found IEEE address 0x%llx in hash table (slot: %d)", ieee_addr, slot);
            return s_device_manager.ieee_index.entries[slot].device;
        }
    }
    return NULL;
}

static void
ieee_hash_table_remove(const uint64_t ieee_addr)
{
    uint32_t idx = ieee_hash(ieee_addr) & (ZB_IEEE_HASH_SIZE - 1);
    for (int i = 0; i < ZB_IEEE_HASH_SIZE; i++)
    {
        uint32_t slot = (idx + i) & (ZB_IEEE_HASH_SIZE - 1);
        if (!s_device_manager.ieee_index.entries[slot].used)
            return;
        if (s_device_manager.ieee_index.entries[slot].ieee_addr == ieee_addr)
        {
            s_device_manager.ieee_index.entries[slot].used = false;
            s_device_manager.ieee_index.entries[slot].device = NULL;
            return;
        }
    }
}

/******************************************************************************
 * Event notification helper
 ******************************************************************************/

/* Copy a device's identity, capabilities and logical-function list into the
 * shared joined-info payload. Used both for the DEVICE_JOINED event and for
 * each DEVICE_LIST_ITEM in a list snapshot. */
static void
device_manager_fill_joined_info(const s_zb_device_t *device, s_zb_device_joined_info_t *info)
{
    memset(info, 0, sizeof(*info));
    strncpy(info->manufacturer, device->manufacturer, sizeof(info->manufacturer));
    strncpy(info->model, device->model, sizeof(info->model));
    strncpy(info->date_code, device->date_code, sizeof(info->date_code));
    strncpy(info->product_label, device->product_label, sizeof(info->product_label));
    strncpy(info->serial_number, device->serial_number, sizeof(info->serial_number));
    strncpy(info->sw_build_id, device->sw_build_id, sizeof(info->sw_build_id));
    info->product_code_len = device->product_code_len <= sizeof(info->product_code)
                                 ? device->product_code_len : sizeof(info->product_code);
    memcpy(info->product_code, device->product_code, info->product_code_len);
    info->parent_ieee = device->parent_ieee;
    info->parent_nwk_addr = device->parent_nwk_addr;
    info->app_version = device->app_version;
    info->hw_version = device->hw_version;
    info->physical_environment = device->physical_environment;
    info->battery_powered = device->battery_powered;
    info->sleepy_enabled = device->sleepy_enabled;
    info->ota_supported = device->ota_supported;
    info->device_type = device->device_type;
    info->power_source = device->power_source;
    /* Publish only real sensor/actuator functions. ZB_FUNC_BASIC_INFO is device
     * metadata (manufacturer/model/hw_version/date_code/…), already surfaced at
     * the top level of this struct, so it is omitted from the function list. */
    uint8_t out = 0;
    for (int i = 0; i < device->function_count && out < ZB_MAX_FUNCTIONS; i++)
    {
        if (device->functions[i].type == ZB_FUNC_BASIC_INFO)
        {
            continue;
        }
        info->functions[out].sensor_id = device->functions[i].sensor_id;
        info->functions[out].type = device->functions[i].type;
        strncpy(info->functions[out].name, device->functions[i].name, sizeof(info->functions[out].name));
        out++;
    }
    info->function_count = out;
}

static void
device_manager_notify_device_event(s_zb_device_t *device, e_zb_event_type_t event_type)
{
    ZB_LOGI(TAG, "0x%llx - %s() - Event type: %d", device->ieee_addr, __func__, event_type);
    s_zb_event_t event;
    event.type = event_type;
    event.ieee_addr = device->ieee_addr;

    if (event_type == ZB_EVENT_DEVICE_JOINED)
    {
        device_manager_fill_joined_info(device, &event.device_info);
    }
    if (s_event_notify_callback)
        s_event_notify_callback(&event);
}

void
zb_device_manager_publish_device_list(uint32_t txn_id)
{
    device_manager_lock();
    if (!s_event_notify_callback)
    {
        ZB_LOGW(TAG, "%s() - no event callback registered, dropping txn=%lu",
                __func__, (unsigned long)txn_id);
        device_manager_unlock();
        return;
    }

    /*
     * Runs on the driver task with no yields, so device_count and the pool
     * cannot change underneath us — the burst is an atomic snapshot.
     */
    const uint16_t total = s_device_manager.device_count;
    s_zb_event_t ev;

    memset(&ev, 0, sizeof(ev));
    ev.type = ZB_EVENT_DEVICE_LIST_BEGIN;
    ev.device_list_begin.txn_id = txn_id;
    ev.device_list_begin.total = total;
    s_event_notify_callback(&ev);

    uint16_t emitted = 0;
    for (uint16_t i = 0; i < ZB_MAX_DEVICE; i++)
    {
        s_zb_device_t *device = s_device_manager.pool[i];
        if (!device)
            continue;

        memset(&ev, 0, sizeof(ev));
        ev.type = ZB_EVENT_DEVICE_LIST_ITEM;
        ev.ieee_addr = device->ieee_addr;
        ev.device_list_item.txn_id = txn_id;
        ev.device_list_item.index = emitted;
        ev.device_list_item.total = total;
        ev.device_list_item.nwk_addr = device->nwk_addr;
        ev.device_list_item.online = (device->status == ZB_DEVICE_STATUS_ONLINE);
        ev.device_list_item.last_seen = device->last_seen;
        ev.device_list_item.lqi = device->lqi;
        device_manager_fill_joined_info(device, &ev.device_list_item.info);
        s_event_notify_callback(&ev);
        emitted++;
    }

    memset(&ev, 0, sizeof(ev));
    ev.type = ZB_EVENT_DEVICE_LIST_END;
    ev.device_list_end.txn_id = txn_id;
    ev.device_list_end.count = emitted;
    ev.device_list_end.status = ZB_OK;
    s_event_notify_callback(&ev);

    ZB_LOGI(TAG, "%s() - txn=%lu published %u/%u devices",
            __func__, (unsigned long)txn_id, emitted, total);
    device_manager_unlock();
}

void
zb_device_manager_publish_device_state(uint64_t ieee_addr, uint32_t txn_id)
{
    device_manager_lock();
    if (!s_event_notify_callback)
    {
        ZB_LOGW(TAG, "%s() - no event callback registered, dropping txn=%lu",
                __func__, (unsigned long)txn_id);
        device_manager_unlock();
        return;
    }

    /*
     * Runs on the driver task with no yields, so the pool cannot change and no
     * spontaneous function event interleaves — every value event delivered
     * between BEGIN and END belongs to this snapshot. Each function re-emits its
     * cached value via ops->emit_state (no OTA read), so this works even when
     * the Zigbee network is down. The device-manager mutex is recursive, so the
     * emit_state -> zb_device_manager_notify_event re-lock is safe.
     */
    const uint16_t dev_total = (ieee_addr != 0)
                                   ? (ieee_hash_table_get(ieee_addr) ? 1u : 0u)
                                   : s_device_manager.device_count;

    s_zb_event_t ev;
    memset(&ev, 0, sizeof(ev));
    ev.type = ZB_EVENT_DEVICE_STATE_BEGIN;
    ev.ieee_addr = ieee_addr;
    ev.device_state_begin.txn_id = txn_id;
    ev.device_state_begin.device_count = dev_total;
    s_event_notify_callback(&ev);

    uint16_t emitted = 0;
    for (uint16_t i = 0; i < ZB_MAX_DEVICE; i++)
    {
        s_zb_device_t *device = s_device_manager.pool[i];
        if (!device)
            continue;
        if (ieee_addr != 0 && device->ieee_addr != ieee_addr)
            continue;
        for (uint8_t f = 0; f < device->function_count && f < ZB_MAX_FUNCTIONS; f++)
        {
            s_zb_function_t *func = &device->functions[f];
            if (func->ops && func->ops->emit_state)
            {
                func->ops->emit_state(device, func);
                emitted++;
            }
        }
    }

    memset(&ev, 0, sizeof(ev));
    ev.type = ZB_EVENT_DEVICE_STATE_END;
    ev.ieee_addr = ieee_addr;
    ev.device_state_end.txn_id = txn_id;
    ev.device_state_end.emitted = emitted;
    ev.device_state_end.status = ZB_OK;
    s_event_notify_callback(&ev);

    ZB_LOGI(TAG, "%s() - txn=%lu ieee=0x%llx emitted %u function state(s)",
            __func__, (unsigned long)txn_id, (unsigned long long)ieee_addr, emitted);
    device_manager_unlock();
}

/* Map a harvested per-function event onto a compact tagged value. Returns false
 * for events that carry no snapshot-worthy value. func supplies the identity
 * (sensor_id/type) the event itself doesn't carry. */
static bool
device_manager_event_to_func_value(const s_zb_event_t *ev, const s_zb_function_t *func,
                                   s_zb_func_value_t *out)
{
    memset(out, 0, sizeof(*out));
    out->sensor_id  = func->sensor_id;
    out->type       = func->type;
    out->value_type = ev->type;

    switch (ev->type)
    {
        case ZB_EVENT_SENSOR_TEMPERATURE:
        case ZB_EVENT_SENSOR_HUMIDITY:
        case ZB_EVENT_SENSOR_PRESSURE:
        case ZB_EVENT_SENSOR_ILLUMINANCE:
        case ZB_EVENT_SENSOR_FLOW:
        case ZB_EVENT_SENSOR_PM25:
        case ZB_EVENT_SENSOR_CO2:
        case ZB_EVENT_SENSOR_PM10:
        case ZB_EVENT_SENSOR_PM1:
        case ZB_EVENT_SENSOR_TVOC:
        case ZB_EVENT_SENSOR_FORMALDEHYDE:
        case ZB_EVENT_SENSOR_IAQ:
        case ZB_EVENT_SENSOR_ECO2:
            out->v.sensor.value = ev->sensor.value;
            out->v.sensor.unit  = ev->sensor.unit;
            return true;
        case ZB_EVENT_SENSOR_BATTERY:
            out->v.battery.percent = ev->battery.percent;
            out->v.battery.voltage = ev->battery.voltage;
            return true;
        case ZB_EVENT_SENSOR_OCCUPANCY:
        case ZB_EVENT_BINARY_STATE:
            out->v.binary.active = ev->binary.active;
            return true;
        case ZB_EVENT_SENSOR_ELECTRICAL_MEASUREMENT:
            out->v.electrical.voltage        = ev->electrical_measurement.voltage;
            out->v.electrical.current        = ev->electrical_measurement.current;
            out->v.electrical.power          = ev->electrical_measurement.power;
            out->v.electrical.frequency      = ev->electrical_measurement.frequency;
            out->v.electrical.reactive_power = ev->electrical_measurement.reactive_power;
            out->v.electrical.apparent_power = ev->electrical_measurement.apparent_power;
            out->v.electrical.power_factor   = ev->electrical_measurement.power_factor;
            return true;
        case ZB_EVENT_SENSOR_ENERGY_METERING:
            out->v.energy = ev->energy;
            return true;
        case ZB_EVENT_SENSOR_IAS_ZONE:
            out->v.ias.zone_type   = ev->ias.zone_type;
            out->v.ias.zone_status = ev->ias.zone_status;
            return true;
        case ZB_EVENT_DEVICE_IAS_WARNING:
            out->v.ias_warning.max_duration = ev->ias_warning.max_duration;
            return true;
        case ZB_EVENT_LIGHT_ONOFF_STATE:
            out->v.onoff.on = ev->onoff_light.on;
            return true;
        case ZB_EVENT_LIGHT_DIMMABLE_STATE:
            out->v.level.level = ev->dimmable_light.level;
            return true;
        case ZB_EVENT_LIGHT_COLOR_TEMP_STATE:
            out->v.color_temp.color_temp = ev->color_temp_light.color_temp;
            return true;
        case ZB_EVENT_LIGHT_COLOR_HUE_SAT_STATE:
            out->v.hue_sat.hue        = ev->color_light.hue;
            out->v.hue_sat.saturation = ev->color_light.saturation;
            return true;
        case ZB_EVENT_LIGHT_EXTENDED_COLOR_STATE:
            out->v.ext_color.hue        = ev->extended_color_light.hue;
            out->v.ext_color.saturation = ev->extended_color_light.saturation;
            out->v.ext_color.color_temp = ev->extended_color_light.color_temp;
            return true;
        case ZB_EVENT_SWITCH_ONOFF_STATE:
            out->v.onoff.on = ev->switch_onoff.on;
            return true;
        case ZB_EVENT_SWITCH_LEVEL_STATE:
            out->v.level.level = ev->switch_level.level;
            return true;
        case ZB_EVENT_BUTTON_ACTION:
            out->v.button.action = ev->button.action;
            return true;
        default:
            return false;
    }
}

/* Gather one device's cached function values into `out` by harvesting each
 * function's emit_state event (captured, not published). Returns the count. */
static uint8_t
device_manager_collect_device_values(s_zb_device_t *device, s_zb_func_value_t *out)
{
    uint8_t count = 0;
    for (uint8_t f = 0; f < device->function_count && f < ZB_MAX_FUNCTIONS; f++)
    {
        s_zb_function_t *func = &device->functions[f];
        if (!func->ops || !func->ops->emit_state)
            continue;

        s_zb_event_t captured;
        s_capture_slot = &captured;
        s_capture_hit  = false;
        func->ops->emit_state(device, func);   /* routed into `captured` */
        s_capture_slot = NULL;

        if (s_capture_hit && count < ZB_MAX_FUNCTIONS)
        {
            if (device_manager_event_to_func_value(&captured, func, &out[count]))
                count++;
        }
    }
    return count;
}

/* Read one function's cached state straight out of its context into the
 * driver's tagged value struct. No emit_state(), so nothing goes near the event
 * path - but the result has the same shape the event-derived collector
 * produced, so s_zb_func_value_t stays the single description of what a
 * function holds and callers keep every quantity, not just a headline one.
 *
 * Dispatch is on func->type: it is the vocabulary the rest of the system
 * speaks, and every type resolves to exactly one context layout. The one type
 * served by more than one implementation is ZB_FUNC_TVOC (standard TVOC
 * cluster / Develco VOC / Develco IEEE-754 VOC); those contexts differ in their
 * raw field but agree on `value`, which the assertions below pin down so a
 * later edit to any of them fails the build instead of misreading memory.
 *
 * Returns false for functions that carry no reportable value (basic info,
 * range extender), or whose value has not been observed yet (button). */
_Static_assert(offsetof(s_zb_device_tvoc_sensor_ctx_t, value) ==
               offsetof(s_zb_device_develco_voc_sensor_ctx_t, value),
               "TVOC contexts must agree on `value` - ZB_FUNC_TVOC is read by type");

static bool
device_manager_read_function_value(const s_zb_function_t *func, s_zb_func_value_t *out)
{
    const void *ctx = func->ctx;

    if (func->ops == NULL || ctx == NULL)
    {
        return false;
    }

    memset(out, 0, sizeof(*out));
    out->sensor_id = func->sensor_id;
    out->type      = func->type;

    switch (func->type)
    {
        /* --- measurement: scaled float already sitting in the context -------
         * These all ride in the `sensor` member; the unit is filled in at the
         * end from the event type the reading would be published under. */
        case ZB_FUNC_TEMPERATURE:
            out->value_type     = ZB_EVENT_SENSOR_TEMPERATURE;
            out->v.sensor.value = ((const s_zb_device_temperature_sensor_ctx_t *)ctx)->value;
            break;
        case ZB_FUNC_HUMIDITY:
            out->value_type     = ZB_EVENT_SENSOR_HUMIDITY;
            out->v.sensor.value = ((const s_zb_device_humidity_sensor_ctx_t *)ctx)->value;
            break;
        case ZB_FUNC_PRESSURE:
            out->value_type     = ZB_EVENT_SENSOR_PRESSURE;
            out->v.sensor.value = ((const s_zb_device_pressure_sensor_ctx_t *)ctx)->value;
            break;
        case ZB_FUNC_FLOW:
            out->value_type     = ZB_EVENT_SENSOR_FLOW;
            out->v.sensor.value = ((const s_zb_device_flow_sensor_ctx_t *)ctx)->value;
            break;
        case ZB_FUNC_ILLUMINANCE:
            out->value_type     = ZB_EVENT_SENSOR_ILLUMINANCE;
            out->v.sensor.value = ((const s_zb_device_illuminance_sensor_ctx_t *)ctx)->lux;
            break;
        case ZB_FUNC_PM25:
            out->value_type     = ZB_EVENT_SENSOR_PM25;
            out->v.sensor.value = ((const s_zb_device_pm25_sensor_ctx_t *)ctx)->value;
            break;
        case ZB_FUNC_PM10:
            out->value_type     = ZB_EVENT_SENSOR_PM10;
            out->v.sensor.value = ((const s_zb_device_pm10_sensor_ctx_t *)ctx)->value;
            break;
        case ZB_FUNC_PM1:
            out->value_type     = ZB_EVENT_SENSOR_PM1;
            out->v.sensor.value = ((const s_zb_device_pm1_sensor_ctx_t *)ctx)->value;
            break;
        case ZB_FUNC_CO2:
            out->value_type     = ZB_EVENT_SENSOR_CO2;
            out->v.sensor.value = ((const s_zb_device_co2_sensor_ctx_t *)ctx)->value;
            break;
        case ZB_FUNC_ECO2:
            out->value_type     = ZB_EVENT_SENSOR_ECO2;
            out->v.sensor.value = ((const s_zb_device_eco2_sensor_ctx_t *)ctx)->value;
            break;
        case ZB_FUNC_TVOC:
            /* Three implementations, one `value` offset - see the assertions. */
            out->value_type     = ZB_EVENT_SENSOR_TVOC;
            out->v.sensor.value = ((const s_zb_device_tvoc_sensor_ctx_t *)ctx)->value;
            break;
        case ZB_FUNC_FORMALDEHYDE:
            out->value_type     = ZB_EVENT_SENSOR_FORMALDEHYDE;
            out->v.sensor.value = ((const s_zb_device_formaldehyde_sensor_ctx_t *)ctx)->value;
            break;
        case ZB_FUNC_IAQ:
            out->value_type     = ZB_EVENT_SENSOR_IAQ;
            out->v.sensor.value = ((const s_zb_device_iaq_sensor_ctx_t *)ctx)->value;
            break;

        /* --- multi-quantity measurement ---------------------------------- */
        case ZB_FUNC_BATTERY:
        {
            const s_zb_device_battery_sensor_ctx_t *battery =
                (const s_zb_device_battery_sensor_ctx_t *)ctx;
            out->value_type        = ZB_EVENT_SENSOR_BATTERY;
            /* BatteryPercentageRemaining is in half-percent units,
             * BatteryVoltage in 100mV units. */
            out->v.battery.percent = battery->percentage / 2.0f;
            out->v.battery.voltage = battery->voltage / 10.0f;
            break;
        }
        case ZB_FUNC_ELECTRICAL:
        {
            /* AC phase A plus the device-wide frequency. The ctx also has
             * fields for phases B/C, the 3-phase totals and DC, but nothing in
             * the decode path ever populates them. */
            const s_zb_device_electrical_measurement_ctx_t *em =
                (const s_zb_device_electrical_measurement_ctx_t *)ctx;
            out->value_type                  = ZB_EVENT_SENSOR_ELECTRICAL_MEASUREMENT;
            out->v.electrical.voltage        = em->voltage_a;
            out->v.electrical.current        = em->current_a;
            out->v.electrical.power          = em->active_power_a;
            out->v.electrical.reactive_power = em->reactive_power_a;
            out->v.electrical.apparent_power = em->apparent_power_a;
            out->v.electrical.frequency      = em->ac_frequency;
            /* PowerFactor is a percentage in -100..+100; 0x80 is the ZCL
             * "invalid" sentinel. Reported as a ratio. */
            out->v.electrical.power_factor   = ((uint8_t)em->power_factor_a == 0x80u)
                                                   ? 0.0f
                                                   : (float)em->power_factor_a / 100.0f;
            break;
        }
        case ZB_FUNC_ENERGY:
        {
            const s_zb_device_energy_meter_ctx_t *meter =
                (const s_zb_device_energy_meter_ctx_t *)ctx;
            out->value_type      = ZB_EVENT_SENSOR_ENERGY_METERING;
            out->v.energy = meter->energy;
            break;
        }

        /* --- binary state -------------------------------------------------- */
        case ZB_FUNC_OCCUPANCY:
            out->value_type      = ZB_EVENT_SENSOR_OCCUPANCY;
            out->v.binary.active = ((const s_zb_device_occupancy_sensor_ctx_t *)ctx)->occupied;
            break;

        case ZB_FUNC_IAS_MOTION_SENSOR:
        case ZB_FUNC_IAS_CONTACT_SWITCH:
        case ZB_FUNC_IAS_DOOR_WINDOW_HANDLE:
        case ZB_FUNC_IAS_FIRE_SENSOR:
        case ZB_FUNC_IAS_WATER_SENSOR:
        case ZB_FUNC_IAS_CO_SENSOR:
        case ZB_FUNC_IAS_PERSONAL_EMERGENCY:
        case ZB_FUNC_IAS_VIBRATION_SENSOR:
        case ZB_FUNC_IAS_GENERIC_SENSOR:
        {
            const s_zb_device_ias_zone_ctx_t *zone = (const s_zb_device_ias_zone_ctx_t *)ctx;
            out->value_type        = ZB_EVENT_SENSOR_IAS_ZONE;
            out->v.ias.zone_type   = zone->zone_type;
            out->v.ias.zone_status = zone->zone_status;
            break;
        }

        /* --- on/off actuators ---------------------------------------------- */
        case ZB_FUNC_ONOFF_LIGHT:
        case ZB_FUNC_ONOFF_PLUGIN_UNIT:
            out->value_type = ZB_EVENT_LIGHT_ONOFF_STATE;
            out->v.onoff.on = ((const s_zb_device_on_off_light_ctx_t *)ctx)->on_off ? true : false;
            break;
        case ZB_FUNC_RELAY:
            out->value_type = ZB_EVENT_LIGHT_ONOFF_STATE;
            out->v.onoff.on = ((const s_zb_device_relay_ctx_t *)ctx)->on_off ? true : false;
            break;
        case ZB_FUNC_ONOFF_SMART_PLUG:
            out->value_type = ZB_EVENT_LIGHT_ONOFF_STATE;
            out->v.onoff.on = ((const s_zb_device_on_off_smart_plug_ctx_t *)ctx)->on_off ? true : false;
            break;
        case ZB_FUNC_MAINS_POWER_OUTLET:
            out->value_type = ZB_EVENT_LIGHT_ONOFF_STATE;
            out->v.onoff.on = ((const s_zb_device_mains_power_outlet_ctx_t *)ctx)->on_off;
            break;
        case ZB_FUNC_ONOFF_SWITCH:
            out->value_type = ZB_EVENT_SWITCH_ONOFF_STATE;
            out->v.onoff.on = ((const s_zb_device_on_off_switch_ctx_t *)ctx)->on_off;
            break;

        /* --- level / colour actuators --------------------------------------- */
        case ZB_FUNC_DIMMABLE_LIGHT:
        case ZB_FUNC_DIMMABLE_PLUGIN_UNIT:
            out->value_type    = ZB_EVENT_LIGHT_DIMMABLE_STATE;
            out->v.level.level = ((const s_zb_device_dimmable_light_ctx_t *)ctx)->level;
            break;
        case ZB_FUNC_DIMMER_SWITCH:
            out->value_type    = ZB_EVENT_SWITCH_LEVEL_STATE;
            out->v.level.level = ((const s_zb_device_dimmer_switch_ctx_t *)ctx)->level;
            break;
        case ZB_FUNC_COLOR_TEMP_LIGHT:
            out->value_type              = ZB_EVENT_LIGHT_COLOR_TEMP_STATE;
            out->v.color_temp.color_temp = ((const s_zb_device_color_temp_light_ctx_t *)ctx)->color_temp;
            break;
        case ZB_FUNC_COLOR_LIGHT:
        {
            const s_zb_device_color_light_ctx_t *color = (const s_zb_device_color_light_ctx_t *)ctx;
            out->value_type           = ZB_EVENT_LIGHT_COLOR_HUE_SAT_STATE;
            out->v.hue_sat.hue        = color->hue;
            out->v.hue_sat.saturation = color->saturation;
            break;
        }
        case ZB_FUNC_EXTENDED_COLOR_LIGHT:
        {
            const s_zb_device_extended_color_light_ctx_t *color =
                (const s_zb_device_extended_color_light_ctx_t *)ctx;
            out->value_type             = ZB_EVENT_LIGHT_EXTENDED_COLOR_STATE;
            out->v.ext_color.hue        = color->hue;
            out->v.ext_color.saturation = color->saturation;
            out->v.ext_color.color_temp = color->color_temp;
            break;
        }

        /* --- momentary / value-less ------------------------------------------ */
        case ZB_FUNC_BUTTON:
        {
            const s_zb_device_button_ctx_t *button = (const s_zb_device_button_ctx_t *)ctx;
            if (!button->has_action)
            {
                return false;   /* no press seen yet - nothing to report */
            }
            out->value_type      = ZB_EVENT_BUTTON_ACTION;
            out->v.button.action = button->last_action;
            break;
        }
        case ZB_FUNC_IAS_WARNING:
            out->value_type                 = ZB_EVENT_DEVICE_IAS_WARNING;
            out->v.ias_warning.max_duration = ((const s_zb_device_ias_warning_ctx_t *)ctx)->max_duration;
            break;

        case ZB_FUNC_BASIC_INFO:
            return false;       /* identity only, no reading */

        default:
            /* A type with no branch here would silently vanish from every state
             * snapshot. If it can publish a value, that is a bug in this switch
             * rather than a quiet no-op - say so out loud. */
            if (func->ops->emit_state != NULL)
            {
                ZB_LOGW(TAG, "%s() - func type 0x%02x (%s) has emit_state but no direct read; "
                             "add it to this switch or it stays missing from device state",
                        __func__, (unsigned)func->type, func->name);
            }
            return false;
    }

    /* The `sensor` union member carries a unit alongside the value; fill it for
     * exactly the readings that use that member. */
    switch (out->value_type)
    {
        case ZB_EVENT_SENSOR_TEMPERATURE:
        case ZB_EVENT_SENSOR_HUMIDITY:
        case ZB_EVENT_SENSOR_PRESSURE:
        case ZB_EVENT_SENSOR_ILLUMINANCE:
        case ZB_EVENT_SENSOR_FLOW:
        case ZB_EVENT_SENSOR_PM25:
        case ZB_EVENT_SENSOR_PM10:
        case ZB_EVENT_SENSOR_PM1:
        case ZB_EVENT_SENSOR_CO2:
        case ZB_EVENT_SENSOR_ECO2:
        case ZB_EVENT_SENSOR_TVOC:
        case ZB_EVENT_SENSOR_FORMALDEHYDE:
        case ZB_EVENT_SENSOR_IAQ:
            out->v.sensor.unit = zb_sensor_unit_for_event(out->value_type);
            break;
        default:
            break;
    }

    return true;
}

bool
zb_device_manager_value_is_stale(const s_zb_function_t *func)
{
    return (func != NULL) && func->value_stale;
}

/*
 * Mirror of device_manager_read_function_value(). Every measurement context in
 * this driver is {raw, float value} - the raw half is only kept for change
 * detection, so restoring just the scaled value is enough and leaves the next
 * real report looking like a change. Actuator contexts hold only the state
 * itself, so they restore field for field.
 *
 * What this function will write is a superset of what the state cache chooses
 * to persist - zb_device_state_cache_is_cacheable() holds that policy. A type
 * missing here cannot be restored at all; a type present here is restorable and
 * the cache decides whether it is worth restoring.
 */
bool
zb_device_manager_write_function_value(s_zb_function_t *func, const s_zb_func_value_t *in)
{
    if ((func == NULL) || (in == NULL) || (func->ctx == NULL) || (func->ops == NULL))
    {
        return false;
    }
    if (func->type != in->type)
    {
        return false;
    }

    void *ctx = func->ctx;

    switch (func->type)
    {
        /* --- the `sensor` member: {raw, float value} --- */
        case ZB_FUNC_TEMPERATURE:
            ((s_zb_device_temperature_sensor_ctx_t *)ctx)->value = in->v.sensor.value;
            return true;
        case ZB_FUNC_HUMIDITY:
            ((s_zb_device_humidity_sensor_ctx_t *)ctx)->value = in->v.sensor.value;
            return true;
        case ZB_FUNC_PRESSURE:
            ((s_zb_device_pressure_sensor_ctx_t *)ctx)->value = in->v.sensor.value;
            return true;
        case ZB_FUNC_FLOW:
            ((s_zb_device_flow_sensor_ctx_t *)ctx)->value = in->v.sensor.value;
            return true;
        case ZB_FUNC_ILLUMINANCE:
            ((s_zb_device_illuminance_sensor_ctx_t *)ctx)->lux = in->v.sensor.value;
            return true;
        case ZB_FUNC_PM25:
            ((s_zb_device_pm25_sensor_ctx_t *)ctx)->value = in->v.sensor.value;
            return true;
        case ZB_FUNC_PM10:
            ((s_zb_device_pm10_sensor_ctx_t *)ctx)->value = in->v.sensor.value;
            return true;
        case ZB_FUNC_PM1:
            ((s_zb_device_pm1_sensor_ctx_t *)ctx)->value = in->v.sensor.value;
            return true;
        case ZB_FUNC_CO2:
            ((s_zb_device_co2_sensor_ctx_t *)ctx)->value = in->v.sensor.value;
            return true;
        case ZB_FUNC_ECO2:
            ((s_zb_device_eco2_sensor_ctx_t *)ctx)->value = in->v.sensor.value;
            return true;
        case ZB_FUNC_TVOC:
            ((s_zb_device_tvoc_sensor_ctx_t *)ctx)->value = in->v.sensor.value;
            return true;
        case ZB_FUNC_FORMALDEHYDE:
            ((s_zb_device_formaldehyde_sensor_ctx_t *)ctx)->value = in->v.sensor.value;
            return true;
        case ZB_FUNC_IAQ:
            ((s_zb_device_iaq_sensor_ctx_t *)ctx)->value = in->v.sensor.value;
            return true;

        /* The context holds the raw ZCL encodings - half-percent and 100 mV -
         * so invert exactly what the reader applies on the way out. */
        case ZB_FUNC_BATTERY:
        {
            s_zb_device_battery_sensor_ctx_t *battery = (s_zb_device_battery_sensor_ctx_t *)ctx;
            battery->percentage = (uint8_t)((in->v.battery.percent * 2.0f) + 0.5f);
            battery->voltage    = (uint8_t)((in->v.battery.voltage * 10.0f) + 0.5f);
            return true;
        }

        /* --- multi-quantity measurement ------------------------------------
         * The context keeps both raw and scaled forms; only the scaled half is
         * cached, which is what every consumer reads. */
        case ZB_FUNC_ELECTRICAL:
        {
            s_zb_device_electrical_measurement_ctx_t *em =
                (s_zb_device_electrical_measurement_ctx_t *)ctx;
            em->voltage_a        = in->v.electrical.voltage;
            em->current_a        = in->v.electrical.current;
            em->active_power_a   = in->v.electrical.power;
            em->reactive_power_a = in->v.electrical.reactive_power;
            em->apparent_power_a = in->v.electrical.apparent_power;
            em->ac_frequency     = in->v.electrical.frequency;
            /* The reader turns the ZCL percentage into a ratio and maps the
             * 0x80 "invalid" sentinel to 0.0; invert the scaling. A value that
             * was invalid comes back as a plain 0, which is what the reader
             * would have reported for it anyway. */
            em->power_factor_a   = (int8_t)((in->v.electrical.power_factor * 100.0f) +
                                            ((in->v.electrical.power_factor < 0.0f) ? -0.5f : 0.5f));
            return true;
        }
        case ZB_FUNC_ENERGY:
            ((s_zb_device_energy_meter_ctx_t *)ctx)->energy = in->v.energy;
            return true;

        /* --- on/off actuators ----------------------------------------------
         * Last known position. A mains actuator is re-read within seconds of
         * the network coming up (poll_interval_ms == 0 => one read while
         * !synced), so a restored value is short-lived - but "short-lived" is
         * still seconds of a blank row on every reboot for every lamp and plug
         * in the house, and the caller flags the value stale. */
        case ZB_FUNC_ONOFF_LIGHT:
        case ZB_FUNC_ONOFF_PLUGIN_UNIT:
            ((s_zb_device_on_off_light_ctx_t *)ctx)->on_off = in->v.onoff.on ? 1u : 0u;
            return true;
        case ZB_FUNC_RELAY:
            ((s_zb_device_relay_ctx_t *)ctx)->on_off = in->v.onoff.on ? 1u : 0u;
            return true;
        case ZB_FUNC_ONOFF_SMART_PLUG:
            ((s_zb_device_on_off_smart_plug_ctx_t *)ctx)->on_off = in->v.onoff.on ? 1u : 0u;
            return true;
        case ZB_FUNC_MAINS_POWER_OUTLET:
            ((s_zb_device_mains_power_outlet_ctx_t *)ctx)->on_off = in->v.onoff.on;
            return true;
        case ZB_FUNC_ONOFF_SWITCH:
            ((s_zb_device_on_off_switch_ctx_t *)ctx)->on_off = in->v.onoff.on;
            return true;

        /* --- level / colour actuators --------------------------------------- */
        case ZB_FUNC_DIMMABLE_LIGHT:
        case ZB_FUNC_DIMMABLE_PLUGIN_UNIT:
            ((s_zb_device_dimmable_light_ctx_t *)ctx)->level = in->v.level.level;
            return true;
        case ZB_FUNC_DIMMER_SWITCH:
            ((s_zb_device_dimmer_switch_ctx_t *)ctx)->level = in->v.level.level;
            return true;
        case ZB_FUNC_COLOR_TEMP_LIGHT:
            ((s_zb_device_color_temp_light_ctx_t *)ctx)->color_temp = in->v.color_temp.color_temp;
            return true;
        case ZB_FUNC_COLOR_LIGHT:
        {
            s_zb_device_color_light_ctx_t *color = (s_zb_device_color_light_ctx_t *)ctx;
            color->hue        = in->v.hue_sat.hue;
            color->saturation = in->v.hue_sat.saturation;
            return true;
        }
        case ZB_FUNC_EXTENDED_COLOR_LIGHT:
        {
            s_zb_device_extended_color_light_ctx_t *color =
                (s_zb_device_extended_color_light_ctx_t *)ctx;
            color->hue        = in->v.ext_color.hue;
            color->saturation = in->v.ext_color.saturation;
            color->color_temp = in->v.ext_color.color_temp;
            return true;
        }

        default:
            /* Momentary actions, alarm states and value-less functions: not
             * restorable by design. See zb_device_state_cache_is_cacheable()
             * for why replaying them would be wrong. */
            return false;
    }
}

uint8_t
zb_device_manager_read_function_values(uint64_t ieee_addr, s_zb_func_value_t *out)
{
    device_manager_lock();
    const s_zb_device_t *device = ieee_hash_table_get(ieee_addr);
    if (device == NULL)
    {
        device_manager_unlock();
        return 0;
    }

    uint8_t count = 0;
    for (uint8_t f = 0; f < device->function_count && f < ZB_MAX_FUNCTIONS; f++)
    {
        if (device_manager_read_function_value(&device->functions[f], &out[count]))
        {
            count++;
        }
    }
    device_manager_unlock();
    return count;
}

void
zb_device_manager_publish_device_state_array(uint64_t ieee_addr, uint32_t txn_id)
{
    device_manager_lock();
    if (!s_event_notify_callback)
    {
        ZB_LOGW(TAG, "%s() - no event callback registered, dropping txn=%lu",
                __func__, (unsigned long)txn_id);
        device_manager_unlock();
        return;
    }

    const uint16_t dev_total = (ieee_addr != 0)
                                   ? (ieee_hash_table_get(ieee_addr) ? 1u : 0u)
                                   : s_device_manager.device_count;

    s_zb_event_t ev;
    memset(&ev, 0, sizeof(ev));
    ev.type = ZB_EVENT_DEVICE_STATE_BEGIN;
    ev.ieee_addr = ieee_addr;
    ev.device_state_begin.txn_id = txn_id;
    ev.device_state_begin.device_count = dev_total;
    s_event_notify_callback(&ev);

    uint16_t emitted = 0;
    for (uint16_t i = 0; i < ZB_MAX_DEVICE; i++)
    {
        s_zb_device_t *device = s_device_manager.pool[i];
        if (!device)
            continue;
        if (ieee_addr != 0 && device->ieee_addr != ieee_addr)
            continue;

        memset(&ev, 0, sizeof(ev));
        ev.type = ZB_EVENT_DEVICE_STATE;
        ev.ieee_addr = device->ieee_addr;
        ev.device_state.txn_id = txn_id;
        ev.device_state.count = device_manager_collect_device_values(device, ev.device_state.functions);
        s_event_notify_callback(&ev);
        emitted++;
    }

    memset(&ev, 0, sizeof(ev));
    ev.type = ZB_EVENT_DEVICE_STATE_END;
    ev.ieee_addr = ieee_addr;
    ev.device_state_end.txn_id = txn_id;
    ev.device_state_end.emitted = emitted;
    ev.device_state_end.status = ZB_OK;
    s_event_notify_callback(&ev);

    ZB_LOGI(TAG, "%s() - txn=%lu ieee=0x%llx emitted %u device state(s)",
            __func__, (unsigned long)txn_id, (unsigned long long)ieee_addr, emitted);
    device_manager_unlock();
}

void
zb_device_manager_register_event_notify_callback(zb_event_notify_callback_t callback)
{
    device_manager_lock();
    s_event_notify_callback = callback;
    device_manager_unlock();
}

void
zb_device_manager_notify_event(s_zb_device_t *device, s_zb_function_t *func, s_zb_event_t *event)
{
    device_manager_lock();
    func->value_stale = false;
    event->ieee_addr = device->ieee_addr;
    event->sensor_id = func->sensor_id;
    strncpy(event->name, func->name, sizeof(event->name));
    if (s_capture_slot != NULL)
    {
        /* Array-snapshot in progress: harvest this event instead of publishing.
         * A snapshot reads state, it does not change it - nothing to dirty. */
        *s_capture_slot = *event;
        s_capture_hit = true;
    }
    else
    {
        /* Every driver funnels a changed value through here, so this is the one
         * place that sees them all. mark_seen() already dirties the cache for
         * anything a device sends, but an actuator's optimistic update (the
         * commanded value adopted before the device reports) touches no frame
         * from the device - on a quiet network that new on/off or level would
         * otherwise sit unflushed. */
        zb_device_state_cache_mark_dirty();
        if (s_event_notify_callback)
        {
            s_event_notify_callback(event);
        }
    }
    device_manager_unlock();
}

/******************************************************************************
 * Pool Allocator
 ******************************************************************************/

static s_zb_device_t *
device_manager_alloc_device(void)
{
    if (s_device_manager.device_count >= ZB_MAX_DEVICE)
    {
        ZB_LOGE(TAG, "Device pool exhausted!");
        return NULL;
    }
    for (int i = 0; i < ZB_MAX_DEVICE; i++)
    {
        if (!s_device_manager.block_pool->used[i])
        {
            s_device_manager.block_pool->used[i] = true;
            s_zb_device_t *device = &s_device_manager.block_pool->blocks[i];
            memset(device, 0, sizeof(s_zb_device_t));
            device->pool_idx = i;
            device->nwk_addr = 0xFFFF;
            device->in_use = true;
            s_device_manager.pool[i] = device;
            s_device_manager.device_count++;
            return device;
        }
    }
    ZB_LOGE(TAG, "Device pool exhausted!");
    return NULL;
}

static void
device_manager_free_functions(s_zb_device_t *device)
{
    for (int i = 0; i < device->function_count; i++)
    {
        s_zb_function_t *func = &device->functions[i];
        if (func->ops && func->ops->destroy)
            func->ops->destroy(device, func);
        if (func->ctx)
        {
            ZB_MEM_FREE(func->ctx);
            func->ctx = NULL;
        }
    }
    device->function_count = 0;
}

static void
device_manager_free_device(s_zb_device_t *device)
{
    device_manager_free_functions(device);
    ieee_hash_table_remove(device->ieee_addr);
    availability_slot_reset(device->pool_idx);
    s_device_manager.pool[device->pool_idx] = NULL;
    s_device_manager.block_pool->used[device->pool_idx] = false;
    s_device_manager.device_count--;
    memset(device, 0, sizeof(s_zb_device_t));
}

/******************************************************************************
 * Build function from endpoints
 ******************************************************************************/

static void
device_manager_build_functions(s_zb_device_t *device)
{
    /*
     * Drop any previously-built functions first.
     *
     * If this routine is called more than once on the same device (e.g. after
     * a re-interview), the device->functions[] slots still hold the ctx
     * pointers and ops vtables from the previous build. Without an explicit
     * teardown the next pass would either:
     *   - keep a stale ctx pointer (the `!func->ctx` guard prevents reallocation),
     *     so every device that re-uses the same pool slot ends up "sharing"
     *     a ctx address that was originally allocated for a different
     *     device/function — the symptom reported as "all devices have the
     *     same ctx memory address", or
     *   - silently leak the old ctx blocks if the slot is overwritten.
     * Calling free_functions() here resets function_count to 0 and frees
     * every previous ctx exactly once, guaranteeing a clean slate.
     */
    device_manager_free_functions(device);

    uint8_t name_counters[256] = {0};
    ZB_LOGI(TAG, "Building functions for device 0x%04x %s %s",
            device->nwk_addr, device->manufacturer, device->model);

    for (int i = 0; i < device->endpoint_count; i++)
    {
        s_zb_device_endpoint_t *endpoint = &device->endpoints[i];
        ZB_LOGI(TAG, "Building functions for endpoint %d", endpoint->endpoint_id);

        /* Clusters to build functions for: everything the endpoint advertises,
         * plus any synthetic clusters this device is known to use WITHOUT
         * advertising them (see s_zb_synthetic_cluster_quirk_t). The endpoint's
         * in_clusters[] is deliberately not modified, so what we persist stays a
         * truthful copy of the Simple Descriptor and the quirk re-applies on
         * every build, including after a reload from flash. */
        struct
        {
            uint16_t cluster_id;
            uint16_t ias_zone_type;
            bool     synthetic;   /* added by a quirk, not advertised by the device */
        } build_list[ZB_MAX_IN_CLUSTERS + ZB_MAX_SYNTHETIC_CLUSTERS_PER_ENDPOINT];
        uint8_t build_count = 0;

        for (int j = 0; j < endpoint->in_cluster_count && build_count < ARRAY_SIZE(build_list); j++)
        {
            build_list[build_count].cluster_id = endpoint->in_clusters[j];
            build_list[build_count].ias_zone_type = endpoint->ias_zone_type;
            build_list[build_count].synthetic = false;
            build_count++;
        }

        uint16_t quirk_count = 0;
        const s_zb_synthetic_cluster_quirk_t *quirks = zb_device_schema_synthetic_clusters(&quirk_count);
        for (uint16_t q = 0; q < quirk_count && build_count < ARRAY_SIZE(build_list); q++)
        {
            const s_zb_synthetic_cluster_quirk_t *sq = &quirks[q];
            bool already_present = false;

            if (sq->endpoint_id != endpoint->endpoint_id)
                continue;
            if (sq->manufacturer && strcmp(sq->manufacturer, device->manufacturer) != 0)
                continue;
            if (sq->model && strcmp(sq->model, device->model) != 0)
                continue;

            /* If the device does advertise it after all, the real entry wins. */
            for (uint8_t k = 0; k < build_count; k++)
            {
                if (build_list[k].cluster_id == sq->cluster_id)
                {
                    already_present = true;
                    break;
                }
            }
            if (already_present)
                continue;

            ZB_LOGI(TAG, "0x%llx - synthesising cluster 0x%04X on ep %d for %s/%s",
                    device->ieee_addr, sq->cluster_id, endpoint->endpoint_id,
                    device->manufacturer, device->model);
            build_list[build_count].cluster_id = sq->cluster_id;
            build_list[build_count].ias_zone_type = sq->ias_zone_type;
            build_list[build_count].synthetic = true;
            build_count++;
        }

        for (int j = 0; j < build_count; j++)
        {
            uint16_t cluster_id = build_list[j].cluster_id;
            ZB_LOGI(TAG, "Building function for cluster %x", cluster_id);

            if (zb_device_schema_cluster_is_ignored(cluster_id))
                continue;

            /* Manufacturer/model-gated clusters (e.g. Develco VOC 0xFC03) only
             * produce a function on matching devices. */
            if (!zb_device_schema_cluster_allowed(cluster_id, device->manufacturer, device->model))
            {
                ZB_LOGW(TAG, "0x%llx - Skipping manufacturer-gated cluster 0x%04X on %s/%s",
                        device->ieee_addr, cluster_id, device->manufacturer, device->model);
                continue;
            }

            if (device->function_count >= ZB_MAX_FUNCTIONS)
            {
                ZB_LOGE(TAG, "Max function count reached for device 0x%04x", device->nwk_addr);
                break;
            }

            /*
             * Zero the candidate slot before touching any field. This is the
             * single source of truth that func->ctx == NULL on entry, so the
             * ctx allocation below is always a fresh, per-instance block.
             * function_count is only incremented once schema/ops/ctx have all
             * been resolved successfully, so on the `continue` path the slot
             * stays zeroed and is reused by the next iteration.
             */
            s_zb_function_t *func = &device->functions[device->function_count];
            memset(func, 0, sizeof(*func));
            func->sensor_id = ZB_SENSOR_ID(endpoint->endpoint_id, cluster_id);

            if (cluster_id == ZCL_CLUSTER_ID_SS_IAS_ZONE)
            {
                /* Zone type comes from the build list, not the endpoint: for a
                 * synthesised IAS cluster the device has no zone-type attribute
                 * to read, so the quirk supplies it. */
                uint16_t ias_zone_type = build_list[j].ias_zone_type;
                func->color_caps = ZB_LIGHT_COLOR_CAP_NONE;
                const s_zb_device_ias_zone_schema_t *schema = zb_device_schema_ias_find(ias_zone_type);
                if (!schema)
                {
                    ZB_LOGW(TAG, "No schema found for IAS zone type 0x%04X", ias_zone_type);
                    func->type = ZB_FUNC_IAS_GENERIC_SENSOR;
                    func->ops = &zb_device_ias_zone_ops;
                    snprintf(func->name, sizeof(func->name), "ias_0x%04X", ias_zone_type);
                }
                else
                {
                    ZB_LOGI(TAG, "Found schema for IAS zone type 0x%04X: %s", ias_zone_type, schema->name);
                    func->type = schema->type;
                    func->ops = schema->ops;
                    uint8_t n = ++name_counters[func->type];
                    snprintf(func->name, sizeof(func->name), n == 1 ? "%s" : "%s_%d", schema->name, n);
                }
            }
            else
            {
                const s_zb_device_cluster_schema_t *schema = zb_device_schema_cluster_find(cluster_id, endpoint->profile_id, endpoint->device_id);
                if (!schema)
                {
                    ZB_LOGW(TAG, "No schema found for cluster 0x%04X, profile_id 0x%04X, device_id 0x%04X", cluster_id, endpoint->profile_id, endpoint->device_id);
                    continue;
                }

                const char *base_name = schema->base_name;
                func->type = schema->type;
                func->color_caps = schema->color_capabilities;
                func->ops = schema->ops;

                /* The schema matched on the ZCL device ID, which only names the
                 * light family. ColorCapabilities, read during the interview,
                 * says what the endpoint can really do - and some lights
                 * disagree with their own device ID (see
                 * zb_device_schema_color_find_by_caps). Trust the attribute. */
                uint16_t effective_caps = endpoint->color_caps;
                bool caps_known = endpoint->color_caps_valid;

                /* A model whose reported value is known to be wrong overrides
                 * it - and is authoritative even if the read never landed. */
                if (zb_device_schema_color_caps_override(device->manufacturer, device->model,
                                                         &effective_caps))
                {
                    caps_known = true;
                }

                if (cluster_id == ZCL_CLUSTER_ID_LIGHTING_COLOR_CONTROL && caps_known)
                {
                    const s_zb_device_cluster_schema_t *refined =
                        zb_device_schema_color_find_by_caps(effective_caps);
                    if (refined != NULL && refined->type != func->type)
                    {
                        ZB_LOGI(TAG, "0x%llx - color caps 0x%04X on ep %u: %s -> %s",
                                device->ieee_addr, effective_caps, endpoint->endpoint_id,
                                base_name, refined->base_name);
                        func->type = refined->type;
                        func->color_caps = refined->color_capabilities;
                        func->ops = refined->ops;
                        base_name = refined->base_name;
                    }
                }

                uint8_t n = ++name_counters[func->type];
                snprintf(func->name, sizeof(func->name), n == 1 ? "%s" : "%s_%d", base_name, n);
            }

            /* Resolve the state-sync poll cadence (type default + quirk override).
             * last_read_ms / synced were zeroed by the memset above. */
            func->poll_interval_ms = zb_device_schema_resolve_poll_ms(device->manufacturer, device->model, cluster_id, func->type);

            /*
             * Allocate per-instance ctx. Two devices that share the same
             * function type share the same `func->ops` pointer (vtable
             * singleton) but MUST each own a distinct ctx block — the
             * unconditional ZB_MEM_CALLOC here is what guarantees that.
             */
            if (func->ops && func->ops->ctx_size > 0)
            {
                func->ctx = ZB_MEM_CALLOC(1, func->ops->ctx_size);
                if (!func->ctx)
                {
                    ZB_LOGE(TAG, "0x%llx - Failed to allocate ctx (size=%u) for '%s' - skipping function",
                            device->ieee_addr, (unsigned)func->ops->ctx_size, func->name);
                    memset(func, 0, sizeof(*func));
                    continue;
                }
            }

            if (func->ops && func->ops->init)
                func->ops->init(device, func);

            device->function_count++;
        }
    }

    /* OTA capability: a device is OTA-upgradable if it is a client of the OTA
     * Upgrade cluster (0x0019), which appears in its OUTPUT cluster list. Derive
     * it here so it is correct on both first join and reload from flash (the
     * device record never carries an explicit ota_supported flag from the
     * interview). */
    device->ota_supported = false;
    for (int i = 0; i < device->endpoint_count && !device->ota_supported; i++)
    {
        s_zb_device_endpoint_t *ep = &device->endpoints[i];
        for (int c = 0; c < ep->out_cluster_count; c++)
        {
            if (ep->out_clusters[c] == ZCL_CLUSTER_ID_OTA)
            {
                device->ota_supported = true;
                break;
            }
        }
    }

    /* Capability fallback: a few devices never answer the Node Descriptor, so
     * manu_id/device_type/battery/sleepy stay at their zero defaults. If a quirk
     * exists for this manufacturer/model, apply its known-good values here (the
     * model is only known now, from the Basic cluster, well after the Node
     * Descriptor step). Runs on both first join and reload from flash. */
    const s_zb_device_caps_quirk_t *caps = zb_device_schema_caps_find(device->manufacturer, device->model);
    if (caps)
    {
        device->manu_id = caps->manu_id;
        device->device_type = caps->device_type;
        device->battery_powered = caps->battery_powered;
        device->sleepy_enabled = caps->sleepy_enabled;
        ZB_LOGI(TAG, "0x%llx - applied capability quirk: manu_id=0x%04X type=%u battery=%d sleepy=%d",
                device->ieee_addr, device->manu_id, device->device_type,
                device->battery_powered, device->sleepy_enabled);
    }

    ZB_LOGI(TAG, "Built %d functions for device %s %s (ota=%d)",
            device->function_count, device->manufacturer, device->model, device->ota_supported);
}

static bool
device_manager_function_needs_auto_bind(e_zb_function_type_t type)
{
    switch (type)
    {
        case ZB_FUNC_BATTERY:
        case ZB_FUNC_TEMPERATURE:
        case ZB_FUNC_HUMIDITY:
        case ZB_FUNC_PRESSURE:
        case ZB_FUNC_ILLUMINANCE:
        case ZB_FUNC_OCCUPANCY:
        case ZB_FUNC_FLOW:
        case ZB_FUNC_PM25:
        case ZB_FUNC_CO2:
        case ZB_FUNC_ELECTRICAL:
        case ZB_FUNC_ENERGY:
        case ZB_FUNC_PM10:
        case ZB_FUNC_PM1:
        case ZB_FUNC_TVOC:
        case ZB_FUNC_FORMALDEHYDE:
        case ZB_FUNC_IAQ:
        case ZB_FUNC_ECO2:
            return true;
        default:
            return false;
    }
}

/* Record coordinator bind intent for every report-capable sensor function that
 * does not already have a BIND entry (manual or from a prior join). Called only
 * on first join (add_device), not when reloading devices from flash — so an
 * operator UNBIND persists across reboot via the .cfg sidecar. */
static void
device_manager_auto_bind_sensor_functions(s_zb_device_t *device)
{
    for (uint8_t i = 0; i < device->function_count; i++)
    {
        s_zb_function_t *func = &device->functions[i];
        if (!device_manager_function_needs_auto_bind(func->type))
            continue;

        s_zb_desired_config_t bind = {0};
        bind.kind         = ZB_DESIRED_CFG_BIND;
        bind.src_endpoint = ZB_SENSOR_EP(func->sensor_id);
        bind.cluster_id   = ZB_SENSOR_CLUSTER(func->sensor_id);
        bind.dst_ieee     = 0;              /* coordinator */
        bind.dst_endpoint = ZB_HUB_ENDPOINT;

        bool exists = false;
        for (uint8_t j = 0; j < device->desired_config_count; j++)
        {
            if (desired_config_match(&device->desired_config[j], &bind))
            {
                exists = true;
                break;
            }
        }
        if (exists)
            continue;

        ZB_LOGI(TAG, "0x%llx - auto-bind %s (cl 0x%04x ep %u -> coordinator)",
                device->ieee_addr, func->name, bind.cluster_id, bind.src_endpoint);
        zb_device_manager_record_desired_config(device, &bind);
    }
}

/******************************************************************************
 * Load device from database
 ******************************************************************************/

static void
make_path(uint64_t ieee_addr, char *path, size_t len, bool tmp)
{
    snprintf(path, len, tmp ? "%s/%llX.tmp" : "%s/%llX.pb", ZB_DIR, ieee_addr);
}

/* Desired-config sidecar path; defined with the desired-config section below. */
static void make_cfg_path(uint64_t ieee_addr, char *path, size_t len, bool tmp);

zb_status_t
zb_device_manager_init(void)
{
    if (s_device_manager_mutex == NULL)
    {
        s_device_manager_mutex = zb_os_mutex_create(true);
        if (s_device_manager_mutex == NULL)
        {
            ZB_LOGE(TAG, "Failed to create device-manager mutex");
            return ZB_FAIL;
        }
    }

    device_manager_lock();
    memset(&s_device_manager, 0, sizeof(s_zb_device_manager_t));
    s_device_manager.block_pool = ZB_MEM_CALLOC(1, sizeof(s_zb_device_pool_t));
    if (!s_device_manager.block_pool)
    {
        ZB_LOGE(TAG, "Failed to allocate block pool");
        device_manager_unlock();
        return ZB_FAIL;
    }

    zb_device_manager_load_device_file();
    ZB_LOGI(TAG, "Device manager initialized. Device pool size: %d", sizeof(s_zb_device_pool_t));
    device_manager_unlock();
    return ZB_OK;
}

zb_status_t
zb_device_manager_load_device_file(void)
{
    device_manager_lock();
    struct stat st;
    if (stat(ZB_DIR, &st) != 0)
        mkdir(ZB_DIR, 0777);

    DIR *dir = opendir(ZB_DIR);
    if (!dir)
    {
        ZB_LOGE(TAG, "Failed to open device directory");
        device_manager_unlock();
        return ZB_OK;
    }

    int ok = 0, skip = 0;
    struct dirent *entry;

    while ((entry = readdir(dir)) != NULL)
    {
        ZB_LOGI(TAG, "FILE NAME: %s", entry->d_name);
        // remove leftover .tmp files from interrupted writes
        if (strstr(entry->d_name, ".tmp"))
        {
            char stale[256];
            int n = snprintf(stale, sizeof(stale), "%s/%s", ZB_DIR, entry->d_name);
            if (n < 0 || n >= sizeof(stale))
            {
                ZB_LOGE(TAG, "Failed to create stale path: %s", entry->d_name);
                continue;
            }
            remove(stale);
            ZB_LOGW(TAG, "Removed stale tmp: %s", entry->d_name);
            continue;
        }
        if (!strstr(entry->d_name, ".pb"))
            continue;

        s_zb_device_t *dev = device_manager_alloc_device();
        if (!dev)
        {
            ZB_LOGE(TAG, "Pool full at %d devices", ok);
            break;
        }

        char path[256];
        int n = snprintf(path, sizeof(path), "%s/%s", ZB_DIR, entry->d_name);
        if (n < 0 || n >= sizeof(path))
        {
            ZB_LOGE(TAG, "Failed to create path: %s", entry->d_name);
            continue;
        }

        zb_status_t ret = zb_device_db_load_device(dev, path);
        if (ret != ZB_OK)
        {
            skip++;
            device_manager_free_device(dev);
            continue;
        }

        ieee_hash_table_set(dev->ieee_addr, dev);
        device_manager_build_functions(dev);
        zb_device_manager_load_config_file(dev);
        zb_device_manager_apply_poll_overrides(dev);
        /* After the functions exist - the cached values are written into their
         * contexts, so there has to be something to write into. */
        /* Sets dev->status too, derived from the cached last_seen - so leave it
         * alone here. A device with no cached record stays UNKNOWN (the value
         * the pool was zeroed to) until it first speaks. */
        zb_device_state_cache_restore(dev);
        ok++;
        ZB_LOGI(TAG, "[%d] Restored: %s %s (%d eps, %d funcs)", ok,
            dev->manufacturer, dev->model, dev->endpoint_count, dev->function_count);

    }

    closedir(dir);

    /* Resolve each device's parent short address from its (persisted) parent
     * IEEE: the parent short address is not stored, so look the parent up by
     * IEEE and copy its current nwk_addr. Devices parented by the coordinator
     * have no matching device entry, so their parent_nwk_addr stays 0 - which
     * is exactly the coordinator's short address (0x0000). Refreshed later on
     * rejoin; this just gives a correct tree immediately after restore. */
    for (int i = 0; i < ZB_MAX_DEVICE; i++)
    {
        s_zb_device_t *dev = s_device_manager.pool[i];
        if (dev == NULL || dev->parent_ieee == 0)
        {
            continue;
        }
        s_zb_device_t *parent = ieee_hash_table_get(dev->parent_ieee);
        if (parent != NULL)
        {
            dev->parent_nwk_addr = parent->nwk_addr;
        }
    }

    ZB_LOGI(TAG, "Restore complete: %d ok, %d skipped", ok, skip);
    device_manager_unlock();
    return ZB_OK;
}

zb_status_t
zb_device_manager_update_device_network_address(void)
{
    device_manager_lock();
    for (int i = 0; i < ZB_MAX_DEVICE; i++)
    {
        if (s_device_manager.pool[i])
        {
            zb_zdo_send_nwk_addr_req(s_device_manager.pool[i]->ieee_addr);
            ZB_TASK_DELAY_MS(10);
        }
    }
    device_manager_unlock();
    return ZB_OK;
}

zb_status_t
zb_device_manager_delete_device_file(uint64_t ieee_addr)
{
    device_manager_lock();
    char path[80];
    make_path(ieee_addr, path, sizeof(path), false);
    remove(path);
    make_cfg_path(ieee_addr, path, sizeof(path), false);
    remove(path); /* drop the desired-config sidecar too */
    zb_device_state_cache_forget(ieee_addr);
    ZB_LOGI(TAG, "%s() - Deleted device + config: 0x%llX", __func__, ieee_addr);
    device_manager_unlock();
    return ZB_OK;
}

zb_status_t
zb_device_manager_save_device_file(s_zb_device_t *device)
{
    device_manager_lock();
    char path[80], tmp[80];
    make_path(device->ieee_addr, path, sizeof(path), false);
    make_path(device->ieee_addr, tmp, sizeof(tmp), true);

    // atomic write: .tmp first, then rename over .pb
    zb_status_t ret = zb_device_db_update_device(device, tmp);
    if (ret != ZB_OK)
    {
        ZB_LOGE(TAG, "%s() - Failed to update device: %s", __func__, tmp);
        remove(tmp);
        device_manager_unlock();
        return ZB_FAIL;
    }

    if (rename(tmp, path) != 0)
    {
        remove(tmp);
        device_manager_unlock();
        return ZB_FAIL;
    }

    ZB_LOGI(TAG, "%s() - Saved: %s", __func__, path);
    device_manager_unlock();
    return ZB_OK;
}

/******************************************************************************
 * Desired configuration (bind / attribute reporting)
 *
 * Bindings and reporting configs live on the *remote device*; the host keeps
 * the intent so it can re-apply after a device factory-reset/rejoin (the device
 * comes back blank) or defensively on network-up. Persisted as a small sidecar
 * file "<IEEE>.cfg" next to the protobuf device record.
 ******************************************************************************/

#define ZB_CFG_MAGIC   0x5A424346u   /* 'Z''B''C''F' */
#define ZB_CFG_VERSION 2u            /* v2: added ZB_DESIRED_CFG_POLL + poll_interval_ms */

typedef struct
{
    uint32_t magic;
    uint16_t version;
    uint16_t count;
} s_zb_cfg_file_header_t;

static void
make_cfg_path(uint64_t ieee_addr, char *path, size_t len, bool tmp)
{
    snprintf(path, len, tmp ? "%s/%llX.cfg.tmp" : "%s/%llX.cfg", ZB_DIR, ieee_addr);
}

zb_status_t
zb_device_manager_load_config_file(s_zb_device_t *device)
{
    device_manager_lock();
    char path[80];
    make_cfg_path(device->ieee_addr, path, sizeof(path), false);

    device->desired_config_count = 0; /* clear any stale entries from a reused slot */

    FILE *f = fopen(path, "rb");
    if (!f)
    {
        device_manager_unlock();
        return ZB_OK; /* no sidecar yet - not an error */
    }

    s_zb_cfg_file_header_t hdr = {0};
    if (fread(&hdr, sizeof(hdr), 1, f) == 1 && hdr.magic == ZB_CFG_MAGIC && hdr.version == ZB_CFG_VERSION)
    {
        uint16_t count = (hdr.count > ZB_MAX_DESIRED_CONFIG) ? ZB_MAX_DESIRED_CONFIG : hdr.count;
        for (uint16_t i = 0; i < count; i++)
        {
            if (fread(&device->desired_config[i], sizeof(s_zb_desired_config_t), 1, f) != 1)
                break;
            device->desired_config_count++;
        }
        ZB_LOGI(TAG, "Loaded %u desired-config entries for 0x%llx", device->desired_config_count, device->ieee_addr);
    }
    else
    {
        ZB_LOGW(TAG, "Bad/empty config file: %s", path);
    }
    fclose(f);
    device_manager_unlock();
    return ZB_OK;
}

zb_status_t
zb_device_manager_save_config_file(s_zb_device_t *device)
{
    device_manager_lock();
    char path[80], tmp[80];
    make_cfg_path(device->ieee_addr, path, sizeof(path), false);
    make_cfg_path(device->ieee_addr, tmp, sizeof(tmp), true);

    /* No intent -> drop the sidecar instead of keeping a zero-entry file. */
    if (device->desired_config_count == 0)
    {
        remove(path);
        device_manager_unlock();
        return ZB_OK;
    }

    FILE *f = fopen(tmp, "wb");
    if (!f)
    {
        ZB_LOGE(TAG, "%s() - open for write failed: %s", __func__, tmp);
        device_manager_unlock();
        return ZB_FAIL;
    }
    s_zb_cfg_file_header_t hdr = { .magic = ZB_CFG_MAGIC, .version = ZB_CFG_VERSION, .count = device->desired_config_count };
    bool ok = (fwrite(&hdr, sizeof(hdr), 1, f) == 1) &&
              (fwrite(device->desired_config, sizeof(s_zb_desired_config_t), device->desired_config_count, f) == device->desired_config_count);
    fclose(f);
    if (!ok)
    {
        remove(tmp);
        device_manager_unlock();
        return ZB_FAIL;
    }
    if (rename(tmp, path) != 0)
    {
        remove(tmp);
        device_manager_unlock();
        return ZB_FAIL;
    }
    ZB_LOGI(TAG, "%s() - Saved %u entries: %s", __func__, device->desired_config_count, path);
    device_manager_unlock();
    return ZB_OK;
}

/* Identity for dedup/remove: kind + cluster + src endpoint, plus attr (REPORT)
 * or destination (BIND). */
static bool
desired_config_match(const s_zb_desired_config_t *a, const s_zb_desired_config_t *b)
{
    if (a->kind != b->kind || a->cluster_id != b->cluster_id || a->src_endpoint != b->src_endpoint)
        return false;
    if (a->kind == ZB_DESIRED_CFG_REPORT)
        return a->attr_id == b->attr_id;
    if (a->kind == ZB_DESIRED_CFG_BIND)
        return a->dst_ieee == b->dst_ieee && a->dst_endpoint == b->dst_endpoint;
    return true; /* POLL: one override per (cluster, endpoint) */
}

void
zb_device_manager_record_desired_config(s_zb_device_t *device, const s_zb_desired_config_t *cfg)
{
    device_manager_lock();
    for (uint8_t i = 0; i < device->desired_config_count; i++)
    {
        if (desired_config_match(&device->desired_config[i], cfg))
        {
            device->desired_config[i] = *cfg; /* replace in place */
            zb_device_manager_save_config_file(device);
            device_manager_unlock();
            return;
        }
    }
    if (device->desired_config_count >= ZB_MAX_DESIRED_CONFIG)
    {
        ZB_LOGW(TAG, "Desired-config table full for 0x%llx", device->ieee_addr);
        device_manager_unlock();
        return;
    }
    device->desired_config[device->desired_config_count++] = *cfg;
    zb_device_manager_save_config_file(device);
    device_manager_unlock();
}

void
zb_device_manager_remove_desired_config(s_zb_device_t *device, const s_zb_desired_config_t *cfg)
{
    device_manager_lock();
    for (uint8_t i = 0; i < device->desired_config_count; i++)
    {
        if (desired_config_match(&device->desired_config[i], cfg))
        {
            for (uint8_t j = i + 1; j < device->desired_config_count; j++)
                device->desired_config[j - 1] = device->desired_config[j];
            device->desired_config_count--;
            zb_device_manager_save_config_file(device);
            device_manager_unlock();
            return;
        }
    }
    device_manager_unlock();
}

zb_status_t
zb_device_manager_apply_desired_config_entry(s_zb_device_t *device,
        const s_zb_desired_config_t *cfg, bool for_config_apply)
{
    device_manager_lock();
    if (cfg->kind == ZB_DESIRED_CFG_POLL)
    {
        device_manager_unlock();
        return ZB_OK; /* local override, applied to func->poll_interval_ms at load (not on-air) */
    }

    if (cfg->kind == ZB_DESIRED_CFG_BIND)
    {
        uint64_t dst_ieee = cfg->dst_ieee;
        uint8_t  dst_ep   = cfg->dst_endpoint;
        if (dst_ieee == 0) /* default destination = coordinator */
        {
            s_zb_coordinator_info_t info = {0};
            if (zb_core_get_coordinator_info(&info) != ZB_OK)
            {
                device_manager_unlock();
                return ZB_FAIL;
            }
            dst_ieee = info.ieee_addr;
            dst_ep   = ZB_HUB_ENDPOINT;
        }
        zb_status_t rc = zb_core_bind_enqueue(device->nwk_addr, device->ieee_addr,
                cfg->src_endpoint, dst_ieee, dst_ep, cfg->cluster_id, for_config_apply);
        ZB_LOGI(TAG, "Enqueue BIND 0x%llx ep%u cl 0x%04x -> 0x%llx ep%u (%d)",
                device->ieee_addr, cfg->src_endpoint, cfg->cluster_id, dst_ieee, dst_ep, rc);
        device_manager_unlock();
        return rc;
    }

    /* ZB_DESIRED_CFG_REPORT — ConfigureReporting on the device's cluster. */
    uint8_t buf[sizeof(s_zb_zcl_config_report_cmd_t) + sizeof(s_zb_zcl_config_report_info_t)] = {0};
    s_zb_zcl_config_report_cmd_t *cr = (s_zb_zcl_config_report_cmd_t *)buf;
    cr->num_attr = 1;
    cr->attr_list[0].direction       = ZCL_SEND_ATTR_REPORTS;
    cr->attr_list[0].attr_id         = cfg->attr_id;
    cr->attr_list[0].data_type       = cfg->data_type;
    cr->attr_list[0].min_report_int  = cfg->min_interval;
    cr->attr_list[0].max_report_int  = cfg->max_interval;
    cr->attr_list[0].timeout_period  = 0;
    cr->attr_list[0].reportable_change = (uint8_t *)cfg->change;

    s_zb_af_address_t addr = {0};
    addr.address_mode = AF_ADDRESS_16BIT;
    addr.short_addr   = device->nwk_addr;
    addr.endpoint     = cfg->src_endpoint;
    uint8_t seq = zb_zcl_next_seq_num();
    /* Manufacturer-specific clusters (e.g. Develco VOC 0xFC03) require the
     * manufacturer code in the ConfigureReporting frame, otherwise the device
     * rejects it. Standard clusters use code 0 (non-manufacturer-specific).
     * The air-quality cluster 0x042E follows its selectable addressing mode. */
    uint16_t manuf_code = 0;
    if (cfg->cluster_id == ZCL_CLUSTER_ID_MS_DEVELCO_VOC)
        manuf_code = ZB_MANUFACTURER_CODE_DEVELCO;
    zb_status_t rc = zb_zcl_send_config_report_cmd_manu(ZB_HUB_ENDPOINT, &addr, cfg->cluster_id, cr, ZCL_FRAME_CLIENT_SERVER_DIR, false, manuf_code, seq);
    ZB_LOGI(TAG, "Apply REPORT 0x%llx ep%u cl 0x%04x attr 0x%04x [%u..%u] (%d)",
            device->ieee_addr, cfg->src_endpoint, cfg->cluster_id, cfg->attr_id, cfg->min_interval, cfg->max_interval, rc);
    device_manager_unlock();
    return rc;
}

void
zb_device_manager_apply_desired_config(s_zb_device_t *device)
{
    for (uint8_t i = 0; i < device->desired_config_count; i++)
    {
        zb_device_manager_apply_desired_config_entry(device, &device->desired_config[i], false);
    }
}

void
zb_device_manager_apply_poll_overrides(s_zb_device_t *device)
{
    device_manager_lock();
    /* Push any persisted POLL overrides onto their functions, replacing the
     * build-time resolved cadence. Called after build_functions + load_config. */
    for (uint8_t i = 0; i < device->desired_config_count; i++)
    {
        const s_zb_desired_config_t *e = &device->desired_config[i];
        if (e->kind != ZB_DESIRED_CFG_POLL)
            continue;
        s_zb_function_t *func = zb_device_manager_find_function(device, ZB_SENSOR_ID(e->src_endpoint, e->cluster_id));
        if (func)
        {
            func->poll_interval_ms = e->poll_interval_ms;
            ZB_LOGI(TAG, "0x%llx - poll override %s -> %lu ms", device->ieee_addr, func->name, (unsigned long)func->poll_interval_ms);
        }
    }
    device_manager_unlock();
}

/**
 * @brief Seed freshly built functions with attribute values captured during the
 *        interview.
 *
 * A device under interview has no pool entry and no functions, so reports it
 * sends on join are dropped by the live path. zb_nwksrv_zcl_observe() caches the
 * decoded values on device_info; this replays them once the functions exist.
 *
 * The value is delivered through the normal ops->on_attr_report() vtable entry
 * so every function type decodes it with exactly the same code as a live report
 * - no per-type replay logic. The ZCL message is synthesised from the cache:
 * everything the handlers read (cluster_id, src_endpoint, src_addr, and a
 * one-record report command) is known and filled in truthfully.
 *
 * Caller must hold the device manager lock.
 */
static void
device_manager_apply_cached_attrs(s_zb_device_t *device, const s_zb_device_info_t *device_info)
{
    for (uint8_t i = 0; i < device_info->cached_attr_count; i++)
    {
        const s_zb_cached_attr_t *cached = &device_info->cached_attrs[i];

        s_zb_function_t *func =
            zb_device_manager_find_function(device, ZB_SENSOR_ID(cached->endpoint, cached->cluster_id));
        if (func == NULL || func->ops == NULL || func->ops->on_attr_report == NULL)
        {
            ZB_LOGW(TAG, "0x%llx - no function for cached ep=%d cluster=0x%04x attr=0x%04x",
                device->ieee_addr, cached->endpoint, cached->cluster_id, cached->attr_id);
            continue;
        }

        /* Synthesise the AF + ZCL message. The union gives the flexible-array
         * report command correct alignment and room for its single record. */
        s_zb_af_incoming_msg_t af_msg = {0};
        af_msg.cluster_id = cached->cluster_id;
        af_msg.src_endpoint = cached->endpoint;
        af_msg.dst_endpoint = ZB_HUB_ENDPOINT;
        af_msg.src_addr.address_mode = AF_ADDRESS_16BIT;
        af_msg.src_addr.short_addr = device->nwk_addr;
        af_msg.src_addr.endpoint = cached->endpoint;

        union {
            s_zb_zcl_report_attr_cmd_t cmd;
            uint8_t raw[sizeof(s_zb_zcl_report_attr_cmd_t) + sizeof(s_zb_zcl_report_attr_info_t)];
        } report = {0};

        report.cmd.num_attr = 1;
        report.cmd.attr_list[0].attr_id = cached->attr_id;
        report.cmd.attr_list[0].data_type = cached->data_type;
        report.cmd.attr_list[0].attr_data = (uint8_t *)cached->value;

        s_zb_zcl_incoming_msg_t zcl_msg = {0};
        zcl_msg.msg = &af_msg;
        zcl_msg.hdr.command_id = ZCL_CMD_REPORT;
        zcl_msg.attr_cmd = &report.cmd;

        ZB_LOGI(TAG, "0x%llx - replaying cached report ep=%d cluster=0x%04x attr=0x%04x",
            device->ieee_addr, cached->endpoint, cached->cluster_id, cached->attr_id);
        func->ops->on_attr_report(device, func, &zcl_msg);
    }
}

zb_status_t
zb_device_manager_add_device(s_zb_device_info_t *device_info)
{
    device_manager_lock();
    s_zb_device_t *device = device_manager_alloc_device();
    if (!device)
    {
        ZB_LOGE(TAG, "%s() - No device slot available", __func__);
        device_manager_unlock();
        return ZB_FAIL;
    }

    ieee_hash_table_set(device_info->ieee_addr, device);

    device->ieee_addr = device_info->ieee_addr;
    device->parent_ieee = device_info->parent_ieee;
    device->manu_id = device_info->manu_id;
    device->nwk_addr = device_info->nwk_addr;
    device->parent_nwk_addr = device_info->parent_nwk_addr;
    strncpy(device->manufacturer, device_info->manufacturer, sizeof(device->manufacturer));
    strncpy(device->model, device_info->model, sizeof(device->model));
    strncpy(device->product_label, device_info->product_label, sizeof(device->product_label));
    strncpy(device->serial_number, device_info->serial_number, sizeof(device->serial_number));
    strncpy(device->date_code, device_info->date_code, sizeof(device->date_code));
    device->app_version = device_info->app_version;
    strncpy(device->sw_build_id, device_info->sw_build_id, sizeof(device->sw_build_id));
    device->product_code_len = device_info->product_code_len <= sizeof(device->product_code)
                                   ? device_info->product_code_len : sizeof(device->product_code);
    memcpy(device->product_code, device_info->product_code, device->product_code_len);
    device->hw_version = device_info->hw_version;
    device->physical_environment = device_info->physical_environment;
    device->power_source = device_info->power_source;
    device->capabilities = device_info->capabilities;

    device->endpoint_count = device_info->endpoint_count;
    memcpy(device->endpoints, device_info->endpoints, device_info->endpoint_count * sizeof(s_zb_device_endpoint_t));

    device_manager_build_functions(device);
    /* Pick up any intent persisted from a previous session for this IEEE so a
     * rejoining device gets re-bound/re-configured on the next state sync. */
    zb_device_manager_load_config_file(device);
    device_manager_auto_bind_sensor_functions(device);
    zb_device_manager_apply_poll_overrides(device);
    device_manager_notify_device_event(device, ZB_EVENT_DEVICE_JOINED);
    /* Replay attribute values that arrived while the device was still being
     * interviewed. Deliberately AFTER DEVICE_JOINED: these emit normal sensor
     * events, and the backend must learn the device exists before it is told
     * about its readings. The functions only exist as of build_functions()
     * above, which is why the live report path could not take these values. */
    device_manager_apply_cached_attrs(device, device_info);
    device_manager_unlock();
    return ZB_OK;
}

const s_zb_device_t *
zb_device_manager_acquire_device(uint64_t ieee_addr)
{
    /* Lock stays held on return (found or not); the caller reads under it and
     * must call zb_device_manager_release_device() on every path. */
    device_manager_lock();
    return ieee_hash_table_get(ieee_addr);
}

void
zb_device_manager_release_device(void)
{
    device_manager_unlock();
}

void
zb_device_manager_note_activity(uint64_t ieee_addr, uint8_t lqi)
{
    device_manager_lock();
    s_zb_device_t *device = ieee_hash_table_get(ieee_addr);
    if (device)
    {
        const bool was_online = (device->status == ZB_DEVICE_STATUS_ONLINE);

        device->last_seen = zb_get_utc_epoch_time();
        device->lqi = lqi;
        device->status = ZB_DEVICE_STATUS_ONLINE;
        /* Heard from: drop any pending ping and unwind the backoff ladder, so
         * a device that comes back is not still being checked on the widened
         * interval it earned while it was away. */
        availability_slot_reset(device->pool_idx);

        /* Recovery half of the availability mechanism: a device that had been
         * marked offline (or was never heard from) just spoke, so announce it.
         * Only the transition is published - a chatty sensor must not emit an
         * availability event per report. */
        if (!was_online)
        {
            s_zb_event_t ev = {0};
            ev.type = ZB_EVENT_DEVICE_AVAILABILITY;
            ev.ieee_addr = device->ieee_addr;
            ev.availability.online = true;
            ev.availability.silent_for_s = 0;
            if (s_event_notify_callback)
                s_event_notify_callback(&ev);
        }
        /* Liveness moved even if no function value did. Drivers only call
         * notify_event() when a reading *changes*, so a sensor sitting at a
         * steady value would otherwise never dirty the cache and its persisted
         * last_seen would age forever. */
        zb_device_state_cache_mark_dirty();
    }
    device_manager_unlock();
}

/******************************************************************************
 * Availability
 *
 * The only liveness signal we have is last_seen, refreshed by every incoming
 * frame (zb_device_manager_note_activity). A device that stops talking would
 * otherwise stay ONLINE forever, because nothing else ever writes
 * ZB_DEVICE_STATUS_OFFLINE. This is the other half.
 *
 * The shape follows zigbee2mqtt's availability extension:
 *
 *   timeouts   Routers answer promptly, so 10 minutes of silence means
 *              something is wrong. Sleepy end devices report on their own slow
 *              schedule and get a day, since we deliberately never poll them
 *              (zb_core_query_device_task skips end devices) and waking one to
 *              ask whether it is alive is exactly the cost the timeout exists
 *              to avoid.
 *
 *   ping       Silence is not proof of absence for a mains device: most
 *              functions default to ZB_POLL_NEVER (see poll_default_for_type),
 *              so a smart plug nobody touches legitimately says nothing for
 *              hours. An active device is asked before it is written off.
 *              Passive devices are never pinged.
 *
 *   jitter     Devices tend to be paired in batches and then go quiet
 *              together, so their timeouts expire together. Without jitter a
 *              mains outage would end with every router being pinged in the
 *              same 30-second sweep. Each check is spread over a random window
 *              on top of the timeout.
 *
 *   backoff    A device that stays away must not be re-pinged every timeout
 *              forever - that is airtime spent on something known to be gone.
 *              Each failed check widens the interval x1.5, x3, x6, x12 ...
 *              capped, and the ladder resets the moment the device speaks.
 ******************************************************************************/

#define ZB_AVAIL_ACTIVE_TIMEOUT_S   (10u * 60u)         /* mains / router     */
#define ZB_AVAIL_PASSIVE_TIMEOUT_S  (25u * 60u * 60u)   /* battery / sleepy   */
#define ZB_AVAIL_TICK_MS            (30u * 1000u)       /* sweep cadence      */

/* How long an active device gets to answer the ping before it is written off.
 * A device that is present answers a Basic read in seconds; this only has to
 * cover a retry and a busy network. */
#define ZB_AVAIL_PING_GRACE_S       60u

/* Largest random offset added to a check interval, in seconds. Matches
 * zigbee2mqtt's max_jitter (30000 ms). */
#define ZB_AVAIL_MAX_JITTER_S       30u

/* Backoff ladder, in eighths so the x1.5 step is exact in integer maths:
 * step 0 = x1, 1 = x1.5, 2 = x3, 3 = x6, 4 = x12, then held. Same progression
 * zigbee2mqtt uses (x1.5, x3, x6, x12 ...). */
static const uint8_t s_avail_backoff_eighths[] = { 8u, 12u, 24u, 48u, 96u };
#define ZB_AVAIL_BACKOFF_MAX_STEP   (ARRAY_SIZE(s_avail_backoff_eighths) - 1u)

/* Stop pinging once the backoff multiplier exceeds this many eighths, and wait
 * for the device to come back on its own. 0 disables the cutoff, which is
 * zigbee2mqtt's default (pause_on_backoff_gt = 0) - the ladder alone already
 * makes the traffic negligible. Set e.g. to 48u (x6) to go fully quiet. */
#define ZB_AVAIL_PAUSE_ABOVE_EIGHTHS 0u

/* Epoch seconds for 2020-09-13; anything below this means the RTC has not been
 * set yet, so ages computed from it would be nonsense. */
#define ZB_AVAIL_EPOCH_SANE         1600000000ULL

static uint32_t s_avail_next_ms = 0;
static uint32_t s_avail_cycle = 0;      /* re-rolls the jitter each sweep     */

/*
 * Per-device jitter in seconds, 0 .. ZB_AVAIL_MAX_JITTER_S.
 *
 * Deliberately a hash rather than a PRNG: the driver has no portable RNG, and
 * decorrelating devices from each other is the whole point - true randomness
 * per call is not required. Mixing in the sweep counter stops a device from
 * landing on the same offset every cycle.
 */
static uint32_t
availability_jitter_s(uint64_t ieee_addr)
{
    uint32_t h = (uint32_t)(ieee_addr ^ (ieee_addr >> 32));
    h ^= s_avail_cycle * 2654435761u;       /* Knuth's multiplicative hash */
    h ^= h >> 15;
    h *= 2246822519u;
    h ^= h >> 13;
    return h % (ZB_AVAIL_MAX_JITTER_S + 1u);
}

/* Interval before the next check of a device that is already failing: the base
 * timeout stretched by the backoff ladder. */
static uint32_t
availability_backoff_s(const s_zb_device_t *device, uint8_t step)
{
    const uint32_t base = zb_device_manager_availability_timeout_s(device);
    const uint8_t eighths = s_avail_backoff_eighths[
        (step < ARRAY_SIZE(s_avail_backoff_eighths)) ? step : ZB_AVAIL_BACKOFF_MAX_STEP];
    return (uint32_t)(((uint64_t)base * eighths) / 8u);
}

/*
 * Reset a device's availability bookkeeping. Called when it is heard from, so
 * a device that comes back does not inherit the backoff it had accumulated
 * while it was away.
 */
static void
availability_slot_reset(uint8_t pool_idx)
{
    s_avail[pool_idx].next_check_ms = 0;
    s_avail[pool_idx].backoff_step  = 0;
    s_avail[pool_idx].ping_pending  = 0;
    s_avail[pool_idx].paused        = 0;
}

/*
 * Poke a device that has gone quiet, the way zigbee2mqtt pings before
 * declaring an active device offline. Reading the Basic cluster settles the
 * question; any answer refreshes last_seen through the normal AF path.
 */
static void
availability_ping(s_zb_device_t *device)
{
    for (uint8_t f = 0; (f < device->function_count) && (f < ZB_MAX_FUNCTIONS); f++)
    {
        s_zb_function_t *func = &device->functions[f];
        if ((func->type != ZB_FUNC_BASIC_INFO) ||
            (func->ops == NULL) || (func->ops->on_command == NULL))
        {
            continue;
        }
        const s_zb_cmd_t cmd = { .type = ZB_CMD_READ_STATE };
        (void)func->ops->on_command(device, func, &cmd);
        ZB_LOGI(TAG, "0x%llx - availability ping sent", device->ieee_addr);
        return;
    }
    ZB_LOGW(TAG, "0x%llx - no Basic function to ping", device->ieee_addr);
}

uint32_t
zb_device_manager_availability_timeout_s(const s_zb_device_t *device)
{
    if (device == NULL)
    {
        return ZB_AVAIL_ACTIVE_TIMEOUT_S;
    }
    return ((device->device_type == ZB_DEVICE_TYPE_END_DEVICE) || device->battery_powered)
             ? ZB_AVAIL_PASSIVE_TIMEOUT_S
             : ZB_AVAIL_ACTIVE_TIMEOUT_S;
}

e_zb_device_status_t
zb_device_manager_status_from_last_seen(const s_zb_device_t *device, uint64_t last_seen)
{
    const uint64_t now_s = zb_get_utc_epoch_time();

    /* Never heard from, or no usable wall clock to measure against: say so
     * rather than guessing either way. */
    if ((last_seen == 0) || (now_s < ZB_AVAIL_EPOCH_SANE))
    {
        return ZB_DEVICE_STATUS_UNKNOWN;
    }
    /* Clock stepped backwards (first NTP sync after boot): the age is
     * meaningless, so leave the verdict to the next sweep. */
    if (now_s < last_seen)
    {
        return ZB_DEVICE_STATUS_UNKNOWN;
    }
    return ((now_s - last_seen) <= zb_device_manager_availability_timeout_s(device))
             ? ZB_DEVICE_STATUS_ONLINE
             : ZB_DEVICE_STATUS_OFFLINE;
}

void
zb_device_manager_availability_tick(uint32_t now_ms)
{
    if ((s_avail_next_ms != 0) && ((int32_t)(now_ms - s_avail_next_ms) < 0))
    {
        return;
    }
    s_avail_next_ms = now_ms + ZB_AVAIL_TICK_MS;
    s_avail_cycle++;

    const uint64_t now_s = zb_get_utc_epoch_time();
    if (now_s < ZB_AVAIL_EPOCH_SANE)
    {
        return;             /* no wall clock yet - nothing can be judged */
    }

    /* Devices to poke once the lock is released. Capped per sweep so a network
     * that all went quiet at once (a power cut) does not dump 256 reads into
     * the ZNP queue in one tick; the rest are picked up on later sweeps, and
     * the jitter means they were unlikely to land together anyway. */
    uint64_t ping_list[8];
    uint8_t  ping_count = 0;

    device_manager_lock();
    for (uint16_t i = 0; i < ZB_MAX_DEVICE; i++)
    {
        s_zb_device_t *device = s_device_manager.pool[i];
        if (device == NULL)
        {
            continue;
        }
        s_zb_avail_slot_t *slot = &s_avail[i];

        /* Never heard from at all: status stays UNKNOWN until it speaks. */
        if (device->last_seen == 0)
        {
            continue;
        }
        /* Backed all the way off: nothing more to do until a frame arrives,
         * which resets the slot from note_activity(). */
        if (slot->paused)
        {
            continue;
        }
        /* Not due yet - this is where both the jitter and the backoff spacing
         * actually take effect. */
        if ((slot->next_check_ms != 0) && ((int32_t)(now_ms - slot->next_check_ms) < 0))
        {
            continue;
        }
        if (now_s < device->last_seen)
        {
            continue;       /* clock stepped backwards */
        }

        const uint64_t age = now_s - device->last_seen;
        const uint32_t timeout = zb_device_manager_availability_timeout_s(device);
        const uint32_t jitter = availability_jitter_s(device->ieee_addr);

        if (age <= (uint64_t)timeout + jitter)
        {
            /* Alive and within its window. Re-arm for the moment the window
             * would close, so a quiet device is examined once per timeout
             * rather than on every 30 s sweep. */
            const uint64_t remain = ((uint64_t)timeout + jitter) - age;
            slot->next_check_ms = now_ms + (uint32_t)((remain + 1u) * 1000u);
            slot->backoff_step  = 0;
            slot->ping_pending  = 0;
            continue;
        }

        /* Battery/sleepy devices are never pinged - they would not answer a
         * read anyway. They are written off on the timeout alone. */
        const bool passive = ((device->device_type == ZB_DEVICE_TYPE_END_DEVICE) ||
                              device->battery_powered);
        if (!passive)
        {
            if (!slot->ping_pending)
            {
                /* Queue it: the read goes out over ZNP and must not be issued
                 * with the manager lock held, or every other task that wants a
                 * device blocks for the whole UART round-trip. The flag is only
                 * set once the ping is actually queued, so a device crowded out
                 * by the per-sweep cap is retried rather than written off
                 * without ever having been asked. */
                if (ping_count < (uint8_t)ARRAY_SIZE(ping_list))
                {
                    ping_list[ping_count++] = device->ieee_addr;
                    slot->ping_pending  = 1;
                    slot->next_check_ms = now_ms + (ZB_AVAIL_PING_GRACE_S * 1000u);
                }
                continue;                       /* verdict waits for the reply */
            }
            /* The ping went unanswered: the grace window is what next_check_ms
             * was set to, and we are past it. */
            slot->ping_pending = 0;
        }

        /* Widen the interval before the next attempt, then schedule it. */
        if (slot->backoff_step < ZB_AVAIL_BACKOFF_MAX_STEP)
        {
            slot->backoff_step++;
        }
        const uint32_t next_s = availability_backoff_s(device, slot->backoff_step) +
                                availability_jitter_s(device->ieee_addr);
        slot->next_check_ms = now_ms + (next_s * 1000u);

        if ((ZB_AVAIL_PAUSE_ABOVE_EIGHTHS != 0u) &&
            (s_avail_backoff_eighths[slot->backoff_step] > ZB_AVAIL_PAUSE_ABOVE_EIGHTHS))
        {
            slot->paused = 1;
            ZB_LOGI(TAG, "0x%llx - availability checks paused until it speaks",
                    device->ieee_addr);
        }

        if (device->status != ZB_DEVICE_STATUS_ONLINE)
        {
            continue;       /* already written off; only the schedule moved */
        }

        device->status = ZB_DEVICE_STATUS_OFFLINE;
        zb_device_state_cache_mark_dirty();
        ZB_LOGI(TAG, "0x%llx - offline: silent for %lu s (timeout %lu s, next check in %lu s)",
                device->ieee_addr, (unsigned long)age, (unsigned long)timeout,
                (unsigned long)next_s);

        s_zb_event_t ev = {0};
        ev.type = ZB_EVENT_DEVICE_AVAILABILITY;
        ev.ieee_addr = device->ieee_addr;
        ev.availability.online = false;
        ev.availability.silent_for_s = (uint32_t)age;
        if (s_event_notify_callback)
            s_event_notify_callback(&ev);
    }
    device_manager_unlock();

    /* Now that the lock is gone, send the pings. Same task as every other ZCL
     * transmission, so the device pointers stay valid for the lookup. */
    for (uint8_t p = 0; p < ping_count; p++)
    {
        s_zb_device_t *device = zb_device_manager_find_by_ieee(ping_list[p]);
        if (device != NULL)
        {
            availability_ping(device);
        }
    }
}

void
zb_device_manager_set_lqi(uint64_t ieee_addr, uint8_t lqi)
{
    device_manager_lock();
    s_zb_device_t *device = ieee_hash_table_get(ieee_addr);
    if (device != NULL && device->lqi != lqi)
    {
        device->lqi = lqi;  /* link quality only; last_seen / status untouched */
        zb_device_state_cache_mark_dirty();
    }
    device_manager_unlock();
}

void
zb_device_manager_set_parent(uint64_t child_ieee, uint16_t parent_nwk, uint64_t parent_ieee)
{
    if (child_ieee == 0)
    {
        return;
    }
    device_manager_lock();
    s_zb_device_t *child = ieee_hash_table_get(child_ieee);
    if (child != NULL)
    {
        /* parent_ieee is the persisted field; parent_nwk_addr is re-derived from
         * it on restore, so only an IEEE change warrants a flash write (avoids
         * wearing flash on every repeated Mgmt_Lqi response). The manager mutex
         * is recursive, so saving under the lock is safe. */
        bool parent_ieee_changed = (child->parent_ieee != parent_ieee);
        child->parent_nwk_addr = parent_nwk;
        child->parent_ieee     = parent_ieee;
        if (parent_ieee_changed)
        {
            zb_device_manager_save_device_file(child);
        }
    }
    device_manager_unlock();
}

zb_status_t
zb_device_manager_remove_device(uint64_t ieee_addr)
{
    device_manager_lock();
    s_zb_device_t *device = ieee_hash_table_get(ieee_addr);
    if (!device)
    {
        ZB_LOGE(TAG, "%s() - Device 0x%llx not found", __func__, ieee_addr);
        device_manager_unlock();
        return ZB_FAIL;
    }

    zb_zdo_send_mgmt_leave_req(device->nwk_addr, device->ieee_addr, false, false);
    device_manager_notify_device_event(device, ZB_EVENT_DEVICE_LEFT);
    zb_device_manager_delete_device_file(device->ieee_addr);
    device_manager_free_device(device);
    device_manager_unlock();
    return ZB_OK;
}

zb_status_t
zb_device_manager_update_device(s_zb_device_info_t *device_info)
{
    device_manager_lock();
    s_zb_device_t *device = zb_device_manager_find_by_ieee(device_info->ieee_addr);
    if (!device)
    {
        ZB_LOGE(TAG, "%s() - Device 0x%llx not found", __func__, device_info->ieee_addr);
        device_manager_unlock();
        return ZB_FAIL;
    }
    device_manager_unlock();
    zb_device_manager_on_device_rejoined(device, device_info->nwk_addr, device_info->parent_nwk_addr);
    return ZB_OK;
}

void
zb_device_manager_on_device_rejoined(s_zb_device_t *device, uint16_t nwk_addr, uint16_t parent_nwk_addr)
{
    if (device == NULL)
    {
        return;
    }

    device_manager_lock();
    device->nwk_addr        = nwk_addr;
    device->parent_nwk_addr = parent_nwk_addr;
    device->last_seen       = zb_get_utc_epoch_time();
    device->status          = ZB_DEVICE_STATUS_ONLINE;

    /* Ensure bind intent exists for any new sensor functions (first-join only
     * adds entries; existing BIND rows are kept so operator UNBIND persists). */
    // device_manager_auto_bind_sensor_functions(device);

    /* Re-apply persisted bind/report on the next zb_core_query_device_task pass. */
    device->config_applied       = false;
    device->config_apply_idx     = 0;
    device->config_bind_in_flight = false;

    ZB_LOGI(TAG, "0x%llx - rejoined at 0x%04x, %u bind/report intent(s) to re-apply",
            device->ieee_addr, nwk_addr, device->desired_config_count);

    if (zb_device_manager_save_device_file(device) != ZB_OK)
    {
        ZB_LOGW(TAG, "0x%llx - failed to persist nwk addr after rejoin", device->ieee_addr);
    }
    device_manager_unlock();
}

zb_status_t
zb_device_manager_remove_all_devices(void)
{
    device_manager_lock();
    for (int i = 0; i < ZB_MAX_DEVICE; i++)
    {
        if (s_device_manager.pool[i])
        {
            zb_device_manager_delete_device_file(s_device_manager.pool[i]->ieee_addr);
            device_manager_free_device(s_device_manager.pool[i]);
        }
    }
    ZB_LOGI(TAG, "Deleted all devices");
    device_manager_unlock();
    return ZB_OK;
}

/******************************************************************************
 * Lookup functions
 ******************************************************************************/

s_zb_device_t *
zb_device_manager_find_by_ieee(uint64_t ieee_addr)
{
    device_manager_lock();
    s_zb_device_t *device = ieee_hash_table_get(ieee_addr);
    device_manager_unlock();
    return device;
}

s_zb_device_t *
zb_device_manager_find_by_short_addr(uint16_t short_addr)
{
    device_manager_lock();
    for (int i = 0; i < ZB_MAX_DEVICE; i++)
    {
        if (s_device_manager.pool[i] && s_device_manager.pool[i]->nwk_addr == short_addr)
        {
            s_zb_device_t *device = s_device_manager.pool[i];
            device_manager_unlock();
            return device;
        }
    }
    device_manager_unlock();
    return NULL;
}

bool
zb_device_manager_get_ieee_by_short_addr(uint16_t short_addr, uint64_t *ieee_out)
{
    if (ieee_out == NULL)
    {
        return false;
    }

    device_manager_lock();
    for (int i = 0; i < ZB_MAX_DEVICE; i++)
    {
        if (s_device_manager.pool[i] && s_device_manager.pool[i]->nwk_addr == short_addr)
        {
            *ieee_out = s_device_manager.pool[i]->ieee_addr;
            device_manager_unlock();
            return true;
        }
    }
    device_manager_unlock();
    return false;
}

s_zb_device_t *
zb_device_manager_find_device_from_start_index(uint16_t *index)
{
    device_manager_lock();
    for (int i = *index; i < ZB_MAX_DEVICE; i++)
    {
        if (s_device_manager.pool[i])
        {
            *index = i;
            s_zb_device_t *device = s_device_manager.pool[i];
            device_manager_unlock();
            return device;
        }
    }
    device_manager_unlock();
    return NULL;
}

s_zb_function_t *
zb_device_manager_find_function(s_zb_device_t *device, zb_sensor_id_t sensor_id)
{
    device_manager_lock();
    ZB_LOGI(TAG, "0x%llx - %s() - Sensor ID: 0x%04x", device->ieee_addr, __func__, sensor_id);
    for (int i = 0; i < device->function_count; i++)
    {
        if (device->functions[i].sensor_id == sensor_id)
        {
            ZB_LOGI(TAG, "Found function: %s", device->functions[i].name);
            s_zb_function_t *func = &device->functions[i];
            device_manager_unlock();
            return func;
        }
    }
    device_manager_unlock();
    return NULL;
}

uint16_t
zb_device_manager_endpoint_color_caps(const s_zb_device_t *device, uint8_t endpoint_id)
{
    if (device == NULL)
    {
        return 0;
    }

    /* A model-specific override wins over whatever the endpoint reported: some
     * lights misreport this, and the value drives which colour command they
     * are sent. See s_zb_color_caps_quirk_t. */
    uint16_t caps = 0;
    if (zb_device_schema_color_caps_override(device->manufacturer, device->model, &caps))
    {
        return caps;
    }

    for (uint8_t i = 0; i < device->endpoint_count && i < ZB_MAX_ENDPOINTS; i++)
    {
        if (device->endpoints[i].endpoint_id == endpoint_id)
        {
            return device->endpoints[i].color_caps_valid ? device->endpoints[i].color_caps : 0;
        }
    }
    return 0;
}

uint8_t
zb_device_manager_find_endpoint_with_cluster(const s_zb_device_t *device, uint16_t cluster_id)
{
    if (device == NULL)
    {
        return 0;
    }
    for (uint8_t i = 0; i < device->endpoint_count && i < ZB_MAX_ENDPOINTS; i++)
    {
        const s_zb_device_endpoint_t *ep = &device->endpoints[i];
        for (uint8_t j = 0; j < ep->in_cluster_count && j < ZB_MAX_IN_CLUSTERS; j++)
        {
            if (ep->in_clusters[j] == cluster_id)
            {
                return ep->endpoint_id;
            }
        }
    }
    return 0;
}

bool
zb_device_manager_endpoint_color_temp_range(const s_zb_device_t *device, uint8_t endpoint_id,
                                            uint16_t *min_out, uint16_t *max_out)
{
    if ((device == NULL) || (min_out == NULL) || (max_out == NULL))
    {
        return false;
    }

    for (uint8_t i = 0; i < device->endpoint_count && i < ZB_MAX_ENDPOINTS; i++)
    {
        if (device->endpoints[i].endpoint_id != endpoint_id)
        {
            continue;
        }
        uint16_t lo = device->endpoints[i].color_temp_min;
        uint16_t hi = device->endpoints[i].color_temp_max;
        /* Both zero means never reported. A single zero, or an inverted pair,
         * means the light answered with something unusable. Fall back to the
         * schema table rather than clamping against nonsense - and note the
         * fallback carries the device's own span, NOT a "corrected" one: a
         * bulb reporting {0, 1000} is speaking its own 0..1000 scale and acts
         * on it, so narrowing the range to textbook mireds would throw away
         * half of the output the light can actually produce. */
        if ((lo == 0) || (hi == 0) || (lo >= hi))
        {
            return zb_device_schema_color_temp_range_override(device->manufacturer,
                                                              device->model,
                                                              min_out, max_out);
        }
        *min_out = lo;
        *max_out = hi;
        return true;
    }
    return false;
}

s_zb_device_endpoint_t *
zb_device_manager_find_endpoint_by_dev_cluster(s_zb_device_info_t *device_info, uint16_t cluster_id)
{
    for (int i = 0; i < device_info->endpoint_count; i++)
    {
        for (int j = 0; j < device_info->endpoints[i].in_cluster_count; j++)
        {
            if (device_info->endpoints[i].in_clusters[j] == cluster_id)
                return &device_info->endpoints[i];
        }
    }
    return NULL;
}