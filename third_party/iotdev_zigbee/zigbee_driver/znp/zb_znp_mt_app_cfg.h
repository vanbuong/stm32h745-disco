/*
 * zb_znp_mt_app_cfg.h
 * 
 * Author: Vo Van Buong (BRT-SG)
 */

#ifndef ZB_ZNP_MT_APP_CFG_H_
#define ZB_ZNP_MT_APP_CFG_H_

#ifdef __cplusplus
extern "C"
{
#endif /* __cplusplus */

#include "common/zb_common.h"

/**************************************************************************************************
 * APP CONFIG COMMANDS
 *************************************************************************************************/
// SREQ
#define ZNP_APP_CFG_SET_NWK_FRAME_COUNTER                  0xFF
#define ZNP_APP_CFG_SET_DEFAULT_REMOTE_END_DEVICE_TIMEOUT  0x01
#define ZNP_APP_CFG_SET_END_DEVICE_TIMEOUT                 0x02
#define ZNP_APP_CFG_SET_ALLOW_REJOIN_TC_POLICY             0x03
#define ZNP_APP_CFG_BDB_ADD_INSTALL_CODE                   0x04
#define ZNP_APP_CFG_BDB_START_COMMISSIONING                0x05
#define ZNP_APP_CFG_BDB_SET_JOIN_USE_INSTALL_CODE_KEY      0x06
#define ZNP_APP_CFG_BDB_SET_ACTIVE_DEFAULT_CENTRALIZED_KEY 0x07
#define ZNP_APP_CFG_BDB_SET_CHANNEL                        0x08
#define ZNP_APP_CFG_SET_TC_REQUIRE_KEY_EXCHANGE            0x09
#define ZNP_APP_CFG_BDB_ZED_ATTEMPT_RECOVER_NWK            0x0A

// ARESP
#define ZNP_APP_CFG_BDB_COMMISIONING_NOTIFICATION          0x80

typedef enum e_zb_znp_app_cfg_bdb_commissioning_mode
{
    ZNP_APP_CFG_BDB_COMMISIONING_MODE_INITIALIZATION = 0x00,
    ZNP_APP_CFG_BDB_COMMISIONING_MODE_TOUCHLINK = 0x01,
    ZNP_APP_CFG_BDB_COMMISIONING_MODE_NWK_STEERING = 0x02,
    ZNP_APP_CFG_BDB_COMMISIONING_MODE_NWK_FORMATION = 0x04,
    ZNP_APP_CFG_BDB_COMMISIONING_MODE_FINDING_BINDING = 0x08,
} e_zb_znp_app_cfg_bdb_commissioning_mode_t;

typedef enum e_zb_znp_app_cfg_bdb_commissioning_noti_status
{
    ZNP_APP_CFG_BDB_COMMISIONING_NOTI_SUCCESS = 0x00,
    ZNP_APP_CFG_BDB_COMMISIONING_NOTI_IN_PROGRESS = 0x01,
    ZNP_APP_CFG_BDB_COMMISIONING_NOTI_NO_NETWORK = 0x02,
    ZNP_APP_CFG_BDB_COMMISIONING_NOTI_TL_TARGET_FAILURE = 0x03,
    ZNP_APP_CFG_BDB_COMMISIONING_NOTI_TL_NOT_AA_CAPABLE = 0x04,
    ZNP_APP_CFG_BDB_COMMISIONING_NOTI_TL_NO_SCAN_RESPONSE = 0x05,
    ZNP_APP_CFG_BDB_COMMISIONING_NOTI_TL_NOT_PERMITTED = 0x06,
    ZNP_APP_CFG_BDB_COMMISIONING_NOTI_TCLK_EX_FAILURE = 0x07,
    ZNP_APP_CFG_BDB_COMMISIONING_NOTI_FORMATION_FAILURE = 0x08,
    ZNP_APP_CFG_BDB_COMMISIONING_NOTI_FB_TARGET_INPROGRESS = 0x09,
    ZNP_APP_CFG_BDB_COMMISIONING_NOTI_FB_INITIATOR_INPROGRESS = 0x0A,
    ZNP_APP_CFG_BDB_COMMISIONING_NOTI_FB_NO_IDENTIFY_QUERY_RESPONSE = 0x0B,
    ZNP_APP_CFG_BDB_COMMISIONING_NOTI_FB_BINDING_TABLE_FULL = 0x0C,
    ZNP_APP_CFG_BDB_COMMISIONING_NOTI_FB_NETWORK_RESTORED = 0x0D,
    ZNP_APP_CFG_BDB_COMMISIONING_NOTI_FAILURE = 0x0E,
} e_zb_znp_app_cfg_bdb_commissioning_noti_status_t;

typedef enum e_zb_znp_app_cfg_bdb_commissioning_mode_noti
{
    ZNP_APP_CFG_BDB_COMMISIONING_NOTI_INITIALIZATION = 0x00,
    ZNP_APP_CFG_BDB_COMMISIONING_NOTI_NWK_STEERING = 0x01,
    ZNP_APP_CFG_BDB_COMMISIONING_NOTI_FORMATION = 0x02,
    ZNP_APP_CFG_BDB_COMMISIONING_NOTI_FINDING_BINDING = 0x03,
    ZNP_APP_CFG_BDB_COMMISIONING_NOTI_TOUCHLINK = 0x04,
    ZNP_APP_CFG_BDB_COMMISIONING_NOTI_PARENT_LOST = 0x05,
} e_zb_znp_app_cfg_bdb_commissioning_mode_noti_t;

typedef enum e_zb_znp_app_cfg_remaining_commissioning_mode_noti
{
    ZNP_APP_CFG_BDB_COMMISIONING_NOTI_MODE_INITIATOR_TL = 0x01,
    ZNP_APP_CFG_BDB_COMMISIONING_NOTI_MODE_NWK_STEERING = 0x02,
    ZNP_APP_CFG_BDB_COMMISIONING_NOTI_MODE_NWK_FORMATION = 0x04,
    ZNP_APP_CFG_BDB_COMMISIONING_NOTI_MODE_FINDING_BINDING = 0x08,
    ZNP_APP_CFG_BDB_COMMISIONING_NOTI_MODE_INITIALIZATION = 0x10,
    ZNP_APP_CFG_BDB_COMMISIONING_NOTI_MODE_PARENT_LOST = 0x20,
} e_zb_znp_app_cfg_remaining_commissioning_mode_noti_t;

TYPEDEF_STRUCT_PACKED s_zb_znp_mt_app_cfg_bdb_commissioning_notification_rsp
{
    uint8_t status;
    uint8_t mode;
    uint8_t remaining_mode;
} s_zb_znp_mt_app_cfg_bdb_commissioning_notification_rsp_t;


typedef int (*zb_znp_mt_app_cfg_bdb_commissioning_notification_callback_t)(
    const s_zb_znp_mt_app_cfg_bdb_commissioning_notification_rsp_t *msg);

typedef struct s_zb_znp_mt_app_cfg_cb
{
    zb_znp_mt_app_cfg_bdb_commissioning_notification_callback_t pfn_bdb_commissioning_notification_cb;
} s_zb_znp_mt_app_cfg_cb_t;

void zb_znp_mt_app_cfg_register_callback(s_zb_znp_mt_app_cfg_cb_t callbacks);
void zb_znp_mt_app_cfg_unregister_callback(void);
void zb_znp_mt_app_cfg_process(const uint8_t *rpc_buff, uint8_t rpc_len);
int zb_znp_mt_app_cfg_bdb_start_commissioning(uint8_t mode, uint32_t timeout_ms);
int zb_znp_mt_app_cfg_bdb_set_channel(bool is_primary, uint32_t channel_mask);

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* ZB_ZNP_MT_APP_CFG_H_ */