/*
 * zb_znp_mt_af.h
 *
 * This module contains the definitions and function prototypes for the ZNP MT AF Interface.
 *
 * Author: Vo Van Buong (BRT-SG)
 */

#ifndef ZB_ZNP_MT_AF_H_
#define ZB_ZNP_MT_AF_H_

#ifdef __cplusplus
extern "C"
{
#endif /* __cplusplus */

#include "common/zb_common.h"

#define ZNP_LONG_ADDR_LEN 8

/**************************************************************************************************
 * AF COMMANDS
 *************************************************************************************************/
#define ZNP_AF_REGISTER                       0x00
#define ZNP_AF_DATA_REQUEST                   0x01  /* AREQ optional, but no AREQ response. */
#define ZNP_AF_DATA_REQUEST_EXT               0x02  /* AREQ optional, but no AREQ response. */
#define ZNP_AF_DATA_REQUEST_SRC_RTG           0x03

#define ZNP_AF_INTER_PAN_CTL                  0x10
#define ZNP_AF_DATA_STORE                     0x11
#define ZNP_AF_DATA_RETRIEVE                  0x12
#define ZNP_AF_APSF_CONFIG_SET                0x13

/* AREQ to host */
#define ZNP_AF_DATA_CONFIRM                   0x80
#define ZNP_AF_INCOMING_MSG                   0x81
#define ZNP_AF_INCOMING_MSG_EXT               0x82
#define ZNP_AF_REFLECT_ERROR                  0x83

#define ZNP_AF_STATUS_SUCCESS                 0x00
#define ZNP_AF_STATUS_FAILED                  0x01
#define ZNP_AF_STATUS_INVALID_PARAMETER       0x02
#define ZNP_AF_STATUS_MEM_FAIL                0x10

typedef enum e_zb_znp_af_nwk_latency_req
{
    AF_NWK_LATENCY_REQ_NO_LATENCY = 0,
    AF_NWK_LATENCY_REQ_FAST_BEACON = 1,
    AF_NWK_LATENCY_REQ_SLOW_BEACON = 2,
} e_zb_znp_af_nwk_latency_req_t;

typedef struct s_zb_znp_mt_af_register_cmd
{
    uint8_t endpoint;
    uint16_t profile_id;
    uint16_t device_id;
    uint8_t device_version;
    uint8_t latency;
    uint8_t num_in_clusters;
    uint16_t in_cluster_list[16];
    uint8_t num_out_clusters;
    uint16_t out_cluster_list[20];
} s_zb_znp_mt_af_register_cmd_t;

TYPEDEF_STRUCT_PACKED s_zb_znp_mt_af_data_request_cmd
{
    uint16_t dst_addr;
    uint8_t dst_endpoint;
    uint8_t src_endpoint;
    uint16_t cluster_id;
    uint8_t trans_id;
    uint8_t options;
    uint8_t radius;
    uint8_t len;
    uint8_t data[];
} s_zb_znp_mt_af_data_request_cmd_t;

TYPEDEF_STRUCT_PACKED s_zb_znp_mt_af_data_request_ext_cmd
{
    uint8_t dst_addr_mode;
    uint8_t dst_addr[ZNP_LONG_ADDR_LEN];
    uint8_t dst_endpoint;
    uint16_t dst_pan_id;
    uint8_t src_endpoint;
    uint16_t cluster_id;
    uint8_t trans_id;
    uint8_t options;
    uint8_t radius;
    uint16_t len;
    uint8_t data[];
} s_zb_znp_mt_af_data_request_ext_cmd_t;

typedef struct s_zb_znp_mt_af_data_request_src_rtg_cmd
{
    uint16_t dst_addr;
    uint8_t dst_endpoint;
    uint8_t src_endpoint;
    uint16_t cluster_id;
    uint8_t trans_id;
    uint8_t options;
    uint8_t radius;
    uint8_t relay_count;
    uint16_t relay_list[255];
    uint8_t len;
    uint8_t data[128];
} s_zb_znp_mt_af_data_request_src_rtg_cmd_t;

TYPEDEF_STRUCT_PACKED s_zb_znp_mt_af_inter_pan_ctl_cmd
{
    uint8_t command;
    uint8_t data[3];
} s_zb_znp_mt_af_inter_pan_ctl_cmd_t;

TYPEDEF_STRUCT_PACKED s_zb_znp_mt_af_data_store_cmd
{
    uint16_t index;
    uint8_t length;
    uint8_t *data;
} s_zb_znp_mt_af_data_store_cmd_t;

TYPEDEF_STRUCT_PACKED s_zb_znp_mt_af_data_store_rsp
{
    uint8_t status;
} s_zb_znp_mt_af_data_store_rsp_t;

TYPEDEF_STRUCT_PACKED s_zb_znp_mt_af_data_retrieve_cmd
{
    uint8_t time_stamp[4];
    uint16_t index;
    uint8_t length;
} s_zb_znp_mt_af_data_retrieve_cmd_t;

TYPEDEF_STRUCT_PACKED s_zb_znp_mt_af_data_retrieve_rsp
{
    uint8_t status;
    uint8_t length;
    uint8_t data[253];
} s_zb_znp_mt_af_data_retrieve_rsp_t;

TYPEDEF_STRUCT_PACKED s_zb_znp_mt_af_apsf_config_set_cmd
{
    uint8_t endpoint;
    uint8_t frame_delay;
    uint8_t window_size;
} s_zb_znp_mt_af_apsf_config_set_cmd_t;

TYPEDEF_STRUCT_PACKED s_zb_znp_mt_af_data_confirm_msg
{
    uint8_t status;
    uint8_t endpoint;
    uint8_t trans_id;
} s_zb_znp_mt_af_data_confirm_msg_t;

TYPEDEF_STRUCT_PACKED s_zb_znp_mt_af_reflect_error_msg
{
    uint8_t status;
    uint8_t endpoint;
    uint8_t trans_id;
} s_zb_znp_mt_af_reflect_error_msg_t;

TYPEDEF_STRUCT_PACKED s_zb_znp_mt_af_incoming_msg
{
    uint16_t group_id;
    uint16_t cluster_id;
    uint16_t src_addr;
    uint8_t src_endpoint;
    uint8_t dst_endpoint;
    uint8_t was_broadcast;
    uint8_t link_quality;
    uint8_t security_use;
    uint32_t timestamp;
    uint8_t trans_seq_num;
    uint8_t len;
    uint8_t data[];
} s_zb_znp_mt_af_incoming_msg_t;

TYPEDEF_STRUCT_PACKED s_zb_znp_mt_af_incoming_ext_msg
{
    uint16_t group_id;
    uint16_t cluster_id;
    uint8_t src_addr_mode;
    uint8_t src_addr[ZNP_LONG_ADDR_LEN];
    uint8_t src_endpoint;
    uint16_t src_pan_id;
    uint8_t dst_endpoint;
    uint8_t was_broadcast;
    uint8_t link_quality;
    uint8_t security_use;
    uint32_t timestamp;
    uint8_t trans_seq_num;
    uint8_t len;
    uint8_t data[];
} s_zb_znp_mt_af_incoming_ext_msg_t;

typedef int (*zb_znp_mt_af_data_confirm_callback_t)(const s_zb_znp_mt_af_data_confirm_msg_t *msg);
typedef int (*zb_znp_mt_af_incoming_msg_callback_t)(const s_zb_znp_mt_af_incoming_msg_t *msg);
typedef int (*zb_znp_mt_af_incoming_msg_ext_callback_t)(const s_zb_znp_mt_af_incoming_ext_msg_t *msg);
typedef int (*zb_znp_mt_af_reflect_error_callback_t)(const s_zb_znp_mt_af_reflect_error_msg_t *msg);
typedef int (*zb_znp_mt_af_data_store_callback_t)(const s_zb_znp_mt_af_data_store_rsp_t *rsp);
typedef int (*zb_znp_mt_af_data_retrieve_callback_t)(const s_zb_znp_mt_af_data_retrieve_rsp_t *rsp);

typedef struct s_zb_znp_mt_af_cb
{
    zb_znp_mt_af_data_confirm_callback_t pfn_af_data_confirm_cb;
    zb_znp_mt_af_incoming_msg_callback_t pfn_af_incoming_msg_cb;
    zb_znp_mt_af_incoming_msg_ext_callback_t pfn_af_incoming_msg_ext_cb;
    zb_znp_mt_af_reflect_error_callback_t pfn_af_reflect_error_cb;
    zb_znp_mt_af_data_store_callback_t pfn_af_data_store_rsp_cb;
    zb_znp_mt_af_data_retrieve_callback_t pfn_af_data_retrieve_rsp_cb;
} s_zb_znp_mt_af_cb_t;

void zb_znp_mt_af_register_callback(s_zb_znp_mt_af_cb_t callbacks);
void zb_znp_mt_af_unregister_callback(void);
void zb_znp_mt_af_process(const uint8_t *rpc_buff, uint8_t rpc_len);
int zb_znp_mt_af_register(const s_zb_znp_mt_af_register_cmd_t *req_cmd);
int zb_znp_mt_af_data_request(const s_zb_znp_mt_af_data_request_cmd_t *req_cmd);
int zb_znp_mt_af_data_request_ext(const s_zb_znp_mt_af_data_request_ext_cmd_t *req_cmd);
int zb_znp_mt_af_data_request_src_rtg(const s_zb_znp_mt_af_data_request_src_rtg_cmd_t *req_cmd);
int zb_znp_mt_af_inter_pan_ctl(const s_zb_znp_mt_af_inter_pan_ctl_cmd_t *req_cmd);
int zb_znp_mt_af_data_store(const s_zb_znp_mt_af_data_store_cmd_t *req_cmd);
int zb_znp_mt_af_data_retrieve(const s_zb_znp_mt_af_data_retrieve_cmd_t *req_cmd);
int zb_znp_mt_af_apsf_config_set(const s_zb_znp_mt_af_apsf_config_set_cmd_t *req_cmd);

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* ZB_ZNP_MT_AF_H_ */