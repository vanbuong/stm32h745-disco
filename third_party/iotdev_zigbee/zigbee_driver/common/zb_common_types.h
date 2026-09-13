#ifndef ZB_COMMON_TYPES_H_
#define ZB_COMMON_TYPES_H_

#ifdef __cplusplus
extern "C"
{
#endif /* __cplusplus */

#include <stdint.h>
#include <stdbool.h>

/******************************************************************************
 * Constants
 ******************************************************************************/

#define ZB_ALL_CHANNEL_MASK                0x07FFF800U

#define ZB_MAX_DEVICE                      256  /* Maximum number of devices */
#define ZB_MAX_ENDPOINTS                   4    /* Maximum number of endpoints */
#define ZB_MAX_IN_CLUSTERS                 16   /* Maximum number of in clusters */
#define ZB_MAX_OUT_CLUSTERS                8    /* Maximum number of out clusters */
#define ZB_MAX_FUNCTIONS                   16   /* Maximum number of functions */
/* Clusters that may be synthesised onto one endpoint by the Device Quirk
 * Register (clusters a device uses but omits from its Simple Descriptor). */
#define ZB_MAX_SYNTHETIC_CLUSTERS_PER_ENDPOINT 4
#define ZB_MAX_ATTRS                       8    /* Maximum number of attributes per cluster */

#define ZB_DEVICE_TYPE_COORD               0x00 /* Coordinator */
#define ZB_DEVICE_TYPE_ROUTER              0x01 /* Router */
#define ZB_DEVICE_TYPE_END_DEVICE          0x02 /* End device */

#define ZB_DEVICE_CAP_DEVICE_TYPE_MASK     0x03 /* Device type mask */
#define ZB_DEVICE_CAP_BATTERY_POWER_MASK   0x04 /* Battery power mask */
#define ZB_DEVICE_CAP_SLEEPY_MASK          0x08 /* Sleepy mask */

#define ZB_DEVICE_BATTERY_VOLTAGE          0
#define ZB_DEVICE_BATTERY_PERCENTAGE       1
#define ZB_DEVICE_BATTERY_ALL              2

#define ZB_NETWORK_STATE_INIT              0
#define ZB_NETWORK_STATE_RUN               1
#define ZB_NETWORK_STATE_STOP              2

/* Coordinator (ZNP module) link state — orthogonal to the Zigbee network state
 * above. Carried in s_zb_network_info_t.coordinator_state. */
typedef enum e_zb_coordinator_state
{
    ZB_COORDINATOR_STATE_OFFLINE = 0,   /* cannot communicate with the ZNP module    */
    ZB_COORDINATOR_STATE_FW_UPDATE,     /* in bootloader; firmware being updated      */
    ZB_COORDINATOR_STATE_READY,         /* ZNP reachable (network may be init/run/stop) */
} e_zb_coordinator_state_t;

#define ARRAY_SIZE(x)  (sizeof(x) / sizeof((x)[0]))

/******************************************************************************
 * Network configuration
 ******************************************************************************/
#define ZB_NETWORK_CONFIG_F_TX_POWER      (1u << 0)
#define ZB_NETWORK_CONFIG_F_CHANNEL_MASK  (1u << 1)

typedef struct s_zb_network_config
{
    uint32_t channel_mask;
    int8_t tx_power;
    uint8_t fields;   /* ZB_NETWORK_CONFIG_F_*; used with ZB_CMD_NETWORK_CONFIG */
} s_zb_network_config_t;

/******************************************************************************
 * OTA progress (device OTA + coordinator FW update)
 ******************************************************************************/
#define ZB_OTA_PROGRESS_PHASE_DOWNLOAD    0u
#define ZB_OTA_PROGRESS_PHASE_VERIFY      1u
#define ZB_OTA_PROGRESS_PHASE_ERASE       2u

typedef struct s_zb_coordinator_info
{
    uint8_t state;
    uint8_t channel;
    int8_t tx_power;
    uint16_t pan_id;
    uint32_t version;
    uint64_t ieee_addr;
} s_zb_coordinator_info_t;

typedef struct s_zb_network_info
{
    uint8_t state;              /* ZB_NETWORK_STATE_* — Zigbee network formation      */
    uint8_t coordinator_state;  /* e_zb_coordinator_state_t — ZNP link / bootloader   */
    uint8_t channel;
    uint32_t channel_mask;
    int8_t tx_power;
    uint16_t pan_id;
    uint32_t version;           /* ZNP firmware version (0 when unknown/offline)      */
    uint64_t ieee_addr;
} s_zb_network_info_t;

/******************************************************************************
 * Function color capabilities
 ******************************************************************************/
typedef enum e_zb_light_color_cap
{
    ZB_LIGHT_COLOR_CAP_NONE     = 0x00,
    ZB_LIGHT_COLOR_CAP_HUE_SAT  = 0x01,
    ZB_LIGHT_COLOR_CAP_TEMP     = 0x02,
    ZB_LIGHT_COLOR_CAP_ALL      = 0x03,
} e_zb_light_color_cap_t;

/******************************************************************************
 * Function types
 ******************************************************************************/
typedef enum e_zb_function_type
{
    // sensors
    ZB_FUNC_BASIC_INFO              = 0x00,
    ZB_FUNC_BATTERY                 = 0x01,
    ZB_FUNC_TEMPERATURE             = 0x02,
    ZB_FUNC_HUMIDITY                = 0x03,
    ZB_FUNC_PRESSURE                = 0x04,
    ZB_FUNC_ILLUMINANCE             = 0x05,
    ZB_FUNC_OCCUPANCY               = 0x06,
    ZB_FUNC_FLOW                    = 0x07,
    ZB_FUNC_PM25                    = 0x08,
    ZB_FUNC_CO2                     = 0x09,
    ZB_FUNC_ELECTRICAL              = 0x0A,
    ZB_FUNC_ENERGY                  = 0x0B,
    ZB_FUNC_PM10                    = 0x0C,
    ZB_FUNC_PM1                     = 0x0D,
    ZB_FUNC_TVOC                    = 0x0E,
    ZB_FUNC_FORMALDEHYDE            = 0x0F,

    // IAS sensors
    ZB_FUNC_IAS_MOTION_SENSOR       = 0x10,
    ZB_FUNC_IAS_CONTACT_SWITCH      = 0x11,
    ZB_FUNC_IAS_DOOR_WINDOW_HANDLE  = 0x12,
    ZB_FUNC_IAS_FIRE_SENSOR         = 0x13,
    ZB_FUNC_IAS_WATER_SENSOR        = 0x14,
    ZB_FUNC_IAS_CO_SENSOR           = 0x15,
    ZB_FUNC_IAS_PERSONAL_EMERGENCY  = 0x16,
    ZB_FUNC_IAS_VIBRATION_SENSOR    = 0x17,
    ZB_FUNC_IAS_GENERIC_SENSOR      = 0x1F,

    // lights
    ZB_FUNC_ONOFF_LIGHT             = 0x20,
    ZB_FUNC_DIMMABLE_LIGHT          = 0x21,
    ZB_FUNC_COLOR_TEMP_LIGHT        = 0x22,
    ZB_FUNC_COLOR_LIGHT             = 0x23,
    ZB_FUNC_EXTENDED_COLOR_LIGHT    = 0x24,
    ZB_FUNC_ONOFF_PLUGIN_UNIT       = 0x25,
    ZB_FUNC_DIMMABLE_PLUGIN_UNIT    = 0x26,

    // switches / actuators
    ZB_FUNC_ONOFF_SWITCH            = 0x30,
    ZB_FUNC_DIMMER_SWITCH           = 0x31,
    ZB_FUNC_BUTTON                  = 0x32,   /* momentary multi-action button (single/double/long) */
    ZB_FUNC_ONOFF_SMART_PLUG        = 0x40,
    ZB_FUNC_RELAY                   = 0x41,
    ZB_FUNC_IAS_WARNING             = 0x42,
    ZB_FUNC_MAINS_POWER_OUTLET      = 0x43,
    ZB_FUNC_IAQ                     = 0x44,
    ZB_FUNC_ECO2                    = 0x45,

    ZB_FUNC_ELEC_CURRENT            = 0x50,
    ZB_FUNC_ELEC_VOLTAGE            = 0x51,
    ZB_FUNC_ELEC_FREQUENCY          = 0x52,
    ZB_FUNC_ELEC_TOTAL_ACTIVE_POWER = 0x53,
    ZB_FUNC_ELEC_ACTIVE_POWER       = 0x54,
    ZB_FUNC_ELEC_REACTIVE_POWER     = 0x55,
    ZB_FUNC_ELEC_APPARENT_POWER     = 0x56,
    ZB_FUNC_ELEC_POWER_FACTOR       = 0x57,

} e_zb_function_type_t;

/* Action reported by a momentary multi-action button (Multistate Input, 0x0012). */
typedef enum e_zb_button_action
{
    ZB_BUTTON_SINGLE = 0,
    ZB_BUTTON_DOUBLE = 1,
    ZB_BUTTON_LONG   = 2,
} e_zb_button_action_t;

/******************************************************************************
 * Sensor ID - encodes epId + clusterId
 ******************************************************************************/

typedef uint32_t zb_sensor_id_t;
 
#define ZB_SENSOR_ID(epId, clusterId)  (((uint32_t)(epId) << 16) | (clusterId))
#define ZB_SENSOR_EP(sensorId)         ((uint8_t)((sensorId) >> 16))
#define ZB_SENSOR_CLUSTER(sensorId)    ((uint16_t)((sensorId) & 0xFFFF))

/******************************************************************************
 * Event types (library → upper layer)
 ******************************************************************************/

typedef enum e_zb_event_type
{
    ZB_EVENT_SENSOR_BATTERY,
    ZB_EVENT_SENSOR_TEMPERATURE,
    ZB_EVENT_SENSOR_HUMIDITY,
    ZB_EVENT_SENSOR_PRESSURE,
    ZB_EVENT_SENSOR_ILLUMINANCE,
    ZB_EVENT_SENSOR_OCCUPANCY,
    ZB_EVENT_SENSOR_FLOW,
    ZB_EVENT_SENSOR_PM25,
    ZB_EVENT_SENSOR_CO2,
    ZB_EVENT_SENSOR_PM10,
    ZB_EVENT_SENSOR_PM1,
    ZB_EVENT_SENSOR_TVOC,
    ZB_EVENT_SENSOR_FORMALDEHYDE,
    ZB_EVENT_SENSOR_IAQ,
    ZB_EVENT_SENSOR_ECO2,
    ZB_EVENT_SENSOR_ELECTRICAL_MEASUREMENT,
    ZB_EVENT_SENSOR_ENERGY_METERING,
    ZB_EVENT_SENSOR_IAS_ZONE,
    ZB_EVENT_DEVICE_IAS_WARNING,

    ZB_EVENT_BINARY_STATE,
    ZB_EVENT_LIGHT_ONOFF_STATE,
    ZB_EVENT_LIGHT_DIMMABLE_STATE,
    ZB_EVENT_LIGHT_COLOR_TEMP_STATE,
    ZB_EVENT_LIGHT_COLOR_HUE_SAT_STATE,
    ZB_EVENT_LIGHT_EXTENDED_COLOR_STATE,

    ZB_EVENT_DEVICE_JOINED,
    ZB_EVENT_DEVICE_UPDATED,
    ZB_EVENT_DEVICE_LEFT,
    ZB_EVENT_DEVICE_OTA_STARTED,
    ZB_EVENT_DEVICE_OTA_FAILED,
    ZB_EVENT_DEVICE_OTA_PROGRESS,
    ZB_EVENT_DEVICE_OTA_COMPLETED,
    ZB_EVENT_DEVICE_OTA_ABORTED,

    ZB_EVENT_NETWORK_STATE_INIT,
    ZB_EVENT_NETWORK_STATE_STOP,
    ZB_EVENT_NETWORK_STATE_RUN,     /* @deprecated prefer ZB_EVENT_NETWORK_INFO.state==RUN */
    ZB_EVENT_NETWORK_INFO,
    ZB_EVENT_NETWORK_OPEN,
    ZB_EVENT_NETWORK_CLOSE,
    ZB_EVENT_NETWORK_AF_INCOMING_MSG,
    ZB_EVENT_NETWORK_TX_POWER_CHANGED,
    ZB_EVENT_NETWORK_CHANNEL_MASK_CHANGED,
    
    ZB_EVENT_COORDINATOR_BOOTLOADER_ENTER_SUCCESS,
    ZB_EVENT_COORDINATOR_BOOTLOADER_ENTER_FAILED,
    ZB_EVENT_COORDINATOR_FW_UPDATE_STARTED,
    ZB_EVENT_COORDINATOR_FW_UPDATE_FAILED,
    ZB_EVENT_COORDINATOR_FW_UPDATE_PROGRESS,
    ZB_EVENT_COORDINATOR_FW_UPDATE_COMPLETED,
    ZB_EVENT_COORDINATOR_FW_UPDATE_ABORTED,

    /*
     * Device-list snapshot, delivered as a framed burst in response to
     * ZB_CMD_DEVICE_LIST: one BEGIN, N ITEM, one END, all carrying the
     * same txn_id so the consumer can correlate them to its request.
     * The burst is emitted atomically on the driver task, so it is a
     * coherent point-in-time snapshot even while the network is open and
     * devices are joining; subsequent joins/leaves arrive as the usual
     * ZB_EVENT_DEVICE_JOINED / ZB_EVENT_DEVICE_LEFT deltas after END.
     * New event types are appended here (never reordered) so existing
     * numeric values stay stable for upper layers that persist them.
     */
    ZB_EVENT_DEVICE_LIST_BEGIN,
    ZB_EVENT_DEVICE_LIST_ITEM,
    ZB_EVENT_DEVICE_LIST_END,

    /*
     * Input controls (wall on/off switch, dimmer switch) reporting their own
     * position — distinct from the LIGHT_* events, which are controllable
     * outputs.  Carry `switch_onoff` / `switch_level`.
     */
    ZB_EVENT_SWITCH_ONOFF_STATE,
    ZB_EVENT_SWITCH_LEVEL_STATE,

    /*
     * Cached-state snapshot brackets, emitted atomically on the driver task (no
     * yields), so everything delivered between a BEGIN and its END belongs to
     * that snapshot. Two producers share these brackets; distinguish by the item
     * event type in between:
     *   - ZB_CMD_DEVICE_STATE (burst): the normal per-function value events
     *     (SENSOR_*, LIGHT_*, SWITCH_*, …) re-emitted from cache, each keeping
     *     its own type + ieee_addr/sensor_id (no txn_id stamped on them).
     *   - ZB_CMD_DEVICE_STATE_ARRAY: one ZB_EVENT_DEVICE_STATE per device.
     */
    ZB_EVENT_DEVICE_STATE_BEGIN,
    ZB_EVENT_DEVICE_STATE_END,

    /*
     * Array-form cached snapshot (ZB_CMD_DEVICE_STATE_ARRAY): one event per
     * device carrying ALL of that device's function values in `device_state`.
     * Also framed by DEVICE_STATE_BEGIN/_END so the all-devices case has a
     * completion signal (BEGIN.device_count = number of these to follow).
     */
    ZB_EVENT_DEVICE_STATE,

    /*
     * One neighbour-table (ZDO Mgmt_Lqi) entry, emitted per entry as the
     * responses from each queried node arrive. Carries `neighbor_lqi`:
     * `src_addr` is the node whose table it is; the rest describe one neighbour
     * (short/IEEE address, relationship, depth, and the measured per-hop LQI).
     * Unlike the device-list burst these are not framed - each is a
     * self-contained edge - and they may page in over time.
     */
    ZB_EVENT_NETWORK_NEIGHBOR_LQI,

    /* Momentary button action (Multistate Input, 0x0012): carries `button`
     * with an e_zb_button_action_t. Emitted per press; has no resting state. */
    ZB_EVENT_BUTTON_ACTION,

    /*
     * Liveness changed: the device went quiet for longer than its availability
     * timeout, or spoke again after having been marked offline. Carries
     * `availability`. Only transitions are emitted, never a repeat of the state
     * already published. Appended here on purpose - subscribers key off these
     * ordinals, so new events go at the end rather than in the middle.
     */
    ZB_EVENT_DEVICE_AVAILABILITY,

    ZB_EVENT_TYPE_MAX

} e_zb_event_type_t;

/******************************************************************************
 * Sensor units (scalar events only — use e_zb_sensor_unit_t, not strings)
 ******************************************************************************/

typedef enum e_zb_sensor_unit
{
    ZB_UNIT_NONE       = 0,
    ZB_UNIT_DEGC       = 1,   /* °C */
    ZB_UNIT_PERCENT_RH = 2,   /* %RH */
    ZB_UNIT_HPA        = 3,   /* hPa */
    ZB_UNIT_LUX        = 4,   /* lux */
    ZB_UNIT_M3_PER_H   = 5,   /* m³/h */
    ZB_UNIT_PPM        = 6,   /* ppm */
    ZB_UNIT_UG_PER_M3  = 7,   /* µg/m³ */
    ZB_UNIT_PPB        = 8,   /* ppb */
} e_zb_sensor_unit_t;

typedef struct s_zb_config
{
    uint8_t form_new_network;
    uint8_t is_factory_reset;
    int8_t tx_power;
    uint8_t endpoint;
    uint8_t channel;
    uint32_t channel_mask;
    uint16_t pan_id;
    uint64_t ieee_addr;
    uint16_t profile_id;
    uint16_t device_id;
    uint8_t num_in_cluster;
    uint8_t num_out_cluster;
    uint16_t in_cluster_id[16];
    uint16_t out_cluster_id[20];
} s_zb_config_t;

typedef struct s_zb_device_joined_info
{
    char manufacturer[32];
    char model[32];
    char date_code[16];         /* Basic DateCode (0x0006) — build/date code */
    char product_label[32];     /* Basic ProductLabel (0x000E) */
    char serial_number[32];     /* Basic SerialNumber (0x000D) */
    char sw_build_id[32];       /* Basic SWBuildID (0x4000) */
    uint8_t product_code[16];   /* Basic ProductCode (0x000A, octet string) */
    uint8_t product_code_len;
    uint64_t parent_ieee;       /* parent's IEEE, 0 if unknown */
    uint16_t parent_nwk_addr;   /* parent's short address, 0xFFFF if unknown */
    bool battery_powered;
    bool sleepy_enabled;
    bool ota_supported;
    uint8_t device_type;
    uint8_t app_version;
    uint8_t hw_version;         /* Basic HWVersion (0x0003) */
    uint8_t physical_environment; /* Basic PhysicalEnvironment (0x0011) */
    uint8_t power_source;
    uint8_t function_count;
    struct {
        zb_sensor_id_t sensor_id;
        e_zb_function_type_t type;
        char name[32];
    } functions[ZB_MAX_FUNCTIONS];
} s_zb_device_joined_info_t;

/******************************************************************************
 * One function's cached value, tagged by value_type. Carried in the array-form
 * device-state snapshot (ZB_EVENT_DEVICE_STATE). `value_type` names the active
 * union member and mirrors the per-function event type the value would ride in
 * during a live update (e.g. ZB_EVENT_SENSOR_TEMPERATURE -> `sensor`).
 ******************************************************************************/
typedef struct s_zb_func_value
{
    zb_sensor_id_t       sensor_id;   /* endpoint+cluster of this function */
    e_zb_function_type_t type;        /* ZB_FUNC_*                          */
    e_zb_event_type_t    value_type;  /* which union member below is valid  */
    union {
        struct { float value; e_zb_sensor_unit_t unit; } sensor;
        struct { float percent; float voltage; } battery;
        struct { bool active; } binary;
        struct { float voltage; float current; float power; float frequency;
                 float reactive_power; float apparent_power; float power_factor; } electrical;
        float energy;
        struct { uint16_t zone_type; uint16_t zone_status; } ias;
        struct { uint16_t max_duration; } ias_warning;
        struct { bool on; } onoff;
        struct { uint8_t level; } level;
        struct { uint8_t action; } button;   /* e_zb_button_action_t */
        struct { uint16_t color_temp; } color_temp;
        struct { uint8_t hue; uint8_t saturation; } hue_sat;
        struct { uint8_t hue; uint8_t saturation; uint16_t color_temp; } ext_color;
    } v;
} s_zb_func_value_t;

typedef struct s_zb_event
{
    e_zb_event_type_t type;
    zb_sensor_id_t sensor_id;
    uint64_t ieee_addr;
    char name[32];

    union
    {
        struct
        {
            float value;
            e_zb_sensor_unit_t unit;
        } sensor;

        struct
        {
            uint32_t measurement_type;
            float voltage;
            float current;
            float power;            /* active power, W   */
            float frequency;        /* Hz                */
            float reactive_power;   /* VAR               */
            float apparent_power;   /* VA                */
            float power_factor;     /* cos(phi), -1..+1  */
        } electrical_measurement;

        struct
        {
            bool active;
        } binary;

        struct
        {
            bool on;
        } onoff_light;

        struct
        {
            uint8_t level;
        } dimmable_light;

        /* ZB_EVENT_SWITCH_ONOFF_STATE — input on/off switch position */
        struct
        {
            bool on;
        } switch_onoff;

        /* ZB_EVENT_SWITCH_LEVEL_STATE — input dimmer switch level */
        struct
        {
            uint8_t level;
        } switch_level;

        /* ZB_EVENT_BUTTON_ACTION — momentary button press (e_zb_button_action_t) */
        struct
        {
            uint8_t action;
        } button;

        struct
        {
            uint16_t color_temp;
        } color_temp_light;

        struct
        {
            uint8_t hue;
            uint8_t saturation;
        } color_light;

        struct
        {
            uint8_t hue;
            uint8_t saturation;
            uint16_t color_temp;
        } extended_color_light;

        struct
        {
            float percent;
            float voltage;
        } battery;

        struct
        {
            uint16_t zone_type;
            uint16_t zone_status;
        } ias;

        struct
        {
            uint16_t max_duration;
        } ias_warning;

        /* ZB_EVENT_DEVICE_AVAILABILITY */
        struct
        {
            bool     online;        /* new liveness state                    */
            uint32_t silent_for_s;  /* seconds since the last frame, 0 if now */
        } availability;

        float energy;

        // used for DEVICE_JOINED
        s_zb_device_joined_info_t device_info;
        s_zb_coordinator_info_t coord_info;
        s_zb_network_info_t network_info;

        /* ZB_EVENT_DEVICE_OTA_PROGRESS / ZB_EVENT_COORDINATOR_FW_UPDATE_PROGRESS */
        struct
        {
            uint8_t percent;    /* 0-100 */
            uint8_t phase;      /* ZB_OTA_PROGRESS_PHASE_* */
        } ota_progress;

        // ZB_EVENT_DEVICE_LIST_BEGIN — opens a device-list snapshot
        struct
        {
            uint32_t txn_id;    /* echoes the requesting command's txn_id */
            uint16_t total;     /* number of ITEM events that will follow */
        } device_list_begin;

        // ZB_EVENT_DEVICE_LIST_ITEM — one device in the snapshot.
        // `ieee_addr` (top-level) identifies the device.
        struct
        {
            uint32_t txn_id;
            uint16_t index;     /* 0-based position within this snapshot */
            uint16_t total;     /* same total as BEGIN (for convenience) */
            uint16_t nwk_addr;  /* current short address, 0xFFFF if unknown */
            bool     online;    /* false when the device is unreachable */
            uint64_t last_seen; /* UTC epoch seconds of last message, 0 if never */
            uint8_t  lqi;       /* LQI of the last frame received, 0 if never */
            s_zb_device_joined_info_t info; /* identity, caps, functions, parent */
        } device_list_item;

        // ZB_EVENT_DEVICE_LIST_END — closes the snapshot
        struct
        {
            uint32_t txn_id;
            uint16_t count;     /* number of ITEM events actually emitted */
            uint8_t  status;    /* ZB_OK on success */
        } device_list_end;

        // ZB_EVENT_DEVICE_STATE_BEGIN — opens a cached-state snapshot.
        // `ieee_addr` (top-level) is the requested device, 0 for all devices.
        struct
        {
            uint32_t txn_id;        /* echoes the requesting command's txn_id */
            uint16_t device_count;  /* devices whose functions will be emitted */
        } device_state_begin;

        // ZB_EVENT_DEVICE_STATE_END — closes the snapshot
        struct
        {
            uint32_t txn_id;
            uint16_t emitted;   /* number of function value events emitted */
            uint8_t  status;    /* ZB_OK on success */
        } device_state_end;

        // ZB_EVENT_DEVICE_STATE — array-form snapshot: ALL of one device's
        // function values in a single event (`ieee_addr` = the device). Emitted
        // by ZB_CMD_DEVICE_STATE_ARRAY, one per device, between DEVICE_STATE_BEGIN
        // and _END.
        struct
        {
            uint32_t txn_id;
            uint8_t  count;     /* valid entries in functions[] */
            s_zb_func_value_t functions[ZB_MAX_FUNCTIONS];
        } device_state;

        // ZB_EVENT_NETWORK_NEIGHBOR_LQI — one neighbour-table entry from `src_addr`.
        struct
        {
            uint16_t src_addr;      /* node whose neighbour table this is (the responder) */
            uint64_t src_ieee;      /* responder's IEEE, 0 if unknown            */
            uint16_t nwk_addr;      /* neighbour's short address                 */
            uint64_t ieee_addr;     /* neighbour's IEEE, 0 if unknown            */
            uint8_t  relationship;  /* ZB_NEIGHBOR_REL_* (parent/child/sibling…) */
            uint8_t  depth;         /* neighbour's tree depth                    */
            uint8_t  lqi;           /* measured LQI of this radio link           */
        } neighbor_lqi;

        void *af_msg;
    };
} s_zb_event_t;

/******************************************************************************
 * Command types (upper layer → library)
 ******************************************************************************/
typedef enum e_zb_cmd_type
{
    // network commands
    ZB_CMD_NETWORK_INFO,
    ZB_CMD_NETWORK_CONFIG,

    ZB_CMD_NETWORK_OPEN,
    ZB_CMD_NETWORK_CLOSE,
    ZB_CMD_NETWORK_START,
    ZB_CMD_NETWORK_STOP,
    ZB_CMD_NETWORK_FACTORY_RESET,

    // device commands
    ZB_CMD_DEVICE_LIST,
    ZB_CMD_DEVICE_STATE,            // snapshot cached function values as a burst (ieee=0 → all)
    ZB_CMD_DEVICE_STATE_ARRAY,      // snapshot cached values as one array event per device
    ZB_CMD_DEVICE_BIND,
    ZB_CMD_DEVICE_UNBIND,
    ZB_CMD_DEVICE_REMOVE,
    ZB_CMD_DEVICE_CONFIG_REPORT,
    ZB_CMD_DEVICE_OTA_START,
    ZB_CMD_DEVICE_OTA_ABORT,
    ZB_CMD_DEVICE_OTA_PROGRESS,

    // coprocessor commands
    ZB_CMD_COPROC_FW_UPDATE_START,
    ZB_CMD_COPROC_BOOTLOADER_CHECK,

    // actuator commands
    ZB_CMD_SET_ONOFF,
    ZB_CMD_SET_LEVEL,
    ZB_CMD_SET_COLOR_TEMP,
    ZB_CMD_SET_HUE_SAT,
    ZB_CMD_IAS_WARNING,
    ZB_CMD_READ_STATE,              // request current state sync
    ZB_CMD_SET_POLL_INTERVAL,       // override a function's state-sync poll cadence (ms)
    ZB_CMD_DEVICE_WRITE_ATTR,       // write one attribute on a device (device configuration)

    // network diagnostics (appended to keep existing command values stable)
    ZB_CMD_NETWORK_QUERY_LQI,       // query coordinator neighbour LQI (devices -> parent)
    ZB_CMD_DEVICE_IDENTIFY,         // make a device announce itself (Identify cluster)
} e_zb_cmd_type_t;

typedef struct s_zb_cmd
{
    e_zb_cmd_type_t type;
    union {
        // network commands
        s_zb_network_config_t network_config;
        struct { uint8_t duration; } network_open;
        /* ZB_CMD_DEVICE_BIND / ZB_CMD_DEVICE_UNBIND — bind=false means unbind. */
        struct {
            bool bind;
            uint64_t ieee_addr;
            zb_sensor_id_t sensor_id;
        } device_bind;
        /* ZB_CMD_DEVICE_CONFIG_REPORT — configure attribute reporting on the
         * target function's cluster (top-level sensor_id). Sent to the device;
         * it then reports to its binding destination (bind to the coordinator
         * first). max_interval 0xFFFF disables reporting. change[] holds the
         * reportable-change value (little-endian) for analog types; change_len
         * = 0 for discrete types. */
        struct {
            uint16_t attr_id;
            uint8_t  data_type;
            uint16_t min_interval;
            uint16_t max_interval;
            uint8_t  change_len;
            uint8_t  change[8];
        } device_config_report;
        /* ZB_CMD_DEVICE_WRITE_ATTR — write one attribute on the target device.
         * For device settings that live in an attribute rather than behind a
         * cluster command: an Aqara P1's detection interval / motion
         * sensitivity (0xFCC0 attrs 0x0102 / 0x010C, manuf_code 0x115F), a
         * TS0210's vibration sensitivity (IAS Zone 0x0013, manuf_code 0).
         *
         *   cluster_id : 0 = the cluster encoded in the top-level sensor_id.
         *                Set it explicitly when the setting lives on a
         *                DIFFERENT cluster from the function being addressed,
         *                which is the usual case for manufacturer clusters.
         *   manuf_code : 0 = standard (non-manufacturer-specific) frame.
         *   value      : little-endian, value_len bytes.
         *
         * NOTE: most such devices are sleepy end devices. The write only lands
         * while the device is awake, so wake it first (walk in front of a
         * motion sensor, press the button on a vibration sensor). */
        struct {
            uint16_t cluster_id;
            uint16_t manuf_code;
            uint16_t attr_id;
            uint8_t  data_type;    /* ZCL_DATATYPE_* */
            uint8_t  value_len;
            uint8_t  value[8];
        } device_write_attr;

        struct { uint32_t txn_id; } device_list;  /* correlates the DEVICE_LIST_* response burst */
        struct { uint32_t txn_id; } device_state; /* correlates the DEVICE_STATE_* response burst */
        struct { const char* path; } device_ota_start;
        struct { const char* path; } coprocessor_fw_update_start;

        /* ZB_CMD_SET_POLL_INTERVAL — runtime override of the target function's
         * state-sync poll cadence (see func->poll_interval_ms): ZB_POLL_NEVER =
         * never read, 0 = read once then rely on reports, N = poll every N ms. */
        struct { uint32_t interval_ms; } poll;
        
        // actuator commands
        struct { bool on; } onoff;
        struct { uint8_t level; uint16_t trans_ms; } brightness;
        struct { uint16_t color_temp; uint16_t trans_ms; } color_temp;
        struct { uint8_t hue; uint8_t saturation; uint16_t trans_ms; } hue_sat;

        /* ZB_CMD_IAS_WARNING - IAS Warning Device (WD) "Start Warning".
         * Sounds the siren / flashes the strobe on a warning device
         * (e.g. a Zigbee siren). Fields map to the ZCL SS IAS WD
         * Start-Warning payload; see SS_IAS_* defines in zb_zcl_ss.h.
         * mode = SS_IAS_START_WARNING_WARNING_MODE_STOP cancels an
         * in-progress warning. */
        struct {
            uint8_t  mode;             /* SS_IAS_START_WARNING_WARNING_MODE_* */
            uint8_t  strobe;           /* SS_IAS_START_WARNING_STROBE_*       */
            uint8_t  siren_level;      /* SS_IAS_SIREN_LEVEL_*                */
            uint16_t duration;         /* warning duration, seconds           */
            uint8_t  strobe_duty_cycle;/* 0..100 %, 0 = strobe off            */
            uint8_t  strobe_level;     /* SS_IAS_STROBE_LEVEL_*               */
        } ias_warning;

        /* ZB_CMD_DEVICE_IDENTIFY — Identify cluster (0x0003) "Identify"
         * command: the device announces itself physically (blink, beep,
         * whatever its firmware does) for identify_time seconds, so an
         * installer can tell which unit an IEEE address belongs to.
         *
         *   identify_time : seconds to keep identifying; 0 stops an
         *                   identify already in progress.
         *
         * Addressed to the device as a whole, so the top-level sensor_id is
         * unused — the dispatcher picks the device's first endpoint that
         * advertises the Identify cluster. */
        struct {
            uint16_t identify_time;
        } device_identify;
    };
} s_zb_cmd_t;

/******************************************************************************
 * Read request descriptor — used by on_read_rsp to know
 * which attrs to request for a full state sync
 ******************************************************************************/
typedef struct s_zb_attr_read
{
    uint16_t cluster_id;
    uint8_t attr_count;
    uint16_t attr_id[ZB_MAX_ATTRS];
} s_zb_attr_read_t;

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* ZB_COMMON_TYPES_H_ */