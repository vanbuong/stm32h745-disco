/*
 * zb_af.h
 * 
 * Author: Vo Van Buong (BRT-SG)
 */

#ifndef ZB_AF_H
#define ZB_AF_H

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

#include "common/zb_common.h"
#include "znp/zb_znp_mt_af.h"

#define AF_LONG_ADDR_LEN              8
#define AF_DEFAULT_RADIUS             (2 * 0x0F)

#define AF_DATA_STORE_MAX_LEN         247
#define AF_DATA_REQ_HDR_LEN           20

#define AF_BROADCAST_ENDPOINT         0xFF

#define AF_WILDCARD_PROFILE_ID        0x02
#define AF_PREPROCESS                 0x04
#define AF_LIMIT_CONCENTRATOR         0x08
#define AF_ACK_REQUEST                0x10
#define AF_SUPPRESS_ROUTE_DISC_NWK    0x20
#define AF_EN_SECURITY                0x40
#define AF_SKIP_ROUTING               0x80

#define AF_TX_OPTION_NONE             0

typedef enum e_zb_af_address_mode
{
    AF_ADDRESS_NOT_PRESENT = 0,
    AF_ADDRESS_GROUP = 1,
    AF_ADDRESS_16BIT = 2,
    AF_ADDRESS_64BIT = 3,
    AF_ADDRESS_BROADCAST = 15,
} e_zb_af_address_mode_t;

typedef enum e_zb_af_status
{
    AF_STATUS_SUCCESS = ZB_SUCCESS,
    AF_STATUS_FAILURE = ZB_FAILURE,
    AF_STATUS_INVALID_PARAMETER = ZB_INVALID_PARAMETER,
    AF_STATUS_MEM_ERROR = ZB_MEM_ERROR,
    AF_STATUS_NWK_NO_ROUTE = ZB_NWK_NO_ROUTE,
} e_zb_af_status_t;

typedef struct s_zb_af_address
{
    union
    {
        uint16_t short_addr;
        uint64_t long_addr;
    };
    e_zb_af_address_mode_t address_mode;
    uint8_t endpoint;
    uint16_t pan_id; // Used for the INTER_PAN feature
} s_zb_af_address_t;

typedef struct s_zb_af_msg_command
{
    uint16_t data_length;
    uint8_t *data;
} s_zb_af_msg_command_t;

typedef struct s_zb_af_incoming_msg
{
    uint16_t group_id;
    uint16_t cluster_id;
    s_zb_af_address_t src_addr;
    uint8_t src_endpoint;
    uint8_t dst_endpoint;
    uint8_t was_broadcast;
    uint8_t link_quality;
    uint8_t security_use;
    uint32_t timestamp;
    s_zb_af_msg_command_t command;
    uint16_t mac_src_addr;
    uint8_t radius;
} s_zb_af_incoming_msg_t;

typedef struct s_zb_af_data_confirm
{
    uint8_t endpoint;
    uint8_t trans_id;
    uint16_t cluster_id;
} s_zb_af_data_confirm_t;

typedef struct s_zb_af_reflect_error
{
    uint8_t endpoint;       // destination endpoint
    uint8_t trans_id;       // transaction ID of sent message
    uint8_t dst_addr_mode;  // destination address mode: 0 - short address, 1 - group address
    uint16_t dst_addr;      // destination address - depends on dst_addr_mode
} s_zb_af_reflect_error_t;

/** Free an AF incoming message and its command payload (use for messages from zb_znp_mt_af_incoming_msg_cb). */
void zb_af_incoming_msg_free(s_zb_af_incoming_msg_t *msg);

/* AF/APS transaction ID management.  The transaction id correlates the local
 * AF_DATA_CONFIRM back to its afDataRequest and is assigned inside
 * zb_af_data_req().  It is intentionally separate from the over-the-air ZCL
 * transaction sequence number (see zb_zcl_next_seq_num()). */
void zb_af_trans_id_init(void);

/* Original AF data request function (for backward compatibility) */
int zb_af_data_req(
    s_zb_af_address_t *dst_addr, uint16_t src_endpoint,
    uint16_t cluster_id, uint8_t *buff, uint16_t buff_len,
    uint8_t options, uint8_t radius);

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* ZB_AF_H */
