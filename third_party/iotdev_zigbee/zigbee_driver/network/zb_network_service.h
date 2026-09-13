/*
 * zb_network_service.h
 * 
 * Author: Vo Van Buong (BRT-SG)
 */

#ifndef ZB_NETWORK_SERVICE_H_
#define ZB_NETWORK_SERVICE_H_

#include "device/zb_device_db.h"
#include "znp/zb_znp_mt_zdo.h"

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

/* Number of state machine will be added for processing new device when current state machines are full */
#define ZB_NWKSRV_MAX_STATE_MACHINES            8

/* Number of tries to issue message to remote decive without handling device status */
#define ZB_NWKSRV_MAX_FAILED_ATTEMPTS           3
#define ZB_NWKSRV_MAX_IAS_ZONE_FAILED_ATTEMPTS  20

/* Time in millis seconds of 1 timer tick */
#define ZB_NWKSRV_ONE_TICK_TIME                 250

/* Total waiting time in each state for waiting response */
#define ZB_NWKSRV_WAITING_TIME                  8000

/* Number of tick state machine need to wait for response */
#define ZB_NWKSRV_WAITING_TICKS                 (ZB_NWKSRV_WAITING_TIME / ZB_NWKSRV_ONE_TICK_TIME)

/* State machine errors (for callback) */
#define ZB_NWKSRV_ADS_OK                        0
#define ZB_NWKSRV_ADS_NO_RSP                    1   /* One or more of the zigbee calls did not respond */
#define ZB_NWKSRV_ADS_NO_MEM                    2   /* Couldn't allocate structures or state machine */
#define ZB_NWKSRV_ADS_BUSY                      3   /* Already busy on this device ID */

/* Event queue size */
#define ZB_NWKSRV_EVENT_QUEUE_LENGTH            32

/* Source of network event */
#define ZB_NWKSRV_EVENT_SOURCE_DEVICE_UPDATE    0
#define ZB_NWKSRV_EVENT_SOURCE_DEVICE_REMOVE    1
#define ZB_NWKSRV_EVENT_SOURCE_DEVICE_ANNCE     2
#define ZB_NWKSRV_EVENT_SOURCE_IEEE_ADDR        3
#define ZB_NWKSRV_EVENT_SOURCE_NWK_ADDR         4
#define ZB_NWKSRV_EVENT_SOURCE_NODE_DESC        5
#define ZB_NWKSRV_EVENT_SOURCE_ACTIVE_EP        6
#define ZB_NWKSRV_EVENT_SOURCE_SIMPLE_DESC      7

typedef struct s_zb_nwksrv_event_element_s
{
    uint8_t event_source;
    void *response;
} s_zb_nwksrv_event_element_t;

/* Result of offering an incoming ZCL message to the interview state machines. */
typedef enum e_zb_zcl_observe
{
    ZB_ZCL_OBS_IGNORED,   /* No interview cares      -> normal path continues  */
    ZB_ZCL_OBS_OBSERVED,  /* Interview took the data -> normal path continues  */
    ZB_ZCL_OBS_CONSUMED,  /* Interview owns the frame -> stop processing       */
} e_zb_zcl_observe_t;

/**
 * @brief Offer a parsed ZCL message to the device-interview state machines.
 *
 * Called from the single ZCL pipeline in zb_core BEFORE the device-pool lookup,
 * because a device under interview is not in the pool yet and would otherwise
 * have all of its frames dropped.
 *
 * - Read/Write responses that an interview is waiting for are CONSUMED: the
 *   state machine advances and no further processing happens.
 * - Attribute reports from a device under interview are OBSERVED: the values
 *   are cached into the state machine's device_info (see s_zb_cached_attr_t)
 *   so they survive to be seeded into the functions at add_device() time, and
 *   the message still continues down the normal path.
 * - Everything else is IGNORED and behaves exactly as before.
 *
 * @param msg  Incoming ZCL message with msg->attr_cmd already parsed.
 */
e_zb_zcl_observe_t zb_nwksrv_zcl_observe(s_zb_zcl_incoming_msg_t *msg);


/* Function that will be called when the state machine is complete. After this function is called. device_info is freed */
typedef void (*pfn_zb_nwksrv_add_device_cb)(s_zb_device_info_t *device_info, uint64_t ieee_addr, int err);

int zb_nwksrv_init(void);
int zb_nwksrv_deinit(void);
void zb_nwksrv_task(void);

/**
 * @brief True while at least one device interview (join/commissioning) is in
 *        progress. Low-priority diagnostics (e.g. Mgmt_Lqi queries) should defer
 *        while this is true to avoid backing up the ZNP and expiring an
 *        interview frame.
 */
bool zb_nwksrv_is_commissioning(void);
int zb_nwksrv_event_queue_put(s_zb_nwksrv_event_element_t *element);
void zb_nwksrv_ad_process_device_announce_signal(const uint64_t ieee_addr);
void zb_nwksrv_ad_process_device_update_signal(s_zb_znp_mt_zdo_tc_device_ind_t *tc_dev_ind);
void zb_nwksrv_ad_process_device_remove_signal(s_zb_znp_mt_zdo_leave_ind_t *leave_ind);
void zb_nwksrv_add_device_info_cb(s_zb_device_info_t *device_info, uint64_t ieee_addr, int err);
void zb_nwksrv_add_device(
    uint64_t ieee_addr,
    s_zb_znp_mt_zdo_tc_device_ind_t *tc_dev_ind,
    pfn_zb_nwksrv_add_device_cb pfn_add_device_cb);

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* ZB_NETWORK_SERVICE_H_ */
