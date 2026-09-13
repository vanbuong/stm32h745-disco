#include "device/zb_device_schema.h"

#include "device/generic/zb_device_basic_info.h"
#include "device/generic/zb_device_battery_sensor.h"
#include "device/generic/zb_device_on_off_switch.h"
#include "device/generic/zb_device_dimmer_switch.h"
#include "device/generic/zb_device_button.h"
#include "device/generic/zb_device_on_off_smart_plug.h"
#include "device/generic/zb_device_mains_power_outlet.h"
#include "device/generic/zb_device_relay.h"
#include "device/measurement/zb_device_pressure_sensor.h"
#include "device/measurement/zb_device_flow_sensor.h"
#include "device/measurement/zb_device_temperature_sensor.h"
#include "device/measurement/zb_device_humidity_sensor.h"
#include "device/measurement/zb_device_illuminance_sensor.h"
#include "device/measurement/zb_device_pm25_sensor.h"
#include "device/measurement/zb_device_co2_sensor.h"
#include "device/measurement/zb_device_pm10_sensor.h"
#include "device/measurement/zb_device_pm1_sensor.h"
#include "device/measurement/zb_device_tvoc_sensor.h"
#include "device/measurement/zb_device_develco_voc_sensor.h"
#include "device/measurement/zb_device_formaldehyde_sensor.h"
#include "device/measurement/zb_device_iaq_sensor.h"
#include "device/measurement/zb_device_eco2_sensor.h"
#include "device/measurement/zb_device_occupancy_sensor.h"
#include "device/measurement/zb_device_electrical_measurement.h"
#include "device/measurement/zb_device_energy_meter.h"
#include "device/lighting/zb_device_color_light.h"
#include "device/lighting/zb_device_color_temp_light.h"
#include "device/lighting/zb_device_dimmable_light.h"
#include "device/lighting/zb_device_on_off_light.h"
#include "device/lighting/zb_device_extended_color_light.h"
#include "device/intruder/zb_device_ias_zone.h"
#include "device/intruder/zb_device_ias_warning.h"
#include "zcl/zb_zcl.h"
#include "zcl/zb_zcl_lighting.h"
#include "zcl/zb_zcl_general.h"
#include "zcl/zb_zcl_ms.h"
#include "zcl/zb_zcl_ss.h"
#include "manu/zb_manu.h"

#include <string.h>

#define TAG "ZB_DEV_SCHEMA"

/******************************************************************************
 * Ignored clusters - never produce functions
 ******************************************************************************/

static const uint16_t s_ignored_clusters[] = {
    ZCL_CLUSTER_ID_GENERAL_IDENTIFY,
    ZCL_CLUSTER_ID_GENERAL_GROUPS,
    ZCL_CLUSTER_ID_GENERAL_SCENES,
    ZCL_CLUSTER_ID_OTA,
    ZCL_CLUSTER_ID_TOUCHLINK,
};

/******************************************************************************
 * Device Quirk Register
 * Order: exact manufacturer and model match
 ******************************************************************************/

/*
 * Poll-interval overrides. A function's state-sync cadence defaults to its
 * function type (poll_default_for_type) but a device whose reporting is
 * unconfigured/unsupported can be forced to poll here. Match order: exact
 * manufacturer/model, optionally narrowed to a cluster (0xFFFF = any).
 */
typedef struct s_zb_poll_quirk
{
    const char *manufacturer;   // exact ManufacturerName, NULL = any
    const char *model;          // exact ModelIdentifier,  NULL = any
    uint16_t    cluster_id;     // 0xFFFF = any cluster
    uint32_t    poll_interval_ms;
} s_zb_poll_quirk_t;

static const s_zb_poll_quirk_t s_poll_quirks[] = {
    /* Tuya TS011F (_TZ3000_wzmuk9ai): reporting is commonly not configured,
     * so poll the live electrical/metering values instead of waiting for reports. */
    // { "_TZ3000_wzmuk9ai", "TS011F", ZCL_CLUSTER_ID_MS_ELECTRICAL_MEASUREMENT, 30000 },
    // { "_TZ3000_wzmuk9ai", "TS011F", ZCL_CLUSTER_ID_SE_METERING,               60000 },
    /* frient/Develco AQSZB-110: VOC lives on the manufacturer-specific cluster
     * 0xFC03 whose reporting can be unreliable, so poll the MeasuredValue. */
    // { "frient A/S", "AQSZB-110", ZCL_CLUSTER_ID_MS_DEVELCO_VOC, 60000 },
};

static uint32_t
poll_default_for_type(e_zb_function_type_t type)
{
    switch (type)
    {
        /* Actuators with readable state: read once on first sync, then rely on
         * the device's own reports. Without this initial read the hub has no
         * idea whether a light is on until someone touches it - the state a
         * report would correct is never established in the first place. This
         * is what zigbee2mqtt's light()/onOff() extends do at configure time
         * (setupAttributes(..., read = true)). 0 = read once, see
         * zb_core_func_due(). */
        // case ZB_FUNC_ONOFF_LIGHT:
        // case ZB_FUNC_ONOFF_PLUGIN_UNIT:
        // case ZB_FUNC_DIMMABLE_LIGHT:
        // case ZB_FUNC_DIMMABLE_PLUGIN_UNIT:
        // case ZB_FUNC_COLOR_TEMP_LIGHT:
        // case ZB_FUNC_COLOR_LIGHT:
        // case ZB_FUNC_EXTENDED_COLOR_LIGHT:
        // case ZB_FUNC_ONOFF_SMART_PLUG:
        // case ZB_FUNC_RELAY:
        // case ZB_FUNC_MAINS_POWER_OUTLET: return 0;
        case ZB_FUNC_ELECTRICAL:  return 60000; /* 60s for live electrical values */
        case ZB_FUNC_ENERGY:      return 300000; /* 5min for live energy values */
        default:                  return ZB_POLL_NEVER; /* rely on the device's reports */
    }
}

uint32_t
zb_device_schema_resolve_poll_ms(const char *manufacturer, const char *model,
                                 uint16_t cluster_id, e_zb_function_type_t type)
{
    for (uint16_t i = 0; i < (uint16_t)ARRAY_SIZE(s_poll_quirks); i++)
    {
        const s_zb_poll_quirk_t *q = &s_poll_quirks[i];
        if (q->manufacturer && strcmp(q->manufacturer, manufacturer) != 0)
            continue;
        if (q->model && strcmp(q->model, model) != 0)
            continue;
        if (q->cluster_id != 0xFFFF && q->cluster_id != cluster_id)
            continue;
        return q->poll_interval_ms;
    }
    return poll_default_for_type(type);
}

/*
 * Cluster-gating quirks. A cluster listed here is manufacturer/model-specific:
 * it produces a function ONLY on a device whose manufacturer/model matches one
 * of its entries. This is used for manufacturer-specific clusters that are
 * reused by multiple vendors for unrelated purposes. Clusters not listed here
 * are unrestricted. Match fields: exact string, NULL = any.
 */
typedef struct s_zb_cluster_gate_quirk
{
    uint16_t    cluster_id;
    const char *manufacturer;   // exact ManufacturerName, NULL = any
    const char *model;          // exact ModelIdentifier,  NULL = any
} s_zb_cluster_gate_quirk_t;

static const s_zb_cluster_gate_quirk_t s_cluster_gate_quirks[] = {
    /* Develco/frient VOC (0xFC03) is manufacturer-specific and reused by other
     * vendors (e.g. Philips Hue), so only expose it on Develco/frient devices. */
    { ZCL_CLUSTER_ID_MS_DEVELCO_VOC, "frient A/S",           NULL },
    { ZCL_CLUSTER_ID_MS_DEVELCO_VOC, "Develco Products A/S", NULL },
};

/*
 * Synthetic clusters. See s_zb_synthetic_cluster_quirk_t in the header: these
 * are clusters a device actually uses but does not advertise in its Simple
 * Descriptor, so no function would otherwise be built for them.
 */
static const s_zb_synthetic_cluster_quirk_t s_synthetic_cluster_quirks[] = {
    /* Tuya TS0601 temperature/humidity sensor with LCD clock
     * (_TZE284_vvmbj46n, sold as Nous E6 and relabels). It advertises only the
     * Tuya MCU clusters (0xEF00, plus 0xED00/0xE000 which carry nothing we
     * want) and implements no measurement clusters at all - every reading
     * arrives as a datapoint record that zb_manu_tuya re-emits as a standard
     * report. These three entries give those reports a function to land in.
     * See s_tuya_dp_maps below for the datapoint numbering. */
    { "_TZE284_vvmbj46n", "TS0601", 1, ZCL_CLUSTER_ID_MS_TEMPERATURE_MEASUREMENT, 0 },
    { "_TZE284_vvmbj46n", "TS0601", 1, ZCL_CLUSTER_ID_MS_RELATIVE_HUMIDITY,       0 },
    { "_TZE284_vvmbj46n", "TS0601", 1, ZCL_CLUSTER_ID_GENERAL_POWER_CONFIG,       0 },
    /* Lumi/Aqara water leak sensor (SJCGQ11LM). It sends IAS Zone Status Change
     * Notifications on endpoint 1 but does not list cluster 0x0500 in its
     * in-cluster list, and never runs IAS enrollment. Without this the
     * notification lands in zb_zcl_ss_zone_change_noti_callback() and is
     * dropped with "function not found". Zone type is assumed rather than read,
     * because the device has no IAS Zone attributes to read it from. */
    { "LUMI", "lumi.sensor_wleak.aq1", 1, ZCL_CLUSTER_ID_SS_IAS_ZONE, SS_IAS_ZONE_TYPE_WATER_SENSOR },
    /* Same device: battery level is only ever reported inside the proprietary
     * Basic-cluster TLV (tag 0x01), which zb_manu_lumi_handler decodes
     * and re-injects as a standard PowerConfiguration BatteryVoltage report.
     * The device does not advertise cluster 0x0001, so without this entry the
     * injected value would have no battery function to land in. */
    { "LUMI", "lumi.sensor_wleak.aq1", 1, ZCL_CLUSTER_ID_GENERAL_POWER_CONFIG, 0 },
    /* Lumi/Aqara wireless mini switch (WXKG11LM, "lumi.remote.b1acn01"). Same
     * proprietary battery reporting: the level only ever arrives in the Basic
     * TLV (re-injected as a PowerConfiguration BatteryVoltage report) and the
     * device does not advertise cluster 0x0001. Without this the injected value
     * would have no battery function to land in. */
    { "LUMI", "lumi.remote.b1acn01", 1, ZCL_CLUSTER_ID_GENERAL_POWER_CONFIG, 0 },
    /* Aqara motion sensor P1 (RTCGQ14LM, "lumi.motion.ac02"). Battery is only
     * ever reported inside the proprietary 0x00F7 TLV heartbeat on the Lumi
     * 0xFCC0 cluster, which zb_manu_lumi_handler decodes and
     * re-injects as a standard PowerConfiguration BatteryVoltage report. The
     * device does not advertise cluster 0x0001, so without this entry the
     * injected value would have no battery function to land in. */
    { "LUMI", "lumi.motion.ac02", 1, ZCL_CLUSTER_ID_GENERAL_POWER_CONFIG, 0 },
    /* Same device: it reports motion on the standard Occupancy cluster once the
     * init-write below has switched it out of proprietary mode, but 0x0406 is
     * missing from its Simple Descriptor, so the interview never builds an
     * occupancy function for it to land in. */
    { "LUMI", "lumi.motion.ac02", 1, ZCL_CLUSTER_ID_MS_OCCUPANCY_SENSING, 0 },
    /* Same device: the same 0xFCC0 report also carries the illuminance, which
     * is re-injected as a standard IlluminanceMeasurement report. Cluster
     * 0x0400 is not advertised either. */
    { "LUMI", "lumi.motion.ac02", 1, ZCL_CLUSTER_ID_MS_ILLUMINANCE_MEASUREMENT, 0 },
};

/*
 * Occupancy auto-clear. Aqara motion sensors report a detection and then go
 * quiet - they never send occupancy=0 - so the host has to age the motion out
 * or it latches on forever. The value is the fallback used until the device
 * tells us its own detection interval (Lumi attr 0x0069, re-injected as the
 * standard PIROccupiedToUnoccupiedDelay); it matches what
 * zigbee-herdsman-converters uses: the model default plus 2 s of slack.
 *
 * Anything not listed here gets 0 = feature off, so a standards-compliant
 * sensor that holds occupancy for minutes is never cut short.
 */
static const struct {
    const char *manufacturer;
    const char *model;
    uint32_t    clear_ms;
} s_occupancy_clear_quirks[] = {
    /* Aqara motion sensor P1 (RTCGQ14LM): 30 s device default + 2 s. */
    { "LUMI", "lumi.motion.ac02", (ZB_LUMI_DETECTION_INTERVAL_DEFAULT_S + 2u) * 1000u },
};

uint32_t
zb_device_schema_resolve_occupancy_clear_ms(const char *manufacturer, const char *model)
{
    if (manufacturer == NULL || model == NULL)
    {
        return 0;
    }
    for (uint16_t i = 0; i < (uint16_t)ARRAY_SIZE(s_occupancy_clear_quirks); i++)
    {
        if (strcmp(s_occupancy_clear_quirks[i].manufacturer, manufacturer) == 0 &&
            strcmp(s_occupancy_clear_quirks[i].model, model) == 0)
        {
            return s_occupancy_clear_quirks[i].clear_ms;
        }
    }
    return 0;
}

const s_zb_synthetic_cluster_quirk_t *
zb_device_schema_synthetic_clusters(uint16_t *count)
{
    if (count)
    {
        *count = (uint16_t)ARRAY_SIZE(s_synthetic_cluster_quirks);
    }
    return s_synthetic_cluster_quirks;
}

/*
 * Capability quirks. See s_zb_device_caps_quirk_t in the header: identity and
 * MAC-capability values for devices that do not answer the Node Descriptor.
 */
static const s_zb_device_caps_quirk_t s_caps_quirks[] = {
    /* frient/Develco AQSZB-110 air-quality sensor. It does not respond to the
     * ZDO Node Descriptor request, so manu_id/device_type/battery/sleepy are
     * unknown after the interview. It is a battery-powered end device; it
     * answers unicast ZCL reads (Basic identity is read successfully), so it is
     * treated as non-sleepy. Adjust device_type/sleepy if a capture shows
     * otherwise. */
    { "frient A/S", "AQSZB-110", ZB_MANUFACTURER_CODE_DEVELCO,
      ZB_DEVICE_TYPE_END_DEVICE, /*battery_powered*/ true, /*sleepy_enabled*/ true },
};

const s_zb_device_caps_quirk_t *
zb_device_schema_caps_find(const char *manufacturer, const char *model)
{
    if (manufacturer == NULL || model == NULL)
    {
        return NULL;
    }
    for (uint16_t i = 0; i < (uint16_t)ARRAY_SIZE(s_caps_quirks); i++)
    {
        const s_zb_device_caps_quirk_t *q = &s_caps_quirks[i];
        if (strcmp(q->manufacturer, manufacturer) == 0 && strcmp(q->model, model) == 0)
        {
            return q;
        }
    }
    return NULL;
}

bool
zb_device_schema_cluster_allowed(uint16_t cluster_id, const char *manufacturer, const char *model)
{
    bool gated = false;
    for (uint16_t i = 0; i < (uint16_t)ARRAY_SIZE(s_cluster_gate_quirks); i++)
    {
        const s_zb_cluster_gate_quirk_t *q = &s_cluster_gate_quirks[i];
        if (q->cluster_id != cluster_id)
            continue;
        gated = true; /* cluster is restricted; require a matching entry */
        if (q->manufacturer && (manufacturer == NULL || strcmp(q->manufacturer, manufacturer) != 0))
            continue;
        if (q->model && (model == NULL || strcmp(q->model, model) != 0))
            continue;
        return true;
    }
    /* Restricted clusters with no matching entry are denied; all others pass. */
    return !gated;
}

/******************************************************************************
 * Cluster Schema Registry
 * Order: exact deviceId matches before wildcard (0xFFFF) fallbacks
 ******************************************************************************/

static s_zb_device_cluster_schema_t s_device_cluster_schemas[] = {

    // Basic Info
    { ZCL_CLUSTER_ID_GENERAL_BASIC,  ZCL_HA_PROFILE_ID, ZCL_DEVICEID_NONE,                     ZB_FUNC_BASIC_INFO,          "basic info",       ZB_LIGHT_COLOR_CAP_NONE, &zb_device_basic_info_ops },
    // - Cluster: On/Off (0x0006) - deviceId disambiguates type ----------------
    // Light
    { ZCL_CLUSTER_ID_GENERAL_ON_OFF, ZCL_HA_PROFILE_ID, ZCL_DEVICEID_ON_OFF_LIGHT,             ZB_FUNC_ONOFF_LIGHT,         "onoff light",      ZB_LIGHT_COLOR_CAP_NONE, &zb_device_on_off_light_ops },
    { ZCL_CLUSTER_ID_GENERAL_ON_OFF, ZCL_HA_PROFILE_ID, ZCL_DEVICEID_ON_OFF_PLUG_IN_UNIT,      ZB_FUNC_ONOFF_PLUGIN_UNIT,   "onoff plugin",     ZB_LIGHT_COLOR_CAP_NONE, &zb_device_on_off_light_ops },
    { ZCL_CLUSTER_ID_GENERAL_ON_OFF, ZCL_LL_PROFILE_ID, ZCL_DEVICEID_ZLL_ON_OFF_LIGHT,         ZB_FUNC_ONOFF_LIGHT,         "onoff light",      ZB_LIGHT_COLOR_CAP_NONE, &zb_device_on_off_light_ops },
    { ZCL_CLUSTER_ID_GENERAL_ON_OFF, ZCL_LL_PROFILE_ID, ZCL_DEVICEID_ZLL_ON_OFF_PLUGIN_UNIT,   ZB_FUNC_ONOFF_PLUGIN_UNIT,   "onoff plugin",     ZB_LIGHT_COLOR_CAP_NONE, &zb_device_on_off_light_ops },
    { ZCL_CLUSTER_ID_GENERAL_ON_OFF, ZCL_HA_PROFILE_ID, ZCL_DEVICEID_DIMMABLE_LIGHT,           ZB_FUNC_ONOFF_LIGHT,         "onoff light",      ZB_LIGHT_COLOR_CAP_NONE, &zb_device_on_off_light_ops },
    { ZCL_CLUSTER_ID_GENERAL_ON_OFF, ZCL_HA_PROFILE_ID, ZCL_DEVICEID_DIMMABLE_PLUG_IN_UNIT,    ZB_FUNC_ONOFF_PLUGIN_UNIT,   "onoff plugin",     ZB_LIGHT_COLOR_CAP_NONE, &zb_device_on_off_light_ops },
    { ZCL_CLUSTER_ID_GENERAL_ON_OFF, ZCL_LL_PROFILE_ID, ZCL_DEVICEID_ZLL_DIMMABLE_LIGHT,       ZB_FUNC_ONOFF_LIGHT,         "onoff light",      ZB_LIGHT_COLOR_CAP_NONE, &zb_device_on_off_light_ops },
    { ZCL_CLUSTER_ID_GENERAL_ON_OFF, ZCL_LL_PROFILE_ID, ZCL_DEVICEID_ZLL_DIMMABLE_PLUGIN_UNIT, ZB_FUNC_ONOFF_PLUGIN_UNIT,   "onoff plugin",     ZB_LIGHT_COLOR_CAP_NONE, &zb_device_on_off_light_ops },
    { ZCL_CLUSTER_ID_GENERAL_ON_OFF, ZCL_HA_PROFILE_ID, ZCL_DEVICEID_MANU_IKEA_LIGHT,          ZB_FUNC_ONOFF_LIGHT,         "onoff light",      ZB_LIGHT_COLOR_CAP_NONE, &zb_device_on_off_light_ops },
    { ZCL_CLUSTER_ID_GENERAL_ON_OFF, ZCL_HA_PROFILE_ID, ZCL_DEVICEID_COLOR_DIMMABLE_LIGHT,     ZB_FUNC_ONOFF_LIGHT,         "onoff light",      ZB_LIGHT_COLOR_CAP_NONE, &zb_device_on_off_light_ops },
    { ZCL_CLUSTER_ID_GENERAL_ON_OFF, ZCL_HA_PROFILE_ID, ZCL_DEVICEID_COLOR_TEMPERATURE_LIGHT,  ZB_FUNC_ONOFF_LIGHT,         "onoff light",      ZB_LIGHT_COLOR_CAP_NONE, &zb_device_on_off_light_ops },
    { ZCL_CLUSTER_ID_GENERAL_ON_OFF, ZCL_HA_PROFILE_ID, ZCL_DEVICEID_EXTENDED_COLOR_LIGHT,     ZB_FUNC_ONOFF_LIGHT,         "onoff light",      ZB_LIGHT_COLOR_CAP_NONE, &zb_device_on_off_light_ops },
    { ZCL_CLUSTER_ID_GENERAL_ON_OFF, ZCL_LL_PROFILE_ID, ZCL_DEVICEID_ZLL_COLOR_LIGHT,          ZB_FUNC_ONOFF_LIGHT,         "onoff light",      ZB_LIGHT_COLOR_CAP_NONE, &zb_device_on_off_light_ops },
    { ZCL_CLUSTER_ID_GENERAL_ON_OFF, ZCL_LL_PROFILE_ID, ZCL_DEVICEID_ZLL_EXTENDED_COLOR_LIGHT, ZB_FUNC_ONOFF_LIGHT,         "onoff light",      ZB_LIGHT_COLOR_CAP_NONE, &zb_device_on_off_light_ops },
    { ZCL_CLUSTER_ID_GENERAL_ON_OFF, ZCL_LL_PROFILE_ID, ZCL_DEVICEID_ZLL_COLOR_TEMPERATURE_LIGHT, ZB_FUNC_ONOFF_LIGHT,      "onoff light",      ZB_LIGHT_COLOR_CAP_NONE, &zb_device_on_off_light_ops },

    // - Cluster: Multistate Input (0x0012) - momentary button / scene controller --------
    { ZCL_CLUSTER_ID_GENERAL_MULTISTATE_INPUT_BASIC, ZCL_HA_PROFILE_ID, ZCL_DEVICEID_NONE,      ZB_FUNC_BUTTON,              "button",           ZB_LIGHT_COLOR_CAP_NONE, &zb_device_button_ops },

    // Switch
    { ZCL_CLUSTER_ID_GENERAL_ON_OFF, ZCL_HA_PROFILE_ID, ZCL_DEVICEID_ON_OFF_SWITCH,            ZB_FUNC_ONOFF_SWITCH,        "onoff switch",     ZB_LIGHT_COLOR_CAP_NONE, &zb_device_on_off_switch_ops },
    { ZCL_CLUSTER_ID_GENERAL_ON_OFF, ZCL_HA_PROFILE_ID, ZCL_DEVICEID_ON_OFF_LIGHT_SWITCH,      ZB_FUNC_ONOFF_SWITCH,        "onoff switch",     ZB_LIGHT_COLOR_CAP_NONE, &zb_device_on_off_switch_ops },
    { ZCL_CLUSTER_ID_GENERAL_ON_OFF, ZCL_HA_PROFILE_ID, ZCL_DEVICEID_DIMMER_SWITCH,            ZB_FUNC_ONOFF_SWITCH,        "onoff switch",     ZB_LIGHT_COLOR_CAP_NONE, &zb_device_on_off_switch_ops },
    { ZCL_CLUSTER_ID_GENERAL_ON_OFF, ZCL_HA_PROFILE_ID, ZCL_DEVICEID_LEVEL_CONTROL_SWITCH,     ZB_FUNC_ONOFF_SWITCH,        "onoff switch",     ZB_LIGHT_COLOR_CAP_NONE, &zb_device_on_off_switch_ops },

    // Outputs / plugs / relays
    { ZCL_CLUSTER_ID_GENERAL_ON_OFF, ZCL_HA_PROFILE_ID, ZCL_DEVICEID_ON_OFF_OUTPUT,            ZB_FUNC_RELAY,               "relay",           ZB_LIGHT_COLOR_CAP_NONE, &zb_device_relay_ops },
    { ZCL_CLUSTER_ID_GENERAL_ON_OFF, ZCL_HA_PROFILE_ID, ZCL_DEVICEID_LEVEL_CONTROLLABLE_OUTPUT, ZB_FUNC_RELAY,              "relay",           ZB_LIGHT_COLOR_CAP_NONE, &zb_device_relay_ops },
    { ZCL_CLUSTER_ID_GENERAL_ON_OFF, ZCL_HA_PROFILE_ID, ZCL_DEVICEID_SMART_PLUG,               ZB_FUNC_ONOFF_SMART_PLUG,    "smart plug",      ZB_LIGHT_COLOR_CAP_NONE, &zb_device_on_off_smart_plug_ops },
    { ZCL_CLUSTER_ID_GENERAL_ON_OFF, ZCL_HA_PROFILE_ID, ZCL_DEVICEID_MAINS_POWER_OUTLET,       ZB_FUNC_MAINS_POWER_OUTLET,  "mains outlet",    ZB_LIGHT_COLOR_CAP_NONE, &zb_device_mains_power_outlet_ops },
    { ZCL_CLUSTER_ID_GENERAL_ON_OFF, ZCL_HA_PROFILE_ID, ZCL_DEVICEID_NONE,                     ZB_FUNC_RELAY,               "relay",           ZB_LIGHT_COLOR_CAP_NONE, &zb_device_relay_ops },

    // - Cluster: Level Control (0x0008) - deviceId disambiguates type ----------------
    { ZCL_CLUSTER_ID_GENERAL_LEVEL_CONTROL, ZCL_HA_PROFILE_ID, ZCL_DEVICEID_DIMMABLE_LIGHT,             ZB_FUNC_DIMMABLE_LIGHT,         "dimmable light",   ZB_LIGHT_COLOR_CAP_NONE, &zb_device_dimmable_light_ops },
    { ZCL_CLUSTER_ID_GENERAL_LEVEL_CONTROL, ZCL_HA_PROFILE_ID, ZCL_DEVICEID_DIMMABLE_PLUG_IN_UNIT,      ZB_FUNC_DIMMABLE_PLUGIN_UNIT,   "dimmable plugin",  ZB_LIGHT_COLOR_CAP_NONE, &zb_device_dimmable_light_ops },
    { ZCL_CLUSTER_ID_GENERAL_LEVEL_CONTROL, ZCL_HA_PROFILE_ID, ZCL_DEVICEID_COLOR_DIMMABLE_LIGHT,       ZB_FUNC_DIMMABLE_LIGHT,         "dimmable light",   ZB_LIGHT_COLOR_CAP_NONE, &zb_device_dimmable_light_ops },
    { ZCL_CLUSTER_ID_GENERAL_LEVEL_CONTROL, ZCL_HA_PROFILE_ID, ZCL_DEVICEID_COLOR_TEMPERATURE_LIGHT,    ZB_FUNC_DIMMABLE_LIGHT,         "dimmable light",   ZB_LIGHT_COLOR_CAP_NONE, &zb_device_dimmable_light_ops },
    { ZCL_CLUSTER_ID_GENERAL_LEVEL_CONTROL, ZCL_HA_PROFILE_ID, ZCL_DEVICEID_EXTENDED_COLOR_LIGHT,       ZB_FUNC_DIMMABLE_LIGHT,         "dimmable light",   ZB_LIGHT_COLOR_CAP_NONE, &zb_device_dimmable_light_ops },
    { ZCL_CLUSTER_ID_GENERAL_LEVEL_CONTROL, ZCL_LL_PROFILE_ID, ZCL_DEVICEID_ZLL_DIMMABLE_LIGHT,         ZB_FUNC_DIMMABLE_LIGHT,         "dimmable light",   ZB_LIGHT_COLOR_CAP_NONE, &zb_device_dimmable_light_ops },
    { ZCL_CLUSTER_ID_GENERAL_LEVEL_CONTROL, ZCL_LL_PROFILE_ID, ZCL_DEVICEID_ZLL_DIMMABLE_PLUGIN_UNIT,   ZB_FUNC_DIMMABLE_PLUGIN_UNIT,   "dimmable plugin",  ZB_LIGHT_COLOR_CAP_NONE, &zb_device_dimmable_light_ops },
    { ZCL_CLUSTER_ID_GENERAL_LEVEL_CONTROL, ZCL_LL_PROFILE_ID, ZCL_DEVICEID_ZLL_COLOR_LIGHT,            ZB_FUNC_DIMMABLE_LIGHT,         "dimmable light",   ZB_LIGHT_COLOR_CAP_NONE, &zb_device_dimmable_light_ops },
    { ZCL_CLUSTER_ID_GENERAL_LEVEL_CONTROL, ZCL_LL_PROFILE_ID, ZCL_DEVICEID_ZLL_EXTENDED_COLOR_LIGHT,   ZB_FUNC_DIMMABLE_LIGHT,         "dimmable light",   ZB_LIGHT_COLOR_CAP_NONE, &zb_device_dimmable_light_ops },
    { ZCL_CLUSTER_ID_GENERAL_LEVEL_CONTROL, ZCL_LL_PROFILE_ID, ZCL_DEVICEID_ZLL_COLOR_TEMPERATURE_LIGHT, ZB_FUNC_DIMMABLE_LIGHT,        "dimmable light",   ZB_LIGHT_COLOR_CAP_NONE, &zb_device_dimmable_light_ops },
    // Dimmable switch
    { ZCL_CLUSTER_ID_GENERAL_LEVEL_CONTROL, ZCL_HA_PROFILE_ID, ZCL_DEVICEID_DIMMER_SWITCH,              ZB_FUNC_DIMMER_SWITCH,          "dimmer switch",    ZB_LIGHT_COLOR_CAP_NONE, &zb_device_dimmer_switch_ops },
    { ZCL_CLUSTER_ID_GENERAL_LEVEL_CONTROL, ZCL_HA_PROFILE_ID, ZCL_DEVICEID_LEVEL_CONTROL_SWITCH,       ZB_FUNC_DIMMER_SWITCH,          "dimmer switch",    ZB_LIGHT_COLOR_CAP_NONE, &zb_device_dimmer_switch_ops },

    // - Cluster: Color Control (0x0003) - deviceId disambiguates type ----------------
    { ZCL_CLUSTER_ID_LIGHTING_COLOR_CONTROL, ZCL_HA_PROFILE_ID, ZCL_DEVICEID_COLOR_DIMMABLE_LIGHT,      ZB_FUNC_COLOR_LIGHT,            "color light",      ZB_LIGHT_COLOR_CAP_HUE_SAT, &zb_device_color_light_ops },
    { ZCL_CLUSTER_ID_LIGHTING_COLOR_CONTROL, ZCL_HA_PROFILE_ID, ZCL_DEVICEID_COLOR_TEMPERATURE_LIGHT,   ZB_FUNC_COLOR_TEMP_LIGHT,       "color temp light", ZB_LIGHT_COLOR_CAP_TEMP, &zb_device_color_temp_light_ops },
    { ZCL_CLUSTER_ID_LIGHTING_COLOR_CONTROL, ZCL_HA_PROFILE_ID, ZCL_DEVICEID_EXTENDED_COLOR_LIGHT,      ZB_FUNC_EXTENDED_COLOR_LIGHT,   "extended color light", ZB_LIGHT_COLOR_CAP_ALL, &zb_device_extended_color_light_ops },
    { ZCL_CLUSTER_ID_LIGHTING_COLOR_CONTROL, ZCL_LL_PROFILE_ID, ZCL_DEVICEID_ZLL_COLOR_LIGHT,           ZB_FUNC_COLOR_LIGHT,            "color light",      ZB_LIGHT_COLOR_CAP_HUE_SAT, &zb_device_color_light_ops },
    { ZCL_CLUSTER_ID_LIGHTING_COLOR_CONTROL, ZCL_LL_PROFILE_ID, ZCL_DEVICEID_ZLL_COLOR_TEMPERATURE_LIGHT, ZB_FUNC_COLOR_TEMP_LIGHT,     "color temp light", ZB_LIGHT_COLOR_CAP_TEMP, &zb_device_color_temp_light_ops },
    { ZCL_CLUSTER_ID_LIGHTING_COLOR_CONTROL, ZCL_LL_PROFILE_ID, ZCL_DEVICEID_ZLL_EXTENDED_COLOR_LIGHT,  ZB_FUNC_EXTENDED_COLOR_LIGHT,   "extended color light", ZB_LIGHT_COLOR_CAP_ALL, &zb_device_extended_color_light_ops },

    // - Cluster: IAS WD (0x0502) - Warning Device: siren / strobe actuator ----------------
    { ZCL_CLUSTER_ID_SS_IAS_WD,                     ZCL_HA_PROFILE_ID, ZCL_DEVICEID_IAS_WARNING,   ZB_FUNC_IAS_WARNING,     "ias warning",     ZB_LIGHT_COLOR_CAP_NONE, &zb_device_ias_warning_ops },

    // - Cluster: Sensors - No Device ID disambiguates type ----------------
    { ZCL_CLUSTER_ID_GENERAL_POWER_CONFIG,          ZCL_HA_PROFILE_ID, ZCL_DEVICEID_NONE,   ZB_FUNC_BATTERY,                "battery",         ZB_LIGHT_COLOR_CAP_NONE, &zb_device_battery_sensor_ops },
    { ZCL_CLUSTER_ID_MS_TEMPERATURE_MEASUREMENT,    ZCL_HA_PROFILE_ID, ZCL_DEVICEID_NONE,   ZB_FUNC_TEMPERATURE,            "temperature",     ZB_LIGHT_COLOR_CAP_NONE, &zb_device_temperature_sensor_ops },
    { ZCL_CLUSTER_ID_MS_ILLUMINANCE_MEASUREMENT,    ZCL_HA_PROFILE_ID, ZCL_DEVICEID_NONE,   ZB_FUNC_ILLUMINANCE,            "illuminance",     ZB_LIGHT_COLOR_CAP_NONE, &zb_device_illuminance_sensor_ops },
    { ZCL_CLUSTER_ID_MS_PRESSURE_MEASUREMENT,       ZCL_HA_PROFILE_ID, ZCL_DEVICEID_NONE,   ZB_FUNC_PRESSURE,               "pressure",        ZB_LIGHT_COLOR_CAP_NONE, &zb_device_pressure_sensor_ops },
    { ZCL_CLUSTER_ID_MS_FLOW_MEASUREMENT,           ZCL_HA_PROFILE_ID, ZCL_DEVICEID_NONE,   ZB_FUNC_FLOW,                   "flow",            ZB_LIGHT_COLOR_CAP_NONE, &zb_device_flow_sensor_ops },
    { ZCL_CLUSTER_ID_MS_PM25_MEASUREMENT,           ZCL_HA_PROFILE_ID, ZCL_DEVICEID_NONE,   ZB_FUNC_PM25,                   "pm25",            ZB_LIGHT_COLOR_CAP_NONE, &zb_device_pm25_sensor_ops },
    { ZCL_CLUSTER_ID_MS_CO2_MEASUREMENT,            ZCL_HA_PROFILE_ID, ZCL_DEVICEID_NONE,   ZB_FUNC_CO2,                    "co2",             ZB_LIGHT_COLOR_CAP_NONE, &zb_device_co2_sensor_ops },
    { ZCL_CLUSTER_ID_MS_PM10_MEASUREMENT,           ZCL_HA_PROFILE_ID, ZCL_DEVICEID_NONE,   ZB_FUNC_PM10,                   "pm10",            ZB_LIGHT_COLOR_CAP_NONE, &zb_device_pm10_sensor_ops },
    { ZCL_CLUSTER_ID_MS_PM1_MEASUREMENT,            ZCL_HA_PROFILE_ID, ZCL_DEVICEID_NONE,   ZB_FUNC_PM1,                    "pm1",             ZB_LIGHT_COLOR_CAP_NONE, &zb_device_pm1_sensor_ops },
    { ZCL_CLUSTER_ID_MS_TVOC_MEASUREMENT,           ZCL_HA_PROFILE_ID, ZCL_DEVICEID_NONE,   ZB_FUNC_TVOC,                   "tvoc",            ZB_LIGHT_COLOR_CAP_NONE, &zb_device_tvoc_sensor_ops },
    // Develco/frient manufacturer-specific VOC (0xFC03) - gated to Develco devices in device_manager
    { ZCL_CLUSTER_ID_MS_DEVELCO_VOC,                ZCL_HA_PROFILE_ID, ZCL_DEVICEID_NONE,   ZB_FUNC_TVOC,                   "voc",             ZB_LIGHT_COLOR_CAP_NONE, &zb_device_develco_voc_sensor_ops },
    { ZCL_CLUSTER_ID_MS_FORMALDEHYDE_MEASUREMENT,   ZCL_HA_PROFILE_ID, ZCL_DEVICEID_NONE,   ZB_FUNC_FORMALDEHYDE,           "formaldehyde",    ZB_LIGHT_COLOR_CAP_NONE, &zb_device_formaldehyde_sensor_ops },
    { ZCL_CLUSTER_ID_MS_IAQ_MEASUREMENT,            ZCL_HA_PROFILE_ID, ZCL_DEVICEID_NONE,   ZB_FUNC_IAQ,                    "iaq",             ZB_LIGHT_COLOR_CAP_NONE, &zb_device_iaq_sensor_ops },
    { ZCL_CLUSTER_ID_MS_ECO2_MEASUREMENT,           ZCL_HA_PROFILE_ID, ZCL_DEVICEID_NONE,   ZB_FUNC_ECO2,                   "eco2",            ZB_LIGHT_COLOR_CAP_NONE, &zb_device_eco2_sensor_ops },
    { ZCL_CLUSTER_ID_MS_OCCUPANCY_SENSING,          ZCL_HA_PROFILE_ID, ZCL_DEVICEID_NONE,   ZB_FUNC_OCCUPANCY,              "occupancy",       ZB_LIGHT_COLOR_CAP_NONE, &zb_device_occupancy_sensor_ops },
    { ZCL_CLUSTER_ID_MS_RELATIVE_HUMIDITY,          ZCL_HA_PROFILE_ID, ZCL_DEVICEID_NONE,   ZB_FUNC_HUMIDITY,               "humidity",        ZB_LIGHT_COLOR_CAP_NONE, &zb_device_humidity_sensor_ops },
    { ZCL_CLUSTER_ID_MS_ELECTRICAL_MEASUREMENT,     ZCL_HA_PROFILE_ID, ZCL_DEVICEID_NONE,   ZB_FUNC_ELECTRICAL,             "electrical",      ZB_LIGHT_COLOR_CAP_NONE, &zb_device_electrical_measurement_ops },
    { ZCL_CLUSTER_ID_SE_METERING,                   ZCL_HA_PROFILE_ID, ZCL_DEVICEID_NONE,   ZB_FUNC_ENERGY,                 "energy",          ZB_LIGHT_COLOR_CAP_NONE, &zb_device_energy_meter_ops },
};

/******************************************************************************
 * IAS Zone Schema Registry
 ******************************************************************************/

static s_zb_device_ias_zone_schema_t s_device_ias_zone_schemas[] = {
    { SS_IAS_ZONE_TYPE_MOTION_SENSOR,               ZB_FUNC_IAS_MOTION_SENSOR,      "motion sensor",        &zb_device_ias_zone_ops },
    { SS_IAS_ZONE_TYPE_CONTACT_SWITCH,              ZB_FUNC_IAS_CONTACT_SWITCH,     "contact switch",       &zb_device_ias_zone_ops },
    { SS_IAS_ZONE_TYPE_DOOR_WINDOW_HANDLE,          ZB_FUNC_IAS_DOOR_WINDOW_HANDLE, "door-window",          &zb_device_ias_zone_ops },
    { SS_IAS_ZONE_TYPE_FIRE_SENSOR,                 ZB_FUNC_IAS_FIRE_SENSOR,        "fire sensor",          &zb_device_ias_zone_ops },
    { SS_IAS_ZONE_TYPE_WATER_SENSOR,                ZB_FUNC_IAS_WATER_SENSOR,       "water sensor",         &zb_device_ias_zone_ops },
    { SS_IAS_ZONE_TYPE_CO_SENSOR,                   ZB_FUNC_IAS_CO_SENSOR,          "co sensor",            &zb_device_ias_zone_ops },
    { SS_IAS_ZONE_TYPE_PERSONAL_EMERGENCY_DEVICE,   ZB_FUNC_IAS_PERSONAL_EMERGENCY, "personal-emergency",   &zb_device_ias_zone_ops },
    { SS_IAS_ZONE_TYPE_VIBRATION_MOVEMENT_SENSOR,   ZB_FUNC_IAS_VIBRATION_SENSOR,   "vibration sensor",     &zb_device_ias_zone_ops },
    { SS_IAS_ZONE_TYPE_GLASS_BREAK_SENSOR,          ZB_FUNC_IAS_GENERIC_SENSOR,     "glass break sensor",   &zb_device_ias_zone_ops },
    { SS_IAS_ZONE_TYPE_REMOTE_CONTROL,              ZB_FUNC_IAS_GENERIC_SENSOR,     "remote control",       &zb_device_ias_zone_ops },
    { SS_IAS_ZONE_TYPE_KEY_FOB,                     ZB_FUNC_IAS_GENERIC_SENSOR,     "key fob",              &zb_device_ias_zone_ops },
    { SS_IAS_ZONE_TYPE_KEYPAD,                      ZB_FUNC_IAS_GENERIC_SENSOR,     "keypad",               &zb_device_ias_zone_ops },
    { SS_IAS_ZONE_TYPE_STANDARD_CIE,                ZB_FUNC_IAS_GENERIC_SENSOR,     "ias zone",             &zb_device_ias_zone_ops },
    { SS_IAS_ZONE_TYPE_SECURITY_REPEATER,           ZB_FUNC_IAS_GENERIC_SENSOR,     "security repeater",    &zb_device_ias_zone_ops },
};

/******************************************************************************
 * Implementation
 ******************************************************************************/

const s_zb_device_cluster_schema_t *zb_device_schema_cluster_find(uint16_t cluster_id, uint16_t profile_id, uint16_t device_id)
{
    const s_zb_device_cluster_schema_t *wildcard = NULL;

    for (uint16_t i = 0; i < (uint16_t)ARRAY_SIZE(s_device_cluster_schemas); i++)
    {
        const s_zb_device_cluster_schema_t *s = &s_device_cluster_schemas[i];
        if (s->cluster_id != cluster_id)
            continue;
        if (s->device_id == device_id && s->profile_id == profile_id)
            return s;
        if (s->device_id == ZCL_DEVICEID_NONE && s->profile_id == profile_id)
            wildcard = s;
    }
    return wildcard;
}

const s_zb_device_cluster_schema_t *zb_device_schema_color_find_by_caps(uint16_t color_caps)
{
    /* Hue/saturation, enhanced hue and xy are three ways to say "this light can
     * be given a colour"; the driver reaches all of them through the same
     * hue/sat entry point, which picks the on-air command per capability. */
    bool has_color = (color_caps & (COLOR_CAPABILITIES_ATTR_BIT_HUE_SATURATION |
                                    COLOR_CAPABILITIES_ATTR_BIT_ENHANCED_HUE |
                                    COLOR_CAPABILITIES_ATTR_BIT_X_Y_ATTRIBUTES)) != 0;
    bool has_temp = (color_caps & COLOR_CAPABILITIES_ATTR_BIT_COLOR_TEMPERATURE) != 0;

    e_zb_function_type_t want;
    if (has_color && has_temp)  want = ZB_FUNC_EXTENDED_COLOR_LIGHT;
    else if (has_color)         want = ZB_FUNC_COLOR_LIGHT;
    else if (has_temp)          want = ZB_FUNC_COLOR_TEMP_LIGHT;
    else                        return NULL;   /* nothing settable: keep the device-ID match */

    for (uint16_t i = 0; i < (uint16_t)ARRAY_SIZE(s_device_cluster_schemas); i++)
    {
        const s_zb_device_cluster_schema_t *s = &s_device_cluster_schemas[i];
        if (s->cluster_id == ZCL_CLUSTER_ID_LIGHTING_COLOR_CONTROL && s->type == want)
            return s;
    }
    return NULL;
}

/*
 * Vendors whose Illuminance MeasuredValue is raw lux. See
 * s_zb_illuminance_quirk_t in the header.
 */
static const s_zb_illuminance_quirk_t s_illuminance_raw_lux_quirks[] = {
    /* Aqara motion sensor RTCGQ11LM: reports lux straight into the standard
     * Illuminance MeasuredValue, so the ZCL logarithmic decode must be skipped.
     *
     * PER MODEL, DELIBERATELY - a vendor wildcard would be wrong. The motion
     * sensor P1 (lumi.motion.ac02) never sends this cluster at all: its lux
     * arrives inside the Lumi 0x0112 attribute, and zb_manu_lumi_decode_opple_attr()
     * re-encodes it to the ZCL logarithmic form on purpose before injecting the
     * report, precisely so the ordinary decode applies. Marking that model raw
     * would double-count the conversion - 300 lx would surface as 24772. Any
     * Lumi model added here must therefore be one that puts lux on cluster
     * 0x0400 itself, not one fed through the manufacturer-cluster path. */
    { "LUMI", "lumi.sensor_motion.aq2" },
};

bool
zb_device_schema_illuminance_is_raw_lux(const char *manufacturer, const char *model)
{
    if (manufacturer == NULL)
    {
        return false;
    }
    for (uint16_t i = 0; i < (uint16_t)ARRAY_SIZE(s_illuminance_raw_lux_quirks); i++)
    {
        const s_zb_illuminance_quirk_t *q = &s_illuminance_raw_lux_quirks[i];
        if (q->manufacturer && strcmp(q->manufacturer, manufacturer) != 0)
            continue;
        if (q->model && ((model == NULL) || (strcmp(q->model, model) != 0)))
            continue;
        return true;
    }
    return false;
}

/*
 * ColorCapabilities overrides. See s_zb_color_caps_quirk_t in the header.
 */
static const s_zb_color_caps_quirk_t s_color_caps_quirks[] = {
    /* Tuya TS0505B RGB+CCT bulb. Firmware revisions differ on what they report
     * (0x1D on some, 0x10 - colour temperature only - on others), and 0x10
     * would strip the bulb of colour entirely. Forced to hue/saturation +
     * enhanced hue + colour loop + colour temperature.
     *
     * XY (0x08) is deliberately NOT set even though the hardware accepts the
     * command: Tuya's firmware applies an internal correction to xy that
     * renders pure blue as purple, which is why zigbee2mqtt pins these lights
     * to hs mode (modes: ["hs"]) rather than the xy default. Leaving the bit
     * clear makes zb_zcl_lighting_color_control_send_hue_sat_by_caps() pick
     * hue/saturation for the same reason. */
    { "_TZ3210_bfwvfyx1", "TS0505B", (uint16_t)(COLOR_CAPABILITIES_ATTR_BIT_HUE_SATURATION |
                                                COLOR_CAPABILITIES_ATTR_BIT_ENHANCED_HUE |
                                                COLOR_CAPABILITIES_ATTR_BIT_COLOR_LOOP |
                                                COLOR_CAPABILITIES_ATTR_BIT_COLOR_TEMPERATURE) },
};

bool
zb_device_schema_color_caps_override(const char *manufacturer, const char *model,
                                     uint16_t *caps_out)
{
    if ((manufacturer == NULL) || (model == NULL) || (caps_out == NULL))
    {
        return false;
    }
    for (uint16_t i = 0; i < (uint16_t)ARRAY_SIZE(s_color_caps_quirks); i++)
    {
        const s_zb_color_caps_quirk_t *q = &s_color_caps_quirks[i];
        if ((strcmp(q->manufacturer, manufacturer) == 0) && (strcmp(q->model, model) == 0))
        {
            *caps_out = q->color_caps;
            return true;
        }
    }
    return false;
}

/*
 * ColorTempPhysicalMin/MaxMireds fallbacks. See s_zb_color_temp_range_quirk_t.
 *
 * Consulted ONLY when the device's own report is unusable - see
 * zb_device_manager_endpoint_color_temp_range(). These lights vary by firmware
 * revision, so a row here must never outrank a sane read.
 */
static const s_zb_color_temp_range_quirk_t s_color_temp_range_quirks[] = {
    /* Tuya TS0505B RGB+CCT bulb.
     *
     * Firmware-dependent, which is exactly why this is a fallback and not an
     * override. Revision 1.2.5 reports honest mireds {153, 500} and needs
     * nothing from this table. Revision 2.2.2 reports {0, 1000} - its own
     * 0..1000 colour-temperature scale - and *acts* on that scale too: the
     * light visibly keeps changing between 500 and 1000, so the wide span is
     * real output, not a misreport to be clamped away.
     *
     * 50 rather than 0 as the floor: 0 is outside the ZCL-valid mired range
     * (1..65279) and some stacks reject it outright. zigbee2mqtt ships
     * e.light_brightness_colortemp_colorhs([50, 1000]) for this fingerprint. */
    { "_TZ3210_bfwvfyx1", "TS0505B", 50, 1000 },
};

bool
zb_device_schema_color_temp_range_override(const char *manufacturer, const char *model,
                                           uint16_t *min_out, uint16_t *max_out)
{
    if ((manufacturer == NULL) || (model == NULL) || (min_out == NULL) || (max_out == NULL))
    {
        return false;
    }
    for (uint16_t i = 0; i < (uint16_t)ARRAY_SIZE(s_color_temp_range_quirks); i++)
    {
        const s_zb_color_temp_range_quirk_t *q = &s_color_temp_range_quirks[i];
        if ((strcmp(q->manufacturer, manufacturer) == 0) && (strcmp(q->model, model) == 0))
        {
            *min_out = q->min_mireds;
            *max_out = q->max_mireds;
            return true;
        }
    }
    return false;
}

/*
 * Tuya datapoint maps. See s_zb_tuya_dp_map_t in the header for the rules -
 * in particular that every row must name both manufacturer and model, because
 * TS0601 datapoint numbering differs completely between products.
 */
static const s_zb_tuya_dp_map_t s_tuya_dp_maps[] = {
    /* Tuya TS0601 temperature/humidity sensor with LCD clock
     * (_TZE284_vvmbj46n). Datapoint numbers match zigbee2mqtt's "nous"
     * datapoints (zigbee-herdsman-converters legacy.ts): nousTemperature 1,
     * nousHumidity 2, nousBattery 4. The remaining datapoints this device
     * sends are settings we do not consume - temperature unit (9), min/max
     * alarm thresholds (10/11), report intervals (17/18), sensitivity (19).
     *
     * Scaling: the DP counts tenths of a degree and the ZCL MeasuredValue
     * counts hundredths, so temperature is x10. Humidity arrives in whole
     * percent against an attribute counting hundredths, so x100. Battery is
     * already a percentage and this driver reads BatteryPercentageRemaining as
     * a plain percent (not the spec's half-percent units), so it passes
     * through unscaled. */
    { "_TZE284_vvmbj46n", "TS0601", 1, ZCL_CLUSTER_ID_MS_TEMPERATURE_MEASUREMENT,
      ATTRID_TEMPERATURE_MEASUREMENT_MEASURED_VALUE, ZCL_DATATYPE_INT16,  10,  1 },
    { "_TZE284_vvmbj46n", "TS0601", 2, ZCL_CLUSTER_ID_MS_RELATIVE_HUMIDITY,
      ATTRID_RELATIVITY_HUMIDITY_MEASURED_VALUE,     ZCL_DATATYPE_UINT16, 100, 1 },
    { "_TZE284_vvmbj46n", "TS0601", 4, ZCL_CLUSTER_ID_GENERAL_POWER_CONFIG,
      ATTRID_POWER_CONFIG_BATTERY_PERCENTAGE_REMAINING, ZCL_DATATYPE_UINT8, 2, 1 },
};

const s_zb_tuya_dp_map_t *
zb_device_schema_tuya_dp_find(const char *manufacturer, const char *model, uint8_t dp_id)
{
    if (manufacturer == NULL || model == NULL)
    {
        return NULL;
    }
    for (uint16_t i = 0; i < (uint16_t)ARRAY_SIZE(s_tuya_dp_maps); i++)
    {
        const s_zb_tuya_dp_map_t *m = &s_tuya_dp_maps[i];
        if (m->dp_id == dp_id &&
            strcmp(m->manufacturer, manufacturer) == 0 &&
            strcmp(m->model, model) == 0)
        {
            return m;
        }
    }
    return NULL;
}

const s_zb_device_ias_zone_schema_t *zb_device_schema_ias_find(uint16_t zone_type)
{
    for (uint16_t i = 0; i < (uint16_t)ARRAY_SIZE(s_device_ias_zone_schemas); i++)
    {
        if (s_device_ias_zone_schemas[i].zone_type == zone_type)
            return &s_device_ias_zone_schemas[i];
    }
    return NULL;
}

bool zb_device_schema_cluster_is_ignored(uint16_t cluster_id)
{
    for (uint16_t i = 0; i < (uint16_t)ARRAY_SIZE(s_ignored_clusters); i++)
    {
        if (s_ignored_clusters[i] == cluster_id)
            return true;
    }
    return false;
}