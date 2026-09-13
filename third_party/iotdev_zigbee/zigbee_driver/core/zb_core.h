#ifndef ZB_CORE_H_
#define ZB_CORE_H_

#include <stdint.h>
#ifdef __cplusplus
extern "C"
{
#endif /* __cplusplus */

#include "common/zb_common.h"
#include "zcl/zb_zcl.h"

/* Zigbee Configuration */
#define ZB_INSTALLCODE_POLICY_ENABLE    false                   /* enable the install code policy for security */
#define ZB_HUB_CHANNEL                  11                      /* Zigbee channel */
#define ZB_HUB_PRIMARY_CHANNEL_MASK     (1l << ZB_HUB_CHANNEL) /* Zigbee primary channel mask use in the example */
#define ZB_HUB_PAN_ID                   0xFFFF                  /* Zigbee PAN ID */
#define ZB_HUB_ENDPOINT                 1                       /* Gateway endpoint identifier */
#define ZB_HUB_ZONEID                   0x01                    /* CIE Zone ID */
#define ZB_EVENT_QUEUE_SIZE             20

#define ZB_PRODTEST_FAIL                (1 << 0)
#define ZB_PRODTEST_CHAN_CHANGE         (1 << 1)
#define ZB_PRODTEST_POWER_CHANGE        (1 << 2)

void
zb_core_init(void);

void
zb_core_deinit(void);

void
zb_core_task();

bool
zb_core_is_task_running(void);

int
zb_core_get_core_temperature(int16_t *temperature);

int
zb_core_get_heap_statistics(uint32_t *free_heap, uint32_t *total_heap);

bool
zb_core_get_running_status(void);

zb_status_t
zb_core_send_event(s_zb_event_t *event);

/**
 * @brief Upper-layer event callback used to surface network / lifecycle
 *        events that don't go through the device manager.
 *
 * The event format is the same `s_zb_event_t` produced by every device
 * function - that way the upper layer (iotdev_zigbee) only has to deal
 * with one event type whether it came from a sensor function or from
 * zb_core itself.  The driver layer therefore does NOT depend on any
 * facade header.
 *
 * Sensor / actuator events from device functions reach the upper layer
 * through `zb_device_manager_register_event_notify_callback()`; this
 * hook is for events that don't have a logical function attached
 * (network formation, network state change, OTA progress, etc.).
 */
typedef void (*zb_core_event_cb_t)(const s_zb_event_t *event);

/**
 * @brief Register the upper-layer event callback.  Pass NULL to clear.
 */
void
zb_core_set_event_callback(zb_core_event_cb_t cb);

/**
 * @brief Upper-layer consumer for raw RS485 bytes received from the ZNP
 *        (MT_APP RS485 data indication).
 *
 * Keeps the driver layer free of any other-component dependency: zb_core only
 * forwards the bytes to whoever registered. The gateway wires this to the
 * Modbus RTU master (iotdev_modbus_rx_feed) inside iotdev_zigbee.c.
 */
typedef void (*zb_core_rs485_rx_cb_t)(const uint8_t *data, uint16_t len);

/**
 * @brief Register the RS485 receive consumer.  Pass NULL to clear.
 */
void
zb_core_set_rs485_rx_callback(zb_core_rs485_rx_cb_t cb);

/**
 * @brief ZNP coprocessor link-state notifications.
 *
 * A reset reboots the coprocessor, so every piece of state the host pushed into
 * it is gone - including the RS485 line configuration, which reverts to the
 * coprocessor's built-in defaults (115200 8N1). Consumers that configured the
 * coprocessor (the Modbus master owns the RS485 UART through it) must know not
 * only THAT it reset, but WHEN it is usable again.
 *
 * A reset is a window, not an instant, and a bring-up may contain several of
 * them (IDLE resets, then zb_start_network() resets again to apply the NV
 * startup options). So:
 *
 *   on_down: fired BEFORE the RESET pulse, and on detecting an unsolicited
 *            reset. The coprocessor is now unusable; consumers must stop using
 *            it and treat their pushed configuration as lost.
 *   on_up:   fired when the state machine reaches a stable state (RUNNING or
 *            STOPPED) - i.e. after the LAST reset of a sequence, never between
 *            two of them. Consumers may use the coprocessor again and should
 *            re-apply their configuration.
 *
 * Both run on the Zigbee task (zb_core_task context). Keep them short and
 * non-blocking; do NOT issue ZNP requests from inside them.
 */
typedef void (*zb_core_znp_link_cb_t)(void);

/**
 * @brief Register the ZNP link-state consumers.  Either may be NULL.
 */
void
zb_core_set_znp_link_callbacks(zb_core_znp_link_cb_t on_down, zb_core_znp_link_cb_t on_up);

/**
 * @brief Convenience helper for the driver layer to publish an event up
 *        to the registered callback (no-op if no callback is set).
 *
 * Any file in zigbee_driver may call this to surface an event without
 * including any facade header.  Example from zb_core itself:
 * @code
 *     s_zb_event_t evt = {
 *         .type = ZB_EVENT_NETWORK_INFO,
 *         .network = {
 *             .channel  = config.channel,
 *             .pan_id   = config.pan_id,
 *             .tx_power = config.tx_power,
 *             .state    = ZB_NETWORK_STATE_FORMED,
 *         },
 *     };
 *     zb_core_publish_event(&evt);
 * @endcode
 *
 * Device functions should NOT call this directly - they go through
 * `zb_device_manager_notify_event()` which fills in `ieee_addr`,
 * `sensor_id` and `name` automatically.
 */
void
zb_core_publish_event(const s_zb_event_t *event);

/* ====================================================================== *
 *  Network-level command entry points
 *  ----------------------------------------------------------------------
 *  These are the direct verbs the upper layer (iotdev_zigbee facade)
 *  invokes when it dispatches a network-level command.  They MUST be
 *  called from the same OS task that runs `zb_core_task()` - the facade
 *  guarantees this by routing every command through its internal
 *  command queue and draining it on the iotdev_zigbee task.  No
 *  internal locking is provided.
 * ====================================================================== */

/**
 * @brief Apply a runtime network configuration (TX power and, optionally,
 *        the BDB commissioning channel mask).
 *
 * @param cfg  Configuration to apply.  cfg->channel_mask == 0 means
 *             "leave channel unchanged".
 * @return ZB_OK on success, ZB_FAIL otherwise.
 */
zb_status_t
zb_core_apply_network_config(const s_zb_network_config_t *cfg, e_zb_cmd_type_t cmd_type);

/**
 * @brief Apply a default network configuration to the cached coordinator
 *        config: channel, channel mask and TX power (typically values loaded
 *        from the config file during init, before the network is formed).
 *
 *        Unlike zb_core_apply_network_config(), this only updates the cached
 *        g_zb_config; it does NOT touch the ZNP or publish events, so it is
 *        safe to call at init time before the Zigbee tasks are running. The
 *        stored values are consumed later by the form/commissioning flow
 *        (zb_set_bdb_commisioning_channel / zb_set_tx_power). Call after
 *        zb_core_init() so it overrides the built-in defaults.
 *
 * @param channel       Preferred single channel (11..26), used for reporting.
 * @param channel_mask  Commissioning channel mask; masked to ZB_ALL_CHANNEL_MASK.
 *                      If that yields 0, it is derived from @p channel.
 * @param tx_power      TX power in dBm.
 * @return ZB_OK on success, ZB_FAIL if no valid channel could be determined.
 */
zb_status_t
zb_core_apply_default_network_config(uint8_t channel, uint32_t channel_mask, int8_t tx_power);

/**
 * @brief Persist the current cached network config (channel, channel mask,
 *        TX power) to the config file in SPI flash.
 *
 *        Counterpart of zb_core_apply_default_network_config(): whatever is
 *        currently in g_zb_config is written back to CFG_ZIGBEE_CHANNEL /
 *        CFG_ZIGBEE_CHANNEL_MASK / CFG_ZIGBEE_TX_POWER so it is restored on
 *        the next boot. Values that already match the stored ones are skipped
 *        and, if nothing changed, no flash write is performed at all.
 *
 *        Call this after a successful zb_core_apply_network_config() (or once
 *        the network has formed and the final channel is known) - not on every
 *        state-machine iteration, to avoid needless flash wear.
 *
 * @return ZB_OK on success (including the "nothing changed" case),
 *         ZB_FAIL if a parameter could not be set or the flash write failed.
 */
zb_status_t
zb_core_save_network_config(void);

/**
 * @brief Request the core state machine to enter ZNP firmware update.
 *
 * The actual SBL flashing happens on the next iteration of
 * `zb_core_task()`; this call only flips the state.
 *
 * @return ZB_OK on success, ZB_FAIL if the request cannot be honoured
 *         from the current state.
 */
zb_status_t
zb_core_request_znp_fw_update(const char *fw_bin_file);

/**
 * @brief Request the core state machine to enter ZNP bootloader.
 *
 * @return ZB_OK on success, ZB_FAIL if the request cannot be honoured
 *         from the current state.
 */
zb_status_t
zb_core_request_znp_bootloader_enter(void);

/**
 * @brief Request the core state machine to start the network.
 *
 * @return ZB_OK on success, ZB_FAIL if the request cannot be honoured
 *         from the current state.
 */
zb_status_t
zb_core_request_start(void);

/**
 * @brief Request the core state machine to stop and put the ZNP back
 *        into normal (non-SBL) mode.
 *
 * The state machine transitions through ZB_STATE_STOPPING and ends in
 * ZB_STATE_STOPPED.  Subscribers will receive a
 * ZB_EVENT_NETWORK_STATE_STOP event when the stop completes.
 *
 * @return ZB_OK on success (or already stopping/stopped).
 */
zb_status_t
zb_core_request_stop(void);

/**
 * @brief Clear persisted devices and re-form the coordinator network on the
 *        next INIT cycle (ZNP NVM cleared via form_new_network).
 *
 * Safe to call while the network is up, stopped, or idle.
 *
 * @return ZB_OK always (request accepted).
 */
zb_status_t
zb_core_request_factory_reset(void);

/**
 * @brief Request a coordinator neighbour-table (Mgmt_Lqi) query - every joined
 *        device's link quality to its parent.
 *
 * Only arms the request; zb_core_task() issues it (and pages the full table) on
 * a later tick, deferring while a device interview is in progress so the
 * diagnostic never competes with commissioning. Results are logged by zb_core's
 * Mgmt_Lqi_rsp handler. Safe to call from any task.
 *
 * @return ZB_OK once the request is armed.
 */
zb_status_t
zb_core_request_lqi_query(void);

/**
 * @brief Configure whether to automatically open permit-join for 180 s
 *        right after the network forms.
 *
 * Default: ON, to preserve historical behaviour.  Production gateways
 * should turn this OFF and use ZB_CMD_NETWORK_OPEN explicitly when an
 * operator wants to admit new devices.
 *
 * @param enable  true to auto-open permit-join after forming.
 */
void
zb_core_set_auto_permit_join_on_form(bool enable);

/**
 * @brief Publish a snapshot of the current network state as a
 *        ZB_EVENT_NETWORK_INFO event up to the registered callback.
 *
 * Reads from the cached coordinator config (channel, pan_id, tx_power,
 * ieee_addr) plus the live state of the core state machine, so this is
 * safe to call whether the network is running or not.
 */
void
zb_core_publish_network_info(void);

/**
 * @brief Return the coordinator snapshot cached by the core state machine.
 *
 * Populated when the network comes up (one ZNP ext-nwk-info read into
 * g_zb_config).  Does not hit the coprocessor on each call — use
 * zb_zdo_get_coordinator_info() only when a fresh ZNP read is required.
 *
 * @return ZB_OK when ieee_addr is known, ZB_FAIL if not yet cached.
 */
zb_status_t
zb_core_get_coordinator_info(s_zb_coordinator_info_t *info);

/* ----------------------------------------------------------------------
 *  DIAGNOSTIC READS
 *  ----------------------------------------------------------------------
 *  Raw ZCL reads aimed at a human debugging a device, not at the device
 *  model: any cluster, any attribute, including ones no function owns.
 *
 *  The responses do not belong to a function, so they are handed to a
 *  registered handler rather than routed into the device model. The handler
 *  sees the parsed payload for both Read Attributes Response (0x01) and Read
 *  Reporting Configuration Response (0x09), and runs on the driver task.
 * --------------------------------------------------------------------*/

/**
 * @brief Handler for diagnostic read responses.
 *
 * @param msg  Parsed incoming message. msg->hdr.command_id says which kind:
 *             ZCL_CMD_READ_RSP -> msg->attr_cmd is s_zb_zcl_read_attr_rsp_cmd_t
 *             ZCL_CMD_READ_REPORT_CFG_RSP -> s_zb_zcl_read_report_cfg_rsp_cmd_t
 * @param ctx  Opaque context supplied at registration.
 */
typedef void (*zb_core_diag_read_cb_t)(const s_zb_zcl_incoming_msg_t *msg, void *ctx);

/**
 * @brief Install (or clear, with NULL) the diagnostic read handler.
 *
 * One handler; registering again replaces it. Called in addition to the normal
 * routing, so a read of an attribute a function does own still reaches both.
 */
void
zb_core_set_diag_read_handler(zb_core_diag_read_cb_t cb, void *ctx);

/**
 * @brief Send a Read Attributes (0x00) for arbitrary attributes.
 *
 * @param nwk_addr    Target short address.
 * @param endpoint    Target endpoint.
 * @param cluster_id  Cluster to read from.
 * @param manuf_code  Manufacturer code, 0 for a standard frame.
 * @param attr_ids    Attribute ids to read.
 * @param attr_count  How many, at most ZB_MAX_ATTRS.
 *
 * @return ZB_OK when the frame was handed to the coprocessor.
 */
zb_status_t
zb_core_diag_read_attributes(uint16_t nwk_addr, uint8_t endpoint, uint16_t cluster_id,
                             uint16_t manuf_code, const uint16_t *attr_ids, uint8_t attr_count);

/**
 * @brief Send a Read Reporting Configuration (0x08) for arbitrary attributes.
 *
 * Asks the device what reporting it actually has configured - min/max interval
 * and reportable change - which is the only way to find out whether a
 * Configure Reporting was honoured, clamped or ignored.
 *
 * Direction is always "reported" (0x00): the values the device sends us.
 * There is no manufacturer-specific variant of this command in the ZCL layer,
 * so unlike the attribute read it takes no manuf_code.
 */
zb_status_t
zb_core_diag_read_reporting_config(uint16_t nwk_addr, uint8_t endpoint, uint16_t cluster_id,
                                   const uint16_t *attr_ids, uint8_t attr_count);

/**
 * @brief Queue a ZDO bind (one in flight at a time; next starts after BIND_RSP).
 *
 * Binds and unbinds share one queue — see zb_core_unbind_enqueue().
 *
 * @param for_config_apply when true, completion advances the device's
 *        desired-config apply pass (zb_core_query_device_task).
 * @return ZB_OK if queued, ZB_BUFFER_FULL if the queue is full, ZB_FAIL otherwise.
 */
zb_status_t
zb_core_bind_enqueue(uint16_t dev_nwk_addr, uint64_t src_ieee, uint8_t src_endpoint,
                     uint64_t dst_ieee, uint8_t dst_endpoint, uint16_t cluster_id,
                     bool for_config_apply);

/**
 * @brief Queue a ZDO unbind (serialized with binds on the same queue).
 *
 * @return ZB_OK if queued, ZB_BUFFER_FULL if the queue is full, ZB_FAIL otherwise.
 */
zb_status_t
zb_core_unbind_enqueue(uint16_t dev_nwk_addr, uint64_t src_ieee, uint8_t src_endpoint,
                       uint64_t dst_ieee, uint8_t dst_endpoint, uint16_t cluster_id);

/**
 * @brief Feed a single decoded attribute into the normal report pipeline.
 *
 * For manufacturer-specific handlers (zigbee_driver/manu/) that decode a vendor
 * payload into standard ZCL attributes. The synthesised message gets exactly
 * the same treatment as a real report: the interview observer sees it first
 * (and caches it while the device is still being commissioned), then the owning
 * device function decodes it. Nothing downstream learns the value came from a
 * vendor frame.
 *
 * Must be called on the zb_core driver task - i.e. from inside a
 * zb_zcl_manu handler, which already runs there.
 *
 * @param src        Original message, for the source addressing.
 * @param cluster_id Standard cluster the value belongs to.
 * @param attr_id    Standard attribute id.
 * @param data_type  ZCL data type of @p value.
 * @param value      Value bytes, in ZCL wire order.
 */
void
zb_core_zcl_inject_report(const s_zb_zcl_incoming_msg_t *src, uint16_t cluster_id,
                          uint16_t attr_id, uint8_t data_type, const uint8_t *value);

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* ZB_CORE_H_ */