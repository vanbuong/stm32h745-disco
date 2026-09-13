#ifndef ZB_DEVICE_MANAGER_H_
#define ZB_DEVICE_MANAGER_H_

#ifdef __cplusplus
extern "C"
{
#endif /* __cplusplus */

#include "common/zb_common.h"
#include "device/zb_device.h"

zb_status_t zb_device_manager_init(void);
zb_status_t zb_device_manager_load_device_file(void);
zb_status_t zb_device_manager_update_device_network_address(void);
zb_status_t zb_device_manager_delete_device_file(uint64_t ieee_addr);
zb_status_t zb_device_manager_save_device_file(s_zb_device_t *device);

/******************************************************************************
 * Desired configuration (bind / attribute reporting) - host-held intent,
 * persisted to a "<IEEE>.cfg" sidecar and re-applied when the network is up or
 * the device re-joins (a factory-reset device loses its own tables).
 ******************************************************************************/
zb_status_t zb_device_manager_load_config_file(s_zb_device_t *device);
zb_status_t zb_device_manager_save_config_file(s_zb_device_t *device);
void        zb_device_manager_record_desired_config(s_zb_device_t *device, const s_zb_desired_config_t *cfg);
void        zb_device_manager_remove_desired_config(s_zb_device_t *device, const s_zb_desired_config_t *cfg);
zb_status_t zb_device_manager_apply_desired_config_entry(s_zb_device_t *device,
        const s_zb_desired_config_t *cfg, bool for_config_apply);
void        zb_device_manager_apply_desired_config(s_zb_device_t *device);
void        zb_device_manager_apply_poll_overrides(s_zb_device_t *device);

zb_status_t zb_device_manager_add_device(s_zb_device_info_t *device_info);
zb_status_t zb_device_manager_remove_device(uint64_t ieee_addr);
zb_status_t zb_device_manager_update_device(s_zb_device_info_t *device_info);
/** Already-commissioned device rejoined: refresh nwk addr and re-arm bind/report apply. */
void        zb_device_manager_on_device_rejoined(s_zb_device_t *device,
                                               uint16_t nwk_addr, uint16_t parent_nwk_addr);
zb_status_t zb_device_manager_remove_all_devices(void);

/**
 * @brief Record that a frame was just received from a device.
 *
 * Updates last_seen, refreshes the link-quality (LQI) reading, and sets
 * status to ZB_DEVICE_STATUS_ONLINE. Called from the driver-task AF path for every incoming
 * frame, so liveness is tracked even for frames that map to no function.
 *
 * @param ieee_addr  Device that sent the frame.
 * @param lqi        ZNP-reported link quality of that frame (0-255).
 */
void zb_device_manager_note_activity(uint64_t ieee_addr, uint8_t lqi);

/**
 * @brief How long a device may stay silent before it counts as offline.
 *
 * Mirrors zigbee2mqtt's split: a mains-powered router is expected to answer
 * promptly, so it gets a short window; a sleepy/battery device reports on its
 * own slow schedule and must not be declared dead between reports.
 *
 * @param device  Device to classify.
 * @return Timeout in seconds.
 */
uint32_t zb_device_manager_availability_timeout_s(const s_zb_device_t *device);

/**
 * @brief Derive liveness from the age of a last_seen stamp.
 *
 * Shared by the periodic sweep and the state-cache restore so a device that
 * was online before a reboot is only announced as online if it would still be
 * within its timeout now.
 *
 * @param device     Device the stamp belongs to (for its timeout class).
 * @param last_seen  UTC epoch seconds of the last frame, 0 if never heard.
 * @return ONLINE, OFFLINE, or UNKNOWN when the stamp or the clock is unusable.
 */
e_zb_device_status_t zb_device_manager_status_from_last_seen(const s_zb_device_t *device,
                                                             uint64_t last_seen);

/**
 * @brief Periodic liveness sweep; marks silent devices offline.
 *
 * A mains device past its timeout is pinged (Basic read) and only written off
 * if it does not answer within the grace window; a battery/sleepy device is
 * written off on the timeout alone, since waking it to ask would defeat the
 * purpose. Check intervals carry a per-device jitter so a batch of devices
 * that went quiet together is not pinged in one burst, and each failed check
 * widens the next interval (x1.5, x3, x6, x12) so a device that has genuinely
 * gone is not polled forever.
 *
 * Devices come back ONLINE through zb_device_manager_note_activity(), which
 * also clears the pending ping and the backoff ladder. Call from the driver
 * task's housekeeping.
 *
 * @param now_ms  OSAL monotonic clock.
 */
void zb_device_manager_availability_tick(uint32_t now_ms);

/**
 * @brief Look up a device by IEEE and hold the device-manager lock.
 *
 * On return the manager lock is HELD (whether or not the device was found), so
 * the caller may safely read the returned record. The caller MUST call
 * zb_device_manager_release_device() exactly once afterwards, on every path,
 * including when NULL is returned.
 *
 * Intended for an upper layer that holds only an IEEE address and wants to copy
 * fields out of the live record synchronously (e.g. the facade's
 * get-device-info wrapper). Because device allocation/removal take the same
 * lock, the record cannot be freed while the caller holds it.
 *
 * @param ieee_addr  Device to look up.
 * @return Pointer to the live device record, or NULL if not present. Valid only
 *         until zb_device_manager_release_device() is called.
 */
const s_zb_device_t *zb_device_manager_acquire_device(uint64_t ieee_addr);

/**
 * @brief Release the lock taken by zb_device_manager_acquire_device().
 *        Any pointer returned by that call is invalid after this returns.
 */
void zb_device_manager_release_device(void);

/**
 * @brief Copy a device's cached function values, read straight from the
 *        function contexts.
 *
 * One entry per function. Functions that carry no reportable value - basic
 * info, range extender, and a button that has not been pressed yet - are
 * skipped, so the count may be lower than device->function_count.
 *
 * @param ieee_addr  Device to read.
 * @param out        Destination array; must hold at least ZB_MAX_FUNCTIONS entries.
 * @return Number of entries written to out[], or 0 if no device is joined with
 *         this IEEE (or none of its functions has a readable value).
 */
uint8_t zb_device_manager_read_function_values(uint64_t ieee_addr, s_zb_func_value_t *out);

/******************************************************************************
 * Event notification callback
 ******************************************************************************/

typedef void (*zb_event_notify_callback_t)(const s_zb_event_t *event);

void zb_device_manager_register_event_notify_callback(zb_event_notify_callback_t callback);
void zb_device_manager_notify_event(s_zb_device_t *device, s_zb_function_t *func, s_zb_event_t *event);

/**
 * Publish a full snapshot of the current device list as a framed event burst:
 *   ZB_EVENT_DEVICE_LIST_BEGIN → N × ZB_EVENT_DEVICE_LIST_ITEM → ZB_EVENT_DEVICE_LIST_END,
 * every event carrying @p txn_id so the consumer can correlate the burst with
 * its ZB_CMD_DEVICE_LIST request.
 *
 * MUST be called from the driver task (the same task that runs zb_core_task).
 * The burst is emitted while holding the device-manager mutex so concurrent
 * OTA lookups cannot mutate the pool mid-snapshot.
 */
void zb_device_manager_publish_device_list(uint32_t txn_id);

/*
 * Emit a cached-state snapshot: for @p ieee_addr (or every device when it is 0),
 * re-publish each function's current cached value as its normal event, bracketed
 * by ZB_EVENT_DEVICE_STATE_BEGIN / _END carrying @p txn_id. No OTA reads are
 * issued, so it works even when the network is down. Same threading contract as
 * zb_device_manager_publish_device_list(): driver task, mutex held for the burst.
 */
void zb_device_manager_publish_device_state(uint64_t ieee_addr, uint32_t txn_id);

/*
 * Array-form of the cached snapshot: one ZB_EVENT_DEVICE_STATE per device (all
 * of that device's function values in a single event), for @p ieee_addr or every
 * device when 0, bracketed by ZB_EVENT_DEVICE_STATE_BEGIN/_END with @p txn_id.
 * Same threading contract as the burst variant.
 */
void zb_device_manager_publish_device_state_array(uint64_t ieee_addr, uint32_t txn_id);

/******************************************************************************
 * Lookup functions
 ******************************************************************************/

s_zb_device_t * zb_device_manager_find_by_ieee(uint64_t ieee_addr);
s_zb_device_t * zb_device_manager_find_by_short_addr(uint16_t short_addr);

/** Update a device's parent (short + IEEE) from an authoritative source such as
 *  a ZDO Mgmt_Lqi neighbour-table CHILD entry. No-op if the child is unknown. */
void zb_device_manager_set_parent(uint64_t child_ieee, uint16_t parent_nwk, uint64_t parent_ieee);

/** Update only a device's link quality (LQI); last_seen / online are untouched.
 *  Used for a device measured by another node (a Mgmt_Lqi neighbour entry) that
 *  did not itself transmit to us. No-op if the device is unknown. */
void zb_device_manager_set_lqi(uint64_t ieee_addr, uint8_t lqi);
/** Copy IEEE under lock; safe for cross-task use (e.g. OTA task). */
bool             zb_device_manager_get_ieee_by_short_addr(uint16_t short_addr, uint64_t *ieee_out);
s_zb_device_t * zb_device_manager_find_device_from_start_index(uint16_t *index);
s_zb_function_t * zb_device_manager_find_function(s_zb_device_t *device, zb_sensor_id_t sensor_id);
s_zb_device_endpoint_t * zb_device_manager_find_endpoint_by_dev_cluster(s_zb_device_info_t *device_info, uint16_t cluster_id);
/** ColorCapabilities (Color Control 0x400A) of one endpoint, as read during
 *  the join interview and persisted with the device. Returns 0 when the
 *  endpoint is unknown or the attribute was never read back, which callers
 *  should treat as "capabilities unknown", not as "no capabilities". */
uint16_t zb_device_manager_endpoint_color_caps(const s_zb_device_t *device, uint8_t endpoint_id);
/** Physical colour-temperature limits (mireds) of one endpoint, as read during
 *  the join interview. Returns false when the endpoint is unknown or the light
 *  never reported them - the range is then unknown, not empty. */
bool zb_device_manager_endpoint_color_temp_range(const s_zb_device_t *device, uint8_t endpoint_id,
                                                 uint16_t *min_out, uint16_t *max_out);
/** First endpoint whose in-cluster list contains @p cluster_id, from the join
 *  interview's Simple Descriptors. Returns 0 (never a valid application
 *  endpoint) when no endpoint offers it. Lets a caller address a cluster
 *  without knowing the device's endpoint layout. */
uint8_t zb_device_manager_find_endpoint_with_cluster(const s_zb_device_t *device, uint16_t cluster_id);

/**
 * @brief Push a cached value back into a function's driver context.
 *
 * The mirror of the internal reader used by
 * zb_device_manager_read_function_values(): both live in zb_device_manager.c
 * because that is where the per-driver context layouts are already known, so
 * restoring state costs no new per-driver hook.
 *
 * Only the reading types worth persisting are handled - see
 * zb_device_state_cache_is_cacheable().
 *
 * @return true when the value was written; false for a type this cannot
 *         restore, a mismatched value, or a function with no context.
 */
bool zb_device_manager_write_function_value(s_zb_function_t *func, const s_zb_func_value_t *in);

/** True when this function's value came from the state cache and the device
 *  has not reported since. */
bool zb_device_manager_value_is_stale(const s_zb_function_t *func);

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* ZB_DEVICE_MANAGER_H_ */