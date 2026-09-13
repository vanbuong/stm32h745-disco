/*
 * zb_znp_mt_util.h
 * 
 * Author: Vo Van Buong (BRT-SG)
 */

#ifndef ZB_ZNP_MT_UTIL_H_
#define ZB_ZNP_MT_UTIL_H_

#ifdef __cplusplus
extern "C"
{
#endif /* __cplusplus */

#include "common/zb_common.h"

/**************************************************************************************************
 * UTIL COMMANDS
 *************************************************************************************************/
#define ZNP_UTIL_GET_DEVICE_INFO              0x00
#define ZNP_UTIL_GET_NV_INFO                  0x01
#define ZNP_UTIL_SET_PAN_ID                   0x02
#define ZNP_UTIL_SET_CHANNEL                  0x03
#define ZNP_UTIL_SET_SECURITY_LEVEL           0x04
#define ZNP_UTIL_SET_PRE_CFG_KEY              0x05
#define ZNP_UTIL_CALLBACK_SUB_CMD             0x06
#define ZNP_UTIL_KEY_EVENT                    0x07
#define ZNP_UTIL_TIME_ALIVE                   0x09
#define ZNP_UTIL_LED_CONTROL                  0x0A

TYPEDEF_STRUCT_PACKED s_zb_znp_mt_util_get_device_info_rsp
{
    uint64_t ieee_addr;
    uint16_t short_addr;
    uint8_t device_type;
    uint8_t device_state;
    uint8_t num_assoc_devices;
    uint16_t *assoc_devices;
} s_zb_znp_mt_util_get_device_info_rsp_t;

TYPEDEF_STRUCT_PACKED s_zb_znp_mt_util_get_nv_info_rsp
{
    uint64_t ieee_addr;
    uint8_t scan_channel[4];
    uint16_t pan_id;
    uint8_t security_level;
    uint8_t pre_config_key[16];
} s_zb_znp_mt_util_get_nv_info_rsp_t;

void zb_znp_mt_util_process(const uint8_t *rpc_buff, uint8_t rpc_len);
int zb_znp_mt_util_get_device_info(s_zb_znp_mt_util_get_device_info_rsp_t *rsp);
int zb_znp_mt_util_get_nv_info(s_zb_znp_mt_util_get_nv_info_rsp_t *rsp);

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* ZB_ZNP_MT_UTIL_H_ */