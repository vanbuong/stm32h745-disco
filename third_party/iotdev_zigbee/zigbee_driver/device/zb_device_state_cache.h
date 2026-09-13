/*
 * zb_device_state_cache.h
 *
 * Last-known device state, persisted across reboots.
 *
 * The device database (<IEEE>.pb) records what a device IS - identity,
 * endpoints, capabilities - and the desired-config sidecar (<IEEE>.cfg) what we
 * want it to DO. Neither holds what it last reported, so every restart came up
 * with an empty table until each device happened to speak again.
 *
 * For a mains router that hardly matters: it is read once at join and answers
 * immediately. For a sleepy end device it matters a lot - it cannot be read at
 * all (zb_core_query_device_task skips end devices), so a battery sensor
 * reporting every 30 minutes leaves its row blank for up to half an hour after
 * every reboot. That gap is what this file closes.
 *
 * Design notes
 * ------------
 *  • ONE file for every device, not one per device. A flush writes the whole
 *    snapshot anyway, so per-device files would mean N small writes instead of
 *    one, and LittleFS metadata dominates on small files.
 *
 *  • SEPARATE from <IEEE>.pb, deliberately. Identity is written on join and
 *    parent change - rare; state is written every few minutes. Merging them
 *    would rewrite identity on every state flush and risk losing data that is
 *    expensive to reacquire (a whole interview) in order to protect data that
 *    is cheap to lose. zigbee2mqtt splits these the same way: database.db for
 *    the registry, state.json for state, saved on its own 5-minute timer.
 *
 *  • Values are cached as s_zb_func_value_t - the driver-neutral form - so the
 *    cache does not care how any particular driver lays out its context.
 *
 *  • Transient readings are NOT cached: see zb_device_state_cache_is_cacheable().
 *
 *  • The record table lives in external RAM (PSRAM where the SoC has it): it
 *    is ~84 KB, touched only on restore and on the five-minute flush, so
 *    internal DRAM is the wrong home for it.
 *
 * Author: Vo Van Buong (BRT-SG)
 */

#ifndef ZB_DEVICE_STATE_CACHE_H_
#define ZB_DEVICE_STATE_CACHE_H_

#ifdef __cplusplus
extern "C"
{
#endif /* __cplusplus */

#include "common/zb_common.h"
#include "common/zb_common_types.h"
#include "device/zb_device.h"

/** Devices held in the cache. Sized for a realistic network rather than
 *  ZB_MAX_DEVICE: the file is rewritten whole on every flush. */
#define ZB_STATE_CACHE_MAX_DEVICES      ZB_MAX_DEVICE

/** Values kept per device; the rest of a very rich device is dropped. */
#define ZB_STATE_CACHE_MAX_VALUES       ZB_MAX_FUNCTIONS

/**
 * @brief Load the cache from flash. Call once, before devices are loaded.
 *
 * A missing, truncated or CRC-failed file is not an error - the cache simply
 * starts empty, which is the pre-existing behaviour.
 */
void zb_device_state_cache_init(void);

/**
 * @brief Push cached values into a freshly built device's functions.
 *
 * Call after device_manager_build_functions() has run for @p device: the
 * functions must exist for their contexts to be populated. Values whose
 * function is no longer present (device re-interviewed, firmware changed) are
 * dropped.
 *
 * Restored values are marked stale - see zb_device_manager_value_is_stale() -
 * so a consumer can say "23.5 C, 2 h ago" rather than implying it is live.
 *
 * Also restores last_seen, LQI and liveness. Liveness is re-derived from the
 * age of the cached last_seen via zb_device_manager_status_from_last_seen()
 * rather than replayed verbatim, so a device that dropped off the network
 * while the hub was powered down comes back OFFLINE, not ONLINE.
 */
void zb_device_state_cache_restore(s_zb_device_t *device);

/**
 * @brief Note that something changed and the cache is out of date.
 *
 * Cheap: sets a flag. The actual snapshot is taken at flush time, so a device
 * reporting every second costs one flag write per report, not a serialise.
 */
void zb_device_state_cache_mark_dirty(void);

/**
 * @brief Periodic hook; writes the cache when dirty and the interval elapsed.
 *
 * Call from the driver task's housekeeping. @p now_ms is the OSAL monotonic
 * clock.
 */
void zb_device_state_cache_tick(uint32_t now_ms);

/**
 * @brief Write the cache now, regardless of the interval.
 *
 * For the paths that know a reboot is coming - a coprocessor firmware update,
 * an application OTA - where waiting for the next tick would lose the window.
 */
void zb_device_state_cache_flush(void);

/**
 * @brief Forget a device's cached state (called when it is removed).
 */
void zb_device_state_cache_forget(uint64_t ieee_addr);

/**
 * @brief True for readings worth persisting.
 *
 * Cached: sensor measurements, battery, actuator position (on/off, level,
 * colour - a lamp, a smart plug, a relay) and the metering that rides along
 * with a plug or outlet. An actuator is re-read within seconds of the network
 * coming up, so its restored value is short-lived - it exists to cover exactly
 * those seconds, in which every lamp and plug would otherwise show a blank row.
 * Restored values are flagged stale until the device next publishes a value,
 * and nothing is ever commanded on the strength of a restored value.
 *
 * Momentary and alarm-like readings are excluded on purpose:
 *
 *   ZB_FUNC_BUTTON      - an action is an event, not a state; restoring one
 *                         would replay a press that happened before the reboot.
 *   ZB_FUNC_OCCUPANCY   - restoring "occupied" fires occupancy automations with
 *   ZB_FUNC_IAS_*         nobody in the room. zigbee2mqtt has a standing
 *                         complaint about exactly this (issue #4225); these
 *                         come back as unknown and wait for the device.
 */
bool zb_device_state_cache_is_cacheable(e_zb_function_type_t type);

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* ZB_DEVICE_STATE_CACHE_H_ */
