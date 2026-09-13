/*
 * iotdev_zigbee.h
 *
 *  Public facade for the Zigbee middleware on ESP32.
 *
 *  Design philosophy
 *  -----------------
 *  The driver layer under zigbee_driver/device/ already builds the right
 *  abstraction: every logical sensor / actuator on a device is keyed by
 *  a `zb_sensor_id_t` (an opaque encoding of endpoint + cluster), and
 *  each device function speaks in semantic terms — temperature value,
 *  on/off bool, brightness level, battery percent, etc. — never in raw
 *  ZCL bytes, attribute IDs or cluster numbers.
 *
 *  This facade therefore exposes that same abstraction unchanged.  An
 *  application component talks to the Zigbee subsystem in three pieces
 *  of information:
 *
 *      - who      -> `ieee_addr`     (uint64_t, 0 for network-wide)
 *      - what     -> `sensor_id`     (zb_sensor_id_t, 0 for non-function cmds)
 *      - payload  -> `s_zb_cmd_t`    on the way down,
 *                   `s_zb_event_t`  on the way up.
 *
 *  Both `s_zb_cmd_t` and `s_zb_event_t` are defined by the driver in
 *  `zigbee_driver/common/zb_common_types.h`.  They are re-used here as
 *  the public contract so we do not duplicate Zigbee taxonomy at the
 *  facade boundary.
 *
 *  Patterns
 *  --------
 *      - Command Pattern  (outbound)  - iotdev_zigbee_send_command()
 *        routes a structured command either to the right device function
 *        (function-level: SET_ONOFF, SET_BRIGHTNESS, READ_STATE, ...)
 *        via the device manager + sensor_id, or to a network-level
 *        handler (NETWORK_OPEN, DEVICE_REMOVE, OTA_START, ...).
 *
 *      - Observer Pattern (inbound)   - components register a callback
 *        with iotdev_zigbee_subscribe() and receive `s_zb_event_t`
 *        events as the driver publishes them.  Multiple subscribers per
 *        event type are supported, including a wildcard "any event".
 *
 *  Constraints met
 *  ---------------
 *      - Resource-constrained ESP32: static subscriber pool, value-type
 *        events (no malloc on the hot path).
 *      - Thread-safe: recursive mutex; subscribers are snapshotted
 *        before invocation so callbacks can subscribe / publish
 *        re-entrantly without deadlocking.
 *      - Decoupled: drivers never include this header.  The facade
 *        depends downward on the driver, never the other way round.
 *
 */

#ifndef COMPONENTS_IOTDEV_ZIGBEE_INCLUDE_IOTDEV_ZIGBEE_H_
#define COMPONENTS_IOTDEV_ZIGBEE_INCLUDE_IOTDEV_ZIGBEE_H_

#ifdef __cplusplus
extern "C"
{
#endif

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

#include "iotdev_common/include/iotdev_common.h"
/*
 * The driver layer owns the Zigbee taxonomy:
 *   - zb_sensor_id_t        (endpoint + cluster encoding)
 *   - e_zb_event_type_t     (event ID enum)
 *   - s_zb_event_t          (event payload union, semantic data)
 *   - e_zb_cmd_type_t       (command ID enum)
 *   - s_zb_cmd_t            (command payload union, semantic data)
 * We re-export them as-is so the facade stays a thin pub/sub + dispatch.
 */
#include "iotdev_zigbee/zigbee_driver/core/zb_core.h"
#include "iotdev_zigbee/zigbee_driver/common/zb_common.h"

/* ========================================================================
 *  CONFIGURATION
 * ====================================================================== */
#ifndef IOTDEV_ZIGBEE_MAX_SUBSCRIBERS
#define IOTDEV_ZIGBEE_MAX_SUBSCRIBERS       16
#endif

#define IOTDEV_ZIGBEE_INVALID_SUB_HANDLE    ((iotdev_zigbee_sub_handle_t)-1)

/**
 * Wildcard event type for subscribe(): the callback receives every event
 * regardless of its `event->type`.  Chosen to be outside the
 * e_zb_event_type_t value range (the enum tops out around 50).
 */
#define IOTDEV_ZIGBEE_EVENT_ANY             ((e_zb_event_type_t)0xFFFF)

/* ========================================================================
 *  STATUS / ERROR CODES
 * ====================================================================== */
typedef int32_t iotdev_zigbee_status_t;

#define IOTDEV_ZIGBEE_OK                    (0)
#define IOTDEV_ZIGBEE_ERR                   (-1)
#define IOTDEV_ZIGBEE_ERR_PARAM             (-2)
#define IOTDEV_ZIGBEE_ERR_NO_MEM            (-3)
#define IOTDEV_ZIGBEE_ERR_NOT_FOUND         (-4)
#define IOTDEV_ZIGBEE_ERR_BUSY              (-5)
#define IOTDEV_ZIGBEE_ERR_TIMEOUT           (-6)
#define IOTDEV_ZIGBEE_ERR_NOT_SUPPORTED     (-7)
#define IOTDEV_ZIGBEE_ERR_NOT_INIT          (-8)
/* -9 reserved (was IOTDEV_ZIGBEE_ERR_VERSION; envelope no longer versioned) */
#define IOTDEV_ZIGBEE_ERR_NOT_READY         (-10)  /**< Zigbee network is not running */

/* ========================================================================
 *  COMMAND ENVELOPE
 *  ----------------------------------------------------------------------
 *  All callers assemble the same envelope:
 *
 *      • For function-level commands (anything addressed to a logical
 *        sensor / actuator on a device — SET_ONOFF, SET_BRIGHTNESS,
 *        READ_STATE, …): set `ieee_addr` and `sensor_id` and fill
 *        `cmd.type` + the matching union member.
 *
 *      • For network-level commands (NETWORK_OPEN, DEVICE_REMOVE,
 *        DEVICE_BIND, OTA_START, …): only `cmd.type` and the union
 *        member matter; `ieee_addr` and `sensor_id` may be left zero.
 *
 *  The dispatcher inspects `cmd.type` to decide which path to take, so
 *  callers never need to know about cluster IDs, attribute IDs, ZCL
 *  frame layouts, or endpoints.
 * ====================================================================== */
typedef struct s_iotdev_zigbee_command_info
{
    uint64_t       ieee_addr;   /**< target device, 0 for network-wide      */
    zb_sensor_id_t sensor_id;   /**< target function, 0 for non-function    */
    s_zb_cmd_t     cmd;         /**< type + semantic payload (driver-owned) */
} s_iotdev_zigbee_command_info_t;

/* ========================================================================
 *  DEVICE INFO SNAPSHOT
 *  ----------------------------------------------------------------------
 *  A flat, facade-owned copy of everything the application typically wants
 *  to know about one device. Filled synchronously by
 *  iotdev_zigbee_get_device_info() from an IEEE address (e.g. one collected
 *  from a ZB_EVENT_DEVICE_LIST_ITEM event). It is a value snapshot taken
 *  under the driver lock; it does not track the device afterwards.
 * ====================================================================== */
typedef struct s_iotdev_zigbee_device_info
{
    /* Addressing */
    uint64_t ieee_addr;
    uint16_t nwk_addr;              /**< 0xFFFF if unknown                 */
    uint64_t parent_ieee;           /**< 0 if unknown                      */
    uint16_t parent_nwk_addr;       /**< 0xFFFF if unknown                 */

    /* Runtime status */
    bool     online;                /**< false when unreachable            */
    uint8_t  lqi;                   /**< LQI of last frame, 0 if never      */
    uint64_t last_seen;             /**< UTC epoch seconds, 0 if never      */

    /* Identity (Basic cluster) */
    char     manufacturer[32];
    char     model[32];
    char     product_label[32];
    char     serial_number[32];
    char     sw_build_id[32];
    char     date_code[16];
    uint8_t  product_code[16];
    uint8_t  product_code_len;

    /* Capabilities / metadata */
    uint16_t manu_id;               /**< Zigbee manufacturer code          */
    uint8_t  device_type;           /**< ZB_DEVICE_TYPE_*                   */
    uint8_t  power_source;          /**< Basic PowerSource; POWER_SOURCE_*  */
    uint8_t  app_version;
    uint8_t  hw_version;
    uint8_t  physical_environment;
    bool     battery_powered;
    bool     sleepy_enabled;
    bool     ota_supported;

    /* Logical functions (sensors/actuators); ZB_FUNC_BASIC_INFO omitted. */
    uint8_t  function_count;
    struct {
        zb_sensor_id_t       sensor_id;
        e_zb_function_type_t type;
        char                 name[32];
    } functions[ZB_MAX_FUNCTIONS];
} s_iotdev_zigbee_device_info_t;

/* ========================================================================
 *  EVENT ENVELOPE
 *  ----------------------------------------------------------------------
 *  Subscribers receive the driver-owned `s_zb_event_t` directly — it
 *  already exposes `ieee_addr`, `sensor_id`, `name` and a semantic union
 *  (battery percent, temperature value, on/off, hue/sat, IAS zone, ...).
 *  No envelope is needed.
 * ====================================================================== */

typedef int16_t iotdev_zigbee_sub_handle_t;

/**
 * @brief  Subscriber callback prototype.
 *
 * @param  event  Event being delivered.  The pointer (and its data) are
 *                only guaranteed valid for the duration of the call;
 *                copy anything you need to retain.
 * @param  ctx    Opaque user context provided at subscribe time.
 *
 * @note   Callbacks are invoked from the publisher's thread context.
 *         Keep them short and non-blocking; for long work, dispatch to
 *         your own task queue.
 */
typedef void (*iotdev_zigbee_event_callback_t)(const s_zb_event_t *event,
                                               void               *ctx);

typedef enum e_iotdev_zigbee_device_state
{
    IOTDEV_ZIGBEE_DEVICE_STATE_UNKNOWN = 0,
    IOTDEV_ZIGBEE_DEVICE_STATE_ONLINE,
    IOTDEV_ZIGBEE_DEVICE_STATE_SUSPECT,
    IOTDEV_ZIGBEE_DEVICE_STATE_OFFLINE,
} e_iotdev_zigbee_device_state_t;

typedef enum e_iotdev_zigbee_device_power_source
{
    IOTDEV_ZIGBEE_DEVICE_POWER_UNKNOWN = 0,
    IOTDEV_ZIGBEE_DEVICE_POWER_BATTERY,
    IOTDEV_ZIGBEE_DEVICE_POWER_MAINS,
} e_iotdev_zigbee_device_power_source_t;

typedef struct s_iotdev_zigbee_device_descriptor
{
    char uid[IOTDEV_UID_MAX_LEN];               /**< ZCL product code */

    char name[IOTDEV_PRODUCT_NAME_MAX_LEN];     /**< ZCL ProductLabel */

    char nick[IOTDEV_PRODUCT_NAME_MAX_LEN];     /**< not tracked yet, empty */

    char model[IOTDEV_MODEL_MAX_LEN];           /**< ZCL ModelIdentifier */

    char manufacturer[IOTDEV_MANUFACTURER_MAX_LEN];

    char serial_no[IOTDEV_SERIAL_MAX_LEN];      /**< ZCL SerialNumber */

    char fw_version[IOTDEV_VERSION_MAX_LEN];    /**< ZCL SoftwareBuildID */

    char product_rev[IOTDEV_VERSION_MAX_LEN];   /**< ZCL HWVersion */

    char manufacture_date[9];                   /**< ZCL DateCode (YYYYMMDD) */

    char device_type[IOTDEV_DEVICE_TYPE_MAX_LEN];

    uint8_t function_count;

    uint16_t function[ZB_MAX_FUNCTIONS];

    bool battery_powered;

    bool sleepy_device;

    bool ota_supported;

    uint64_t ieee_addr;

} s_iotdev_zigbee_device_descriptor_t;

typedef struct s_iotdev_zigbee_device_health
{
    e_iotdev_zigbee_device_state_t state;

    e_iotdev_zigbee_device_power_source_t power_source;

    uint8_t battery;   /**< 0-100%; from the device's battery function cache, 0 if none */

    int8_t rssi;       /**< not tracked per-device by the driver yet; always 0 */

    uint8_t lqi;        /**< link quality of the last received frame (0-255) */

    uint64_t last_seen; /**< UTC epoch seconds; 0 if never seen */

} s_iotdev_zigbee_device_health_t;

typedef struct s_iotdev_zigbee_device_state
{
    uint64_t timestamp;   /**< device's last_seen, UTC epoch seconds */

    uint8_t value_count;

    /* One entry per reading, not per function: a function that measures
     * several quantities (electrical measurement) occupies several slots. */
    uint16_t function[ZB_MAX_FUNCTIONS];

    float value[ZB_MAX_FUNCTIONS];

} s_iotdev_zigbee_device_state_t;

/* ========================================================================
 *  PUBLIC API
 * ====================================================================== */

/**
 * @brief  Initialise the Zigbee middleware.
 *
 * Must be called once before any other API.  Sets up the subscriber
 * pool, the recursive mutex, and registers internal hooks with both
 * the driver (zb_core) and the device manager so all events flow into
 * the broker.
 */
void iotdev_zigbee_init(void);

/**
 * @brief  Tear down the Zigbee middleware.
 */
void iotdev_zigbee_deinit(void);

/* ----- Tasks (FreeRTOS task entry points, signature = TaskFunction_t) - */
void iotdev_zigbee_task(void *param);
void iotdev_zigbee_znp_task(void *param);
void iotdev_zigbee_ota_task(void *param);

bool iotdev_zigbee_is_app_task_running(void);
bool iotdev_zigbee_is_znp_task_running(void);
bool iotdev_zigbee_is_ota_task_running(void);
bool iotdev_zigbee_get_running_status(void);

int  iotdev_zigbee_get_core_temperature(int16_t *temperature);
int  iotdev_zigbee_get_heap_statistics(uint32_t *free_heap, uint32_t *total_heap);

/* ----------------------------------------------------------------------
 *  COMMAND PATTERN — outbound
 * --------------------------------------------------------------------*/

/**
 * @brief  Send a command into the Zigbee subsystem.
 *
 * Thread-safety / execution model
 * --------------------------------
 *  This call is safe to invoke from any FreeRTOS task.  It NEVER runs
 *  driver / ZCL code in the caller's context:
 *
 *   - Function-level commands (SET_ONOFF, SET_BRIGHTNESS, READ_STATE,
 *     …) are copied by value onto an internal queue and executed
 *     later, on the iotdev_zigbee task, via the matching device
 *     function's `on_command` op.  IOTDEV_ZIGBEE_OK therefore means
 *     "queued for execution", not "command completed on the network".
 *     Listen for the corresponding event (state report, attribute
 *     read response, …) via iotdev_zigbee_subscribe() to know when
 *     the operation finishes.
 *
 *   - Network-level commands (NETWORK_OPEN, DEVICE_REMOVE, DEVICE_BIND,
 *     DEVICE_OTA_START, COPROCESSOR_FW_UPDATE_START, …) are forwarded
 *     into zb_core's own event queue, which already serialises them on
 *     the same OS task.
 *
 * Network-readiness gating
 * ------------------------
 *  Function-level commands ALWAYS require the Zigbee network to be
 *  running — every one of them is translated by the target device
 *  function into an over-the-air ZCL frame.  Network-level commands
 *  mostly require it too (NETWORK_OPEN/CLOSE, BIND/UNBIND,
 *  DEVICE_REMOVE, OTA, …).  In all those cases the call returns
 *  IOTDEV_ZIGBEE_ERR_NOT_READY when iotdev_zigbee_get_running_status()
 *  is false.
 *
 *  A small allow-list of network-level commands is exempt and may be
 *  sent at any time:
 *      • ZB_CMD_NETWORK_INFO   (metadata read — often used precisely
 *                               to discover that the network is *not*
 *                               up yet)
 *      • ZB_CMD_NETWORK_CONFIG (apply channel / PAN-ID before forming)
 *      • ZB_CMD_COPROCESSOR_FW_UPDATE_*  (ZNP firmware update runs
 *                               with the Zigbee stack down)
 *
 *  The check is enforced twice: a fast reject at submit time (avoid
 *  filling the command queue with doomed work), and an authoritative
 *  re-check at dispatch time (the network can flip between submit and
 *  execution; only the dispatcher's view is trustworthy).
 *
 * @return IOTDEV_ZIGBEE_OK             accepted (queued for execution).
 * @return IOTDEV_ZIGBEE_ERR_NOT_READY  Zigbee network is not running
 *                                      and this command requires it.
 * @return IOTDEV_ZIGBEE_ERR_BUSY       function-command queue is full;
 *                                      caller may retry.
 * @return IOTDEV_ZIGBEE_ERR_PARAM      cmd is NULL.
 * @return IOTDEV_ZIGBEE_ERR_NOT_INIT   middleware not initialised.
 * @return IOTDEV_ZIGBEE_ERR_*          other failure.
 *
 * @note  Not safe from an ISR.  If you need to send a command from an
 *        ISR, defer to a task first.
 */
iotdev_zigbee_status_t
iotdev_zigbee_send_command(const s_iotdev_zigbee_command_info_t *cmd);

/**
 * @brief  Request a full snapshot of the currently known devices.
 *
 * Convenience wrapper around iotdev_zigbee_send_command() with a
 * ZB_CMD_DEVICE_LIST envelope.  The snapshot is delivered asynchronously,
 * on the iotdev_zigbee task, as a framed burst of events that all carry
 * the supplied @p txn_id:
 *
 *      ZB_EVENT_DEVICE_LIST_BEGIN   (event->device_list_begin.total)
 *      ZB_EVENT_DEVICE_LIST_ITEM   × N  (event->device_list_item — one per
 *                                       device: ieee_addr, nwk_addr, online,
 *                                       last_seen, and the full info/function
 *                                       list)
 *      ZB_EVENT_DEVICE_LIST_END     (event->device_list_end.count / .status)
 *
 * Subscribe to those three event types (or IOTDEV_ZIGBEE_EVENT_ANY) before
 * calling, and match the burst by @p txn_id.
 *
 * Snapshot semantics
 * ------------------
 *  The burst is emitted atomically with respect to device join/leave, so it
 *  is a coherent point-in-time view even while the network is OPEN and
 *  devices are joining.  Devices that join or leave after the snapshot are
 *  reported through the normal ZB_EVENT_DEVICE_JOINED / ZB_EVENT_DEVICE_LEFT
 *  events; a consumer should treat the list as the baseline and those events
 *  as deltas, reconciling by ieee_addr (idempotent upsert / remove).
 *
 *  Allowed whether or not the Zigbee network is running — the list comes from
 *  the local device database, not from the air.
 *
 * @param  txn_id  Opaque correlation token echoed back in every event of the
 *                 resulting burst.  Caller chooses the value (e.g. a counter).
 *
 * @return IOTDEV_ZIGBEE_OK on accept; same error codes as
 *         iotdev_zigbee_send_command().
 */
iotdev_zigbee_status_t
iotdev_zigbee_request_device_list(uint32_t txn_id);

/**
 * @brief  Fetch a full snapshot of one device by IEEE address, synchronously.
 *
 * Unlike iotdev_zigbee_request_device_list() (asynchronous burst of events),
 * this fills @p out in place and returns immediately. Intended for a caller that
 * already holds an IEEE address - e.g. one collected from a
 * ZB_EVENT_DEVICE_LIST_ITEM event - and wants everything about that one device
 * without subscribing to events.
 *
 * Reads the local device database only (no over-the-air traffic), so it is
 * allowed whether or not the Zigbee network is running, and may be called from
 * any task.
 *
 * @param  ieee_addr  Device to look up.
 * @param  out        Destination snapshot (must be non-NULL).
 * @return IOTDEV_ZIGBEE_OK             device found and copied.
 * @return IOTDEV_ZIGBEE_ERR_PARAM      out is NULL.
 * @return IOTDEV_ZIGBEE_ERR_NOT_INIT   middleware not initialised.
 * @return IOTDEV_ZIGBEE_ERR_NOT_FOUND  no device with that IEEE address.
 */
iotdev_zigbee_status_t
iotdev_zigbee_get_device_info(uint64_t ieee_addr, s_iotdev_zigbee_device_info_t *out);

/**
 * @brief  Request a cached snapshot of a device's (or all devices') function
 *         values.
 *
 * Convenience wrapper around iotdev_zigbee_send_command() with a
 * ZB_CMD_DEVICE_STATE envelope. The snapshot is delivered asynchronously, on
 * the iotdev_zigbee task, as a framed burst carrying @p txn_id on its brackets:
 *
 *      ZB_EVENT_DEVICE_STATE_BEGIN  (event->device_state_begin.device_count)
 *      <per-function value events>  × N  (the normal SENSOR_* / LIGHT_* / …
 *                                        events, re-emitted from each function's
 *                                        cached value; each keeps its own type
 *                                        and ieee_addr / sensor_id)
 *      ZB_EVENT_DEVICE_STATE_END    (event->device_state_end.emitted / .status)
 *
 * The burst is emitted atomically on the driver task, so every function event
 * delivered between the BEGIN and its END belongs to this snapshot — correlate
 * by that bracketing (the per-function events do not carry the txn_id).
 *
 * Values come from each function's local cache (no over-the-air read), so this
 * is allowed whether or not the Zigbee network is running and returns the
 * last-known values.
 *
 * @param  ieee_addr  Target device, or 0 to snapshot every known device.
 * @param  txn_id     Correlation token echoed on the BEGIN/END brackets.
 *
 * @return IOTDEV_ZIGBEE_OK on accept; same error codes as
 *         iotdev_zigbee_send_command().
 */
iotdev_zigbee_status_t
iotdev_zigbee_request_device_state(uint64_t ieee_addr, uint32_t txn_id);

/**
 * @brief  Array-form of iotdev_zigbee_request_device_state(): deliver each
 *         device's cached function values as ONE event per device.
 *
 * Same cached data and the same DEVICE_STATE_BEGIN/END brackets as the burst
 * variant, but instead of a stream of per-function events, each device's values
 * arrive together in a single ZB_EVENT_DEVICE_STATE:
 *
 *      ZB_EVENT_DEVICE_STATE_BEGIN  (device_state_begin.device_count)
 *      ZB_EVENT_DEVICE_STATE  × N   (event->ieee_addr = device; event->device_state
 *                                    holds count + functions[] of s_zb_func_value_t,
 *                                    each tagged by value_type)
 *      ZB_EVENT_DEVICE_STATE_END    (device_state_end.emitted / .status)
 *
 * Prefer this when the consumer wants a device's whole state as one object;
 * prefer iotdev_zigbee_request_device_state() when it already handles the live
 * per-function events and wants to reuse those handlers. Cached values only, so
 * it is allowed whether or not the network is running.
 *
 * @param  ieee_addr  Target device, or 0 to snapshot every known device.
 * @param  txn_id     Correlation token echoed on the BEGIN/END brackets.
 */
iotdev_zigbee_status_t
iotdev_zigbee_request_device_state_array(uint64_t ieee_addr, uint32_t txn_id);

/**
 * @brief  Ask a device to physically announce itself (ZCL Identify cluster).
 *
 * Convenience wrapper around iotdev_zigbee_send_command() with a
 * ZB_CMD_DEVICE_IDENTIFY envelope. The device blinks / beeps / does whatever
 * its firmware defines for @p identify_time seconds, which is how an installer
 * works out which physical unit an IEEE address belongs to.
 *
 * Fire-and-forget: the command is queued and executed on the iotdev_zigbee
 * task, and the return value only reports whether it was accepted. Whether the
 * device actually identified is not reported back — the feedback is physical.
 * Devices are free to clamp or ignore the requested duration.
 *
 * Identify is addressed to the device as a whole, not to one of its logical
 * functions, so there is no sensor_id: the middleware picks the endpoint,
 * preferring one that advertises the Identify cluster.
 *
 * Requires a running Zigbee network. Most battery-powered sensors are sleepy
 * end devices that only receive while awake, so wake the device first (press
 * its button / trigger it) or the frame will not be delivered.
 *
 * @param  ieee_addr      Target device. Must be a known (joined) device.
 * @param  identify_time  Seconds to keep identifying; 0 stops an identify
 *                        that is already running.
 *
 * @return IOTDEV_ZIGBEE_OK on accept, IOTDEV_ZIGBEE_ERR_PARAM if ieee_addr is
 *         0, otherwise the error codes of iotdev_zigbee_send_command(). A
 *         device that is unknown by the time the command runs is only logged
 *         at dispatch, not reported back here.
 */
iotdev_zigbee_status_t
iotdev_zigbee_identify_device(uint64_t ieee_addr, uint16_t identify_time);

/**
 * @brief  Set a colour light's colour from an RGB triple.
 *
 * Convenience wrapper for callers that think in RGB - a UI colour picker, a
 * scene, an automation. Zigbee has no RGB colour mode, so the triple is
 * converted here (zb_zcl_lighting_rgb_to_hue_sat()) and sent as a
 * ZB_CMD_SET_HUE_SAT envelope; the driver then picks whichever on-air command
 * the device implements - hue/saturation, enhanced hue, or move-to-colour xy -
 * from its ColorCapabilities.
 *
 * Like iotdev_zigbee_identify_device() this is addressed to the device rather
 * than to one of its functions: the first colour-capable function is chosen,
 * so there is no sensor_id to supply. Use iotdev_zigbee_send_command() with a
 * ZB_CMD_SET_HUE_SAT envelope to target a specific function on a multi-head
 * fixture.
 *
 * BRIGHTNESS IS NOT PART OF THE COLOUR. RGB(64,0,0) and RGB(255,0,0) are the
 * same command - both mean "red" - because level lives on the Level Control
 * cluster. Set it with a ZB_CMD_SET_LEVEL envelope. Greys and black carry no
 * hue and resolve to the light's whitest point.
 *
 * Fire-and-forget: the return value only says the command was accepted.
 *
 * @param  ieee_addr  Target device. Must be a known (joined) device.
 * @param  r, g, b    Colour, 0-255 per channel.
 * @param  trans_ms   Transition time in milliseconds; 0 = immediate.
 *
 * @return IOTDEV_ZIGBEE_OK on accept, IOTDEV_ZIGBEE_ERR_PARAM if ieee_addr is
 *         0, IOTDEV_ZIGBEE_ERR_NOT_FOUND if the device is unknown or exposes
 *         no colour function, otherwise the error codes of
 *         iotdev_zigbee_send_command().
 */
iotdev_zigbee_status_t
iotdev_zigbee_set_device_color_rgb(uint64_t ieee_addr,
                                   uint8_t r, uint8_t g, uint8_t b,
                                   uint16_t trans_ms);

/**
 * @brief  Read a colour light's current colour back as an RGB triple.
 *
 * Answers from the driver's cached state - whatever the last report or read
 * response said - so it costs no on-air traffic. The cache is only as fresh as
 * the device's reporting; see iotdev_zigbee_get_device_state() for the raw
 * per-function values.
 *
 * Returned at full brightness for the same reason the setter ignores it: the
 * cached colour is a hue/saturation pair, and level is a separate function.
 *
 * @param  ieee_addr  Target device.
 * @param  r, g, b    Out: colour, 0-255 per channel.
 *
 * @return true   the device has a colour function and @p r/g/b were written.
 * @return false  unknown device, no colour function, or a NULL output.
 */
bool
iotdev_zigbee_get_device_color_rgb(uint64_t ieee_addr,
                                   uint8_t *r, uint8_t *g, uint8_t *b);

/**
 * @brief  Unpack an IOTDEV_APP_LAMP_COLOR value from the state view into RGB.
 *
 * The state view carries one float per reading, so colour travels as a packed
 * 0xRRGGBB integer (see IOTDEV_APP_LAMP_COLOR). This is the inverse.
 *
 * @param  value  The float taken from s_iotdev_zigbee_device_state_t.value[]
 *                where the matching app[] entry is IOTDEV_APP_LAMP_COLOR.
 * @param  r,g,b  Out: colour, 0-255 per channel. NULL pointers are a no-op.
 */
void
iotdev_zigbee_app_color_to_rgb(float value, uint8_t *r, uint8_t *g, uint8_t *b);

/* ----------------------------------------------------------------------
 *  OBSERVER PATTERN — inbound
 * --------------------------------------------------------------------*/

/**
 * @brief  Subscribe to Zigbee events.
 *
 * Multiple subscribers per event type are supported, up to
 * IOTDEV_ZIGBEE_MAX_SUBSCRIBERS in total across all event types.
 *
 * @param  event_type  e_zb_event_type_t value to listen for, or
 *                     IOTDEV_ZIGBEE_EVENT_ANY to receive all events.
 * @param  cb          Callback (must not be NULL).
 * @param  ctx         Opaque user context, passed back to the callback.
 *
 * @return Subscription handle, or IOTDEV_ZIGBEE_INVALID_SUB_HANDLE on
 *         failure (pool full, bad params).
 */
iotdev_zigbee_sub_handle_t
iotdev_zigbee_subscribe(e_zb_event_type_t              event_type,
                        iotdev_zigbee_event_callback_t cb,
                        void                          *ctx);

/**
 * @brief  Cancel a previous subscription.
 */
iotdev_zigbee_status_t
iotdev_zigbee_unsubscribe(iotdev_zigbee_sub_handle_t handle);

/**
 * @brief  Publish an event to all matching subscribers.
 *
 * Used internally by the bridge from the device manager and zb_core.
 * Application code does not normally need to call this.  Safe to call
 * from any FreeRTOS task (not from a real ISR — defer first).
 */
iotdev_zigbee_status_t
iotdev_zigbee_publish(const s_zb_event_t *event);

/* ----------------------------------------------------------------------
 *  DEVICE SNAPSHOT — blocking reads
 *  ----------------------------------------------------------------------
 *  Unlike iotdev_zigbee_request_device_state() / _array() (async, event
 *  burst), these copy a single device's data out synchronously and
 *  return it directly. They are safe to call from any FreeRTOS task:
 *  each one locates the device in zb_device_manager and copies the
 *  fields it needs while holding the device-manager lock, so the read
 *  cannot race a concurrent join/leave/rebuild on the driver task.
 *
 *  Being blocking, do not call them from an ISR, and avoid calling them
 *  from the iotdev_zigbee driver task itself in a context that already
 *  holds the device-manager lock (the lock is recursive, so re-entrant
 *  use from that task is safe, just redundant).
 * --------------------------------------------------------------------*/

/**
 * @brief  Read a device's descriptor (identity + capabilities).
 *
 * @return true      desc populated.
 * @return false     desc is NULL.
 */
bool
iotdev_zigbee_get_device_descriptor(uint64_t ieee_addr, s_iotdev_zigbee_device_descriptor_t *desc);

/**
 * @brief  Read a device's health (reachability, battery, last-seen).
 *
 * @return true      health populated.
 * @return false     health is NULL.
 */
bool
iotdev_zigbee_get_device_health(uint64_t ieee_addr, s_iotdev_zigbee_device_health_t *health);

/**
 * @brief  Read a device's cached function values (no OTA read).
 *
 * @return true      state populated.
 * @return false     state is NULL.
 */
bool
iotdev_zigbee_get_device_state(uint64_t ieee_addr, s_iotdev_zigbee_device_state_t *state);

/**
 * @brief  Approximate RSSI (dBm) from a Zigbee LQI value.
 *
 * Linear mapping used by the TI Z-Stack coprocessor: LQI 0 -> -90 dBm,
 * LQI 255 -> -20 dBm (rssi = lqi * 70 / 255 - 90).
 *
 * @note  This relationship is TI-specific. Other radio vendors derive LQI
 *        differently, so the returned value is only meaningful for TI-based
 *        coprocessors; treat it as a rough indication elsewhere.
 *
 * @param  lqi  Link quality indicator, 0..255.
 * @return Approximate RSSI in dBm (range -90..-20).
 */
int8_t
iotdev_zigbee_lqi_to_rssi(uint8_t lqi);

iotdev_zigbee_status_t
iotdev_zigbee_find_function(uint64_t ieee_addr, e_zb_function_type_t function_type, zb_sensor_id_t *sensor_id);

bool
iotdev_zigbee_rgb_to_hue_sat(uint8_t r, uint8_t g, uint8_t b, uint8_t *hue, uint8_t *saturation);

/**
 * @brief  Read the Zigbee coprocessor's (CC2652) firmware version.
 *
 * Answers from the snapshot the core caches when it reaches the ZNP, so it
 * costs no on-air or UART traffic and is safe to call from any task.
 *
 * @param  out      Buffer for a dotted "major.minor.maint" string.
 * @param  out_len  Size of @p out; 12 bytes is always enough.
 *
 * @return true   version written to @p out.
 * @return false  the coprocessor has not been reached yet (or bad params);
 *                @p out is set to an empty string when it can hold one.
 */
bool
iotdev_zigbee_get_coprocessor_version(char *out, size_t out_len);

/* ========================================================================
 *  FIRMWARE UPDATE - PROGRESS SNAPSHOTS
 *  ----------------------------------------------------------------------
 *  Both kinds of update run for minutes, so a UI needs to poll rather than
 *  wait. The facade keeps the latest ZB_EVENT_COORDINATOR_FW_UPDATE_* and
 *  ZB_EVENT_DEVICE_OTA_* it saw and hands them out here; subscribing is still
 *  the way to react promptly.
 *
 *  The two are tracked separately because they are independent operations on
 *  independent targets - the coprocessor is the hub's own radio, a device OTA
 *  is one joined device being flashed over the air.
 * ====================================================================== */
typedef enum e_iotdev_zigbee_ota_state
{
    IOTDEV_ZIGBEE_OTA_IDLE = 0,  /**< no update since boot           */
    IOTDEV_ZIGBEE_OTA_RUNNING,   /**< started, percent is meaningful */
    IOTDEV_ZIGBEE_OTA_DONE,      /**< finished successfully          */
    IOTDEV_ZIGBEE_OTA_FAILED,    /**< failed or aborted              */
} e_iotdev_zigbee_ota_state_t;

typedef struct s_iotdev_zigbee_ota_status
{
    e_iotdev_zigbee_ota_state_t state;
    uint8_t  percent;    /**< 0-100, last reported                          */
    uint8_t  phase;      /**< ZB_OTA_PROGRESS_PHASE_* (download/verify/erase) */
    uint64_t ieee_addr;  /**< device being flashed; 0 for the coprocessor    */
} s_iotdev_zigbee_ota_status_t;

/**
 * @brief  Read the latest coprocessor firmware-update progress.
 *
 * Never fails: with no update since boot the state is IDLE and percent 0.
 * ieee_addr is always 0 - the target is the hub's own coprocessor.
 * Safe to call from any task.
 */
void
iotdev_zigbee_get_coprocessor_ota_status(s_iotdev_zigbee_ota_status_t *out);

/**
 * @brief  Read the latest Zigbee device OTA progress.
 *
 * Same contract as the coprocessor call; ieee_addr names the device the OTA
 * server is serving (or last served).
 */
void
iotdev_zigbee_get_device_ota_status(s_iotdev_zigbee_ota_status_t *out);

#ifdef __cplusplus
}
#endif

#endif /* COMPONENTS_IOTDEV_ZIGBEE_INCLUDE_IOTDEV_ZIGBEE_H_ */
