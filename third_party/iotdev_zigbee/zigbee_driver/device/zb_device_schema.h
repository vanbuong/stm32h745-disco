#ifndef ZB_DEVICE_SCHEMA_H_
#define ZB_DEVICE_SCHEMA_H_

#include "common/zb_common.h"
#include "device/zb_device.h"

/******************************************************************************
 * Cluster schema - maps (manufacturerId + profileId + deviceId + ClusterId)
 * to function type
 ******************************************************************************/

typedef struct s_zb_device_cluster_schema
{
    uint16_t cluster_id;
    uint16_t profile_id;
    uint16_t device_id;     // 0xFFFF = any device

    e_zb_function_type_t type;
    const char *base_name;
    uint8_t color_capabilities;
    const s_zb_function_ops_t *ops;
} s_zb_device_cluster_schema_t;

/******************************************************************************
 * IAS zone schema - maps zoneType to function type
 ******************************************************************************/

typedef struct s_zb_device_ias_zone_schema
{
    uint16_t zone_type;
    e_zb_function_type_t type;
    const char *name;
    const s_zb_function_ops_t *ops;
} s_zb_device_ias_zone_schema_t;

/******************************************************************************
 * Synthetic cluster quirk - clusters a device uses but does not advertise
 ******************************************************************************/

/*
 * Some devices send traffic for a cluster that is absent from their Simple
 * Descriptor, so the interview never learns about it and no function is built -
 * the traffic then arrives with nowhere to go ("function not found").
 *
 * The canonical case is the Lumi/Aqara water-leak sensor: it sends IAS Zone
 * Status Change Notifications but does not list cluster 0x0500 in its in-cluster
 * list, and never performs IAS enrollment.
 *
 * An entry here makes device_manager_build_functions() build the function
 * anyway, for matching devices only. The device's advertised in_clusters[] is
 * left untouched (and so stays truthful on flash); the quirk is re-applied on
 * every build, including after a reload from flash.
 */
typedef struct s_zb_synthetic_cluster_quirk
{
    const char *manufacturer;   // exact ManufacturerName, NULL = any
    const char *model;          // exact ModelIdentifier,  NULL = any
    uint8_t     endpoint_id;    // endpoint to attach the function to
    uint16_t    cluster_id;     // cluster to synthesise
    uint16_t    ias_zone_type;  // for ZCL_CLUSTER_ID_SS_IAS_ZONE: assumed zone type
} s_zb_synthetic_cluster_quirk_t;

/******************************************************************************
 * Tuya datapoint map - readings that arrive on cluster 0xEF00
 ******************************************************************************/

/*
 * Tuya's TS0601 devices implement no standard measurement clusters: every
 * reading travels as a numbered "datapoint" (DP) on the manufacturer cluster
 * 0xEF00. An entry here says what one DP means for one product, so the decoder
 * in manu/tuya can re-emit it as an ordinary attribute report and the standard
 * device functions can consume it unchanged.
 *
 * The DP numbering is per PRODUCT, not per cluster: DP 1 is temperature on a
 * TS0601 climate sensor and target setpoint on a TS0601 thermostat. Every entry
 * must therefore name both the manufacturer and the model - a wildcard model
 * would silently mis-decode unrelated Tuya hardware.
 *
 * The value is rescaled into the attribute's units as
 * `attr = dp_value * multiplier / divisor` (e.g. a DP counting tenths of a
 * degree into a ZCL attribute counting hundredths: multiplier 10, divisor 1).
 *
 * Each mapped cluster also needs a synthetic-cluster quirk above, otherwise the
 * re-emitted report has no function to land in - the device never advertised
 * the cluster it is being translated into.
 */
typedef struct s_zb_tuya_dp_map
{
    const char *manufacturer;   // exact ManufacturerName
    const char *model;          // exact ModelIdentifier
    uint8_t     dp_id;          // Tuya datapoint number
    uint16_t    cluster_id;     // standard cluster to re-emit the value on
    uint16_t    attr_id;        // attribute within that cluster
    uint8_t     data_type;      // ZCL_DATATYPE_* of the re-emitted attribute
    int32_t     multiplier;     // attr = dp_value * multiplier / divisor
    int32_t     divisor;
} s_zb_tuya_dp_map_t;

/******************************************************************************
 * Illuminance encoding quirk - vendors that report lux directly
 ******************************************************************************/

/*
 * ZCL says the Illuminance Measurement MeasuredValue is logarithmic:
 *
 *      MeasuredValue = 10000 * log10(lux) + 1     ->  lux = 10^((value-1)/10000)
 *
 * Lumi/Aqara ignore that and put the lux reading straight into the attribute.
 * Decoding one as the other is not a small error: a raw 3000 lx decodes to
 * 1.99 lx, which is wrong but looks plausible enough to go unnoticed.
 *
 * zigbee2mqtt handles this the same way - these devices are routed to their own
 * converter (lumi.fromZigbee.lumi_illuminance) instead of the generic
 * logarithmic one, and the Lumi proprietary-TLV path assigns the value with no
 * maths at all.
 *
 * An entry here says "this vendor's MeasuredValue is already lux".
 */
typedef struct s_zb_illuminance_quirk
{
    const char *manufacturer;   // exact ManufacturerName, NULL = any
    const char *model;          // exact ModelIdentifier, NULL = any model
} s_zb_illuminance_quirk_t;

/******************************************************************************
 * Colour capability quirk - lights that misreport ColorCapabilities
 ******************************************************************************/

/*
 * ColorCapabilities (0x400A) is supposed to say what a light can do, and the
 * driver trusts it for two decisions: which function type the Color Control
 * cluster becomes, and which on-air command a colour change uses.
 *
 * Some lights lie. Tuya's TS0505B is RGB+CCT hardware that has been seen
 * reporting 0x10 (colour temperature only) on some firmware revisions, which
 * would cost it colour entirely. zigbee2mqtt does not trust the attribute for
 * these either - it declares the capability in the device definition, and its
 * TS0505B users write colorCapabilities back to the device to force it.
 *
 * An entry here supplies the value to use instead, keyed by
 * ManufacturerName/ModelIdentifier. The value the device actually reported is
 * left untouched on the endpoint (and so stays truthful on flash); only the
 * decisions above see the override.
 */
typedef struct s_zb_color_caps_quirk
{
    const char *manufacturer;   // exact ManufacturerName
    const char *model;          // exact ModelIdentifier
    uint16_t    color_caps;     // COLOR_CAPABILITIES_ATTR_BIT_* to use instead
} s_zb_color_caps_quirk_t;

/******************************************************************************
 * Capability quirk - identity/capabilities a device fails to advertise
 ******************************************************************************/

/*
 * The Node Descriptor carries manu_id, logical device type, and the MAC
 * capability flags (mains vs battery, rx-on vs sleepy). A few devices never
 * answer the Node Descriptor request (e.g. the frient/Develco AQSZB-110), so
 * those fields are left at their zero defaults after the interview.
 *
 * An entry here supplies the known-good values for such a device, keyed by the
 * ManufacturerName/ModelIdentifier read later from the Basic cluster. It is
 * applied unconditionally on a match (like the other quirks), so the table must
 * hold the device's true capabilities - use it only for devices whose Node
 * Descriptor is genuinely unavailable.
 */
typedef struct s_zb_device_caps_quirk
{
    const char *manufacturer;   // exact ManufacturerName
    const char *model;          // exact ModelIdentifier
    uint16_t    manu_id;        // Zigbee manufacturer code
    uint8_t     device_type;    // ZB_DEVICE_TYPE_*
    bool        battery_powered;
    bool        sleepy_enabled;
} s_zb_device_caps_quirk_t;

/******************************************************************************
 * Schema API
 ******************************************************************************/

// Milliseconds of quiet after which an occupancy function should clear motion
// by itself, for sensors that report detection but never report that it ended
// (Aqara motion sensors). Returns 0 for every other device, meaning "trust the
// sensor to send occupancy=0" - the default, and correct for standard sensors.
// A PIROccupiedToUnoccupiedDelay reported by the device overrides this.
uint32_t zb_device_schema_resolve_occupancy_clear_ms(const char *manufacturer, const char *model);

// Synthetic cluster quirk table. Returns the table and writes its length to
// *count. Callers filter on manufacturer/model/endpoint_id themselves.
const s_zb_synthetic_cluster_quirk_t *zb_device_schema_synthetic_clusters(uint16_t *count);

// Find the capability quirk for a device, or NULL if none. manufacturer/model
// may be NULL (treated as no match).
const s_zb_device_caps_quirk_t *zb_device_schema_caps_find(const char *manufacturer, const char *model);

// Find cluster schema: exact (manuId + profileId + deviceId + clusterId) match first,
// then wildcard (manuId + profileId + 0xFFFF + clusterId) fallback
const s_zb_device_cluster_schema_t *zb_device_schema_cluster_find(uint16_t cluster_id, uint16_t profile_id, uint16_t device_id);

/*
 * Pick the Color Control schema row that matches a device's ColorCapabilities
 * (0x400A) rather than the ZCL device ID it advertises.
 *
 * The device ID only says which light family an endpoint belongs to, and some
 * lights get it wrong: Aqara's ZNLDP13LM (lumi.light.acn014) advertises the
 * colour-dimmable device ID but reports capabilities 0x10 - colour temperature
 * only - and answers Move To Hue And Saturation with UNSUP_CLUSTER_COMMAND.
 * The attribute is authoritative, so prefer it when the interview read it.
 *
 * @param  color_caps  ColorCapabilities bitmask, as read from the endpoint.
 * @return The matching schema row, or NULL when the mask names no colour
 *         capability this driver can drive (caller keeps its device-ID match).
 */
const s_zb_device_cluster_schema_t *zb_device_schema_color_find_by_caps(uint16_t color_caps);

// Find IAS zone schema by zone type
const s_zb_device_ias_zone_schema_t *zb_device_schema_ias_find(uint16_t zone_type);

/*
 * Look up what one Tuya datapoint means on one device.
 *
 * @param  manufacturer  ManufacturerName read from the Basic cluster.
 * @param  model         ModelIdentifier read from the Basic cluster.
 * @param  dp_id         Datapoint number from the 0xEF00 record.
 * @return The mapping, or NULL when this product has no meaning registered for
 *         that datapoint (the common case - these devices report settings and
 *         alarm thresholds alongside the readings).
 */
const s_zb_tuya_dp_map_t *zb_device_schema_tuya_dp_find(const char *manufacturer,
                                                        const char *model,
                                                        uint8_t dp_id);

/*
 * ColorCapabilities to use for a device, in place of what it reported.
 *
 * @param  manufacturer  ManufacturerName read from the Basic cluster.
 * @param  model         ModelIdentifier read from the Basic cluster.
 * @param  caps_out      Written only on a match.
 * @return true when an override applies; false leaves @p caps_out untouched
 *         and the caller should use the device's own value.
 */
bool zb_device_schema_color_caps_override(const char *manufacturer,
                                          const char *model,
                                          uint16_t *caps_out);

/*
 * ColorTempPhysicalMin/MaxMireds overrides.
 *
 * The two attributes are meant to be mireds (ZCL 5.2.2.2.11-12, valid 1..65279),
 * and a light that answers with 0 or with an inverted pair has told us nothing
 * usable. Some vendors do exactly that: Tuya's bulbs report their own internal
 * 0..1000 colour-temperature scale through these attributes instead of mireds.
 *
 * Crucially, such a light also *commands* on that scale - MoveToColorTemperature
 * with 1000 produces visibly different output from 500 - so the reported span is
 * real even though the unit is wrong. Rewriting {0, 1000} to textbook mireds
 * would surrender half the light's usable range. zigbee2mqtt does not convert
 * either: it passes the raw value straight through in the standard ZCL command
 * and simply declares the wider range in the device definition.
 *
 * A row here is therefore a FALLBACK, not an override. It applies only when the
 * device's own report is unusable, because the same manufacturer+model can
 * report honest mireds on one firmware revision and its private scale on the
 * next - a row that outranked a sane read would break the good firmware.
 *
 * What the device reported is left untouched on the endpoint and stays truthful
 * on flash; only callers asking for a usable range see the substitute.
 */
typedef struct s_zb_color_temp_range_quirk
{
    const char *manufacturer;   // exact ManufacturerName
    const char *model;          // exact ModelIdentifier
    uint16_t    min_mireds;     // coolest, in mireds
    uint16_t    max_mireds;     // warmest, in mireds
} s_zb_color_temp_range_quirk_t;

/*
 * The colour-temperature range to use for a device whose own report is
 * unusable. Callers must try the device's reported values first.
 *
 * @param  manufacturer  ManufacturerName read from the Basic cluster.
 * @param  model         ModelIdentifier read from the Basic cluster.
 * @param  min_out       Written only on a match.
 * @param  max_out       Written only on a match.
 * @return true when an override applies; false leaves both outputs untouched
 *         and the caller should use the device's own values.
 */
bool zb_device_schema_color_temp_range_override(const char *manufacturer,
                                                const char *model,
                                                uint16_t *min_out,
                                                uint16_t *max_out);

/*
 * True when this device's Illuminance MeasuredValue is already in lux and must
 * NOT be put through the ZCL logarithmic decode. See s_zb_illuminance_quirk_t.
 */
bool zb_device_schema_illuminance_is_raw_lux(const char *manufacturer, const char *model);

// true if cluster should never produce a function (basic, identify, etc.)
bool zb_device_schema_cluster_is_ignored(uint16_t cluster_id);

// true if a cluster may produce a function on this device. Manufacturer/model-
// gated clusters (Device Quirk Register) only pass for matching devices; all
// other clusters always pass. manufacturer/model may be NULL (treated as "no
// match" against a non-NULL quirk field).
bool zb_device_schema_cluster_allowed(uint16_t cluster_id, const char *manufacturer, const char *model);

// Resolve a function's state-sync poll interval (ms): exact manufacturer/model
// (and optional cluster) override from the Device Quirk Register first, then a
// per-function-type default. Returns ZB_POLL_NEVER (never read), 0 (read once,
// rely on reports), or N ms (poll every N ms).
uint32_t zb_device_schema_resolve_poll_ms(const char *manufacturer, const char *model,
                                          uint16_t cluster_id, e_zb_function_type_t type);

#endif /* ZB_DEVICE_SCHEMA_H_ */