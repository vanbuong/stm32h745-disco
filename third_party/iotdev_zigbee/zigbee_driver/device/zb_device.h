/*
 * zb_device.h
 * 
 * Author: Vo Van Buong (BRT-SG)
 */

#ifndef ZB_DEVICE_H_
#define ZB_DEVICE_H_

#include "common/zb_common.h"
#include "core/zb_core.h"
#include "af/zb_af.h"
#include "zcl/zb_zcl.h"

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

/******************************************************************************
 * Constants
 ******************************************************************************/

#define ZB_IEEE_HASH_SIZE                  512  /* must be power of 2, >= 2 * ZB_MAX_DEVICE */
#define ZB_MAX_DESIRED_CONFIG              16   /* persisted bind/report intents per device */

/******************************************************************************
 * Desired configuration — host-held intent (bindings + attribute reporting)
 * that the host re-applies to a device when it (re)joins or after the network
 * comes up, since a factory-reset/rejoined device loses its own tables.
 ******************************************************************************/
typedef enum e_zb_desired_cfg_kind
{
    ZB_DESIRED_CFG_BIND   = 0,   /* ZDO bind: src cluster → dst (coordinator)  */
    ZB_DESIRED_CFG_REPORT = 1,   /* ZCL ConfigureReporting on src cluster      */
    ZB_DESIRED_CFG_POLL   = 2,   /* local: poll-cadence override for a function */
} e_zb_desired_cfg_kind_t;

typedef struct s_zb_desired_config
{
    uint8_t  kind;          /* e_zb_desired_cfg_kind_t */
    uint8_t  src_endpoint;  /* endpoint on this device */
    uint16_t cluster_id;

    /* BIND target (kind == ZB_DESIRED_CFG_BIND) */
    uint64_t dst_ieee;      /* 0 = coordinator */
    uint8_t  dst_endpoint;

    /* REPORT params (kind == ZB_DESIRED_CFG_REPORT) */
    uint16_t attr_id;
    uint8_t  data_type;
    uint16_t min_interval;
    uint16_t max_interval;   /* 0xFFFF = reporting disabled */
    uint8_t  change_len;     /* reportable-change bytes (0 = discrete) */
    uint8_t  change[8];      /* reportable change, little-endian */

    /* POLL params (kind == ZB_DESIRED_CFG_POLL): persisted override of the
     * function's state-sync cadence (see s_zb_function.poll_interval_ms). */
    uint32_t poll_interval_ms;
} s_zb_desired_config_t;

/******************************************************************************
 * Forward declarations
 ******************************************************************************/

typedef struct s_zb_device s_zb_device_t;
typedef struct s_zb_function s_zb_function_t;

/******************************************************************************
 * Function Ops vtable
 ******************************************************************************/

typedef struct s_zb_function_ops
{
    size_t ctx_size;

    // lifecycle
    void (*init)(s_zb_device_t *device, s_zb_function_t *func);
    void (*destroy)(s_zb_device_t *device, s_zb_function_t *func);

    // unsolicited attribute report from device
    void (*on_attr_report)(s_zb_device_t *device, s_zb_function_t *func, s_zb_zcl_incoming_msg_t *msg);

    // solicited read attribute response (hub/internal requested)
    // typically same logic as on_attr_report but may differ for
    // functions that have multi-attr state (lights read all clusters)
    void (*on_read_rsp)(s_zb_device_t *device, s_zb_function_t *func, s_zb_zcl_incoming_msg_t *msg);

    // returns list of (clusterId, attrId) pairs to read for full state sync
    // called after device rejoins or on demand from upper layer
    const s_zb_attr_read_t* (*get_read_attrs)(s_zb_device_t *device, s_zb_function_t *func);

    // command from upper layer
    zb_status_t (*on_command)(s_zb_device_t *device, s_zb_function_t *func, const s_zb_cmd_t *cmd);

    // publish the function's current CACHED value as its normal event, with no
    // OTA read. Used to answer on-demand state snapshots (ZB_CMD_DEVICE_STATE).
    // NULL for functions that carry no reportable value.
    void (*emit_state)(s_zb_device_t *device, s_zb_function_t *func);

    // periodic housekeeping, ~1 Hz, on the driver task. For state that has to
    // change without the device saying anything - the canonical case is an
    // Aqara motion sensor, which reports motion but never reports that motion
    // ended, so occupancy has to be timed out locally. @p now_ms is the OSAL
    // monotonic clock. Must not block or issue a ZNP SREQ. NULL for functions
    // with nothing to age out.
    void (*tick)(s_zb_device_t *device, s_zb_function_t *func, uint32_t now_ms);
} s_zb_function_ops_t;

/******************************************************************************
 * Function (logical device - one per sensor/actuator per ep)
 ******************************************************************************/

/* poll_interval_ms sentinel: function carries no pollable/readable state. */
#define ZB_POLL_NEVER  0xFFFFFFFFu

typedef struct s_zb_function
{
    zb_sensor_id_t sensor_id;
    e_zb_function_type_t type;
    e_zb_light_color_cap_t color_caps;
    char name[32];
    const s_zb_function_ops_t *ops;
    void *ctx;

    /* State-sync scheduling (driven by zb_core_query_device_task):
     *   poll_interval_ms : ZB_POLL_NEVER = never read (actuator);
     *                      0 = read once on first sync, then rely on the
     *                          device's own attribute reports;
     *                      N = poll every N ms (reporting unconfigured/unsupported).
     *                      Resolved at build time from a type default + the
     *                      Device Quirk Register (zb_device_schema_resolve_poll_ms).
     *   last_read_ms     : monotonic ms of the last issued read (runtime).
     *   synced           : the initial/static read has been issued. Functions
     *                      with static formatting attrs (electrical/energy
     *                      multiplier/divisor) fetch them only while this is
     *                      false. Reset to 0 on (re)build. */
    uint32_t poll_interval_ms;
    uint32_t last_read_ms;
    bool synced;

    /* True while this function's value came from the persisted state cache
     * rather than from the device. Cleared the moment the device reports, so a
     * consumer can distinguish "23.5 C right now" from "23.5 C, last heard two
     * hours ago". See zb_device_state_cache.h. */
    bool value_stale;
} s_zb_function_t;

/******************************************************************************
 * Device endpoint (raw interview data - internal routing only)
 ******************************************************************************/
typedef struct s_zb_device_endpoint
{
    uint8_t endpoint_id;
    uint16_t profile_id;
    uint16_t device_id;
    uint16_t ias_zone_type; // ZB_IAS_ZONE_INVALID if not IAS
    uint16_t color_caps;    // ZCL ColorCapabilities attr value (bitmask)
    bool color_caps_valid;  // true = attr was read successfully
    uint16_t color_temp_min;
    uint16_t color_temp_max;
    uint8_t in_cluster_count;
    uint8_t out_cluster_count;
    uint16_t in_clusters[ZB_MAX_IN_CLUSTERS];
    uint16_t out_clusters[ZB_MAX_OUT_CLUSTERS];
} s_zb_device_endpoint_t;

typedef enum e_zb_device_status
{
    ZB_DEVICE_STATUS_UNKNOWN = 0,
    ZB_DEVICE_STATUS_OFFLINE = 1,
    ZB_DEVICE_STATUS_ONLINE = 2,
} e_zb_device_status_t;

typedef struct s_zb_device
{
    // identity (persisted to LittleFS)
    char manufacturer[32];
    char model[32];
    char product_label[32];         /* Basic 0x000E ProductLabel  */
    char serial_number[32];         /* Basic 0x000D SerialNumber  */
    char sw_build_id[32];           /* Basic 0x4000 SWBuildID     */
    uint8_t product_code[16];       /* Basic 0x000A ProductCode (octstr) */
    uint8_t product_code_len;
    uint64_t ieee_addr;
    uint64_t parent_ieee;
    uint16_t manu_id;
    
    union
    {
        uint32_t capabilities;
        struct
        {
            uint8_t device_type;
            bool battery_powered;
            bool sleepy_enabled;
            bool ota_supported;
        };
    };

    // runtime state (not persisted)
    uint16_t nwk_addr;
    uint16_t parent_nwk_addr;
    uint64_t last_seen;
    e_zb_device_status_t status;
    /* Link quality of the last frame received from this device (ZNP-reported
     * LQI 0-255). Only known from an incoming AF frame, so it is 0 until the
     * device has sent at least one frame since boot. */
    uint8_t lqi;

    // Basic-cluster metadata (from interview / basic_info; static per device,
    // deliberately NOT persisted — re-read on rejoin / next basic report).
    char    date_code[16];
    uint8_t app_version;
    uint8_t hw_version;
    uint8_t physical_environment;
    uint8_t power_source;

    // pool management
    bool in_use;
    uint8_t pool_idx;

    // raw endpoint data (persisted, routing only)
    uint8_t endpoint_count;
    s_zb_device_endpoint_t endpoints[ZB_MAX_ENDPOINTS];

    // logical functions (rebuilt on boot from schema)
    uint8_t function_count;
    s_zb_function_t functions[ZB_MAX_FUNCTIONS];

    // desired configuration intent (persisted to a sidecar file, re-applied
    // when the network is up / the device re-joins). config_applied is runtime
    // only — set once the re-apply pass has run this RUNNING session.
    // config_apply_idx tracks progress through desired_config[] during that pass.
    uint8_t desired_config_count;
    s_zb_desired_config_t desired_config[ZB_MAX_DESIRED_CONFIG];
    bool config_applied;
    uint8_t config_apply_idx;
    bool config_bind_in_flight;   /* BIND at config_apply_idx is in the bind queue */
} s_zb_device_t;

/******************************************************************************
 * Device pool (allocated entirely at init)
 ******************************************************************************/

typedef struct s_zb_device_pool
{
    s_zb_device_t blocks[ZB_MAX_DEVICE];
    bool used[ZB_MAX_DEVICE];
} s_zb_device_pool_t;

/******************************************************************************
 * IEEE address hash table (open addressing, power-of-2 size)
 ******************************************************************************/

typedef struct s_zb_ieee_hash_entry
{
    uint64_t ieee_addr;
    s_zb_device_t *device;
    bool used;
} s_zb_ieee_hash_entry_t;

typedef struct s_zb_ieee_hash_table
{
    s_zb_ieee_hash_entry_t entries[ZB_IEEE_HASH_SIZE];
} s_zb_ieee_hash_table_t;

/******************************************************************************
 * Device manager (lives in internal RAM — only pointers/indices)
 ******************************************************************************/
typedef struct s_zb_device_manager
{
    // pointer arrays in internal RAM (~9KB total)
    uint16_t device_count;

    s_zb_device_t *pool[ZB_MAX_DEVICE];
    s_zb_ieee_hash_table_t ieee_index;

    // block pool allocated in HEAP
    s_zb_device_pool_t *block_pool;

    /*
     * No mutex: the manager is confined to the iotdev_zigbee (driver) task.
     * Every mutator (add/remove/update/build) and every reader (lookups,
     * device-list snapshot) runs on that single task, so access is
     * serialised by construction. Do NOT call manager APIs from other tasks
     * — route through the facade command queue instead.
     */
} s_zb_device_manager_t;

/******************************************************************************
 * Device info - used during interview handoff to manager
 ******************************************************************************/

/******************************************************************************
 * Cached attribute values captured during the interview
 ******************************************************************************/

/* Attribute values decoded from reports (and read responses) that arrived while
 * the device was still being interviewed. At that point the device is not in
 * the device pool yet and its functions do not exist, so the normal report path
 * has nowhere to put the value and drops it. These entries ride along in
 * s_zb_device_info_t and are seeded into the freshly built functions by
 * device_manager_apply_cached_attrs(), right after device_manager_build_functions().
 *
 * Without this, the first values a device sends on join are lost - and a sleepy
 * battery sensor may then stay quiet for hours before reporting again.
 *
 * Entries are deduplicated latest-wins on (endpoint, cluster_id, attr_id), so a
 * chatty device cannot overflow the table and the freshest value always wins.
 * Runtime values only - identity attributes have their own named fields below
 * and are the ones persisted to flash. */
#define ZB_MAX_CACHED_ATTRS      4
#define ZB_CACHED_ATTR_MAX_LEN   64   /* scalars; report values are never long strings */

typedef struct s_zb_cached_attr
{
    uint8_t  endpoint;
    uint16_t cluster_id;
    uint16_t attr_id;
    uint8_t  data_type;
    uint8_t  len;
    uint8_t  value[ZB_CACHED_ATTR_MAX_LEN];
} s_zb_cached_attr_t;

typedef struct s_zb_device_info
{
    uint64_t ieee_addr;
    uint64_t parent_ieee;
    uint16_t manu_id;
    uint16_t nwk_addr;
    uint16_t parent_nwk_addr;

    char manufacturer[32];
    char model[32];
    char product_label[32];     /* Basic 0x000E ProductLabel  */
    char serial_number[32];     /* Basic 0x000D SerialNumber  */
    char date_code[16];         /* Basic DateCode (0x0006) — build/date code */
    char sw_build_id[32];       /* Basic 0x4000 SWBuildID     */
    uint8_t product_code[16];   /* Basic 0x000A ProductCode (octstr) */
    uint8_t product_code_len;
    uint8_t zcl_version;
    uint8_t app_version;
    uint8_t hw_version;         /* Basic HWVersion (0x0003) */
    uint8_t power_source;
    uint8_t physical_environment; /* Basic PhysicalEnvironment (0x0011) */
    union
    {
        uint32_t capabilities;
        struct
        {
            uint8_t device_type;
            bool battery_powered;
            bool sleepy_enabled;
            bool ota_supported;
        };
    };

    uint8_t endpoint_count;
    s_zb_device_endpoint_t endpoints[ZB_MAX_ENDPOINTS];

    /* Values seen during the interview, replayed into the functions once they
     * exist. See s_zb_cached_attr_t above. Not persisted. */
    s_zb_cached_attr_t cached_attrs[ZB_MAX_CACHED_ATTRS];
    uint8_t cached_attr_count;
} s_zb_device_info_t;

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* ZB_DEVICE_H_ */
