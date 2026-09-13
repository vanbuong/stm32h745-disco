/*
 * zb_znp_mt_app_cfg.c
 * 
 * Created on: 26 Jun 2025
 *     Author: Vo Van Buong (BRT-SG)
 */

#include "znp/zb_znp.h"
#include "znp/zb_znp_mt_app_cfg.h"

#define TAG "ZB_ZNP_APP_CFG"

/**************************************************************************************************
 * LOCAL VARIABLES
 *************************************************************************************************/
static s_zb_znp_mt_app_cfg_cb_t g_zb_znp_mt_app_cfg_cb = {0};

/**************************************************************************************************
 * LOCAL FUNCTIONS
 *************************************************************************************************/
static void zb_znp_mt_app_cfg_process_bdb_commissioning_notification(const uint8_t *rpc_buff, uint8_t rpc_len);

void
zb_znp_mt_app_cfg_register_callback(s_zb_znp_mt_app_cfg_cb_t callbacks)
{
    memcpy(&g_zb_znp_mt_app_cfg_cb, &callbacks, sizeof(s_zb_znp_mt_app_cfg_cb_t));
}

void
zb_znp_mt_app_cfg_unregister_callback(void)
{
    memset(&g_zb_znp_mt_app_cfg_cb, 0, sizeof(s_zb_znp_mt_app_cfg_cb_t));
}

void
zb_znp_mt_app_cfg_process(const uint8_t *rpc_buff, uint8_t rpc_len)
{
    ZB_LOGI(TAG, "MT_APP_CFG processing CMD: %02X%02X", rpc_buff[0], rpc_buff[1]);

    // Read CMD1 and processes the specific AREQ
    switch (rpc_buff[1])
    {
        case ZNP_APP_CFG_BDB_COMMISIONING_NOTIFICATION:
            zb_znp_mt_app_cfg_process_bdb_commissioning_notification(rpc_buff, rpc_len);
            break;
        default:
            ZB_LOGW(TAG, "Not handled MT_APP_CFG_ARSP CMD: %02X%02X", rpc_buff[0], rpc_buff[1]);
            break;
    }
}

int
zb_znp_mt_app_cfg_bdb_start_commissioning(uint8_t mode, uint32_t timeout_ms)
{
    int status;
    uint8_t rsp_data;
    s_zb_znp_req_t znp_req;

    znp_req.type = ZNP_CMD_REQ_TYPE_ZNP;
    znp_req.start_tick = zb_os_now_ms();
    znp_req.wants_response = true;
    znp_req.timeout_ms = timeout_ms;
    znp_req.znp.cmd0 = (ZNP_MT_CMD_SREQ | ZNP_MT_SYS_APP_CFG);
    znp_req.znp.cmd1 = ZNP_APP_CFG_BDB_START_COMMISSIONING;
    znp_req.znp.payload = (uint8_t *)&mode;
    znp_req.znp.payload_len = 1;
    znp_req.znp.resp_data = &rsp_data;
    znp_req.znp.resp_len = 1;
    status = zb_znp_send_cmd_req(&znp_req);

    if (status == ZB_OK)
    {
        status = rsp_data;
    }

    return status;
}

int
zb_znp_mt_app_cfg_bdb_set_channel(bool is_primary, uint32_t channel_mask)
{
    int status;
    uint8_t data[5] = {0};
    data[0] = is_primary;
    data[1] = (uint8_t)(channel_mask & 0xFF);
    data[2] = (uint8_t)((channel_mask >> 8) & 0xFF);
    data[3] = (uint8_t)((channel_mask >> 16) & 0xFF);
    data[4] = (uint8_t)((channel_mask >> 24) & 0xFF);
    uint8_t rsp_data;
    s_zb_znp_req_t znp_req;

    znp_req.type = ZNP_CMD_REQ_TYPE_ZNP;
    znp_req.start_tick = zb_os_now_ms();
    znp_req.wants_response = true;
    znp_req.timeout_ms = ZNP_MT_SRSP_DEFAULT_TIMEOUT_MS;
    znp_req.znp.cmd0 = (ZNP_MT_CMD_SREQ | ZNP_MT_SYS_APP_CFG);
    znp_req.znp.cmd1 = ZNP_APP_CFG_BDB_SET_CHANNEL;
    znp_req.znp.payload = data;
    znp_req.znp.payload_len = 5;
    znp_req.znp.resp_data = &rsp_data;
    znp_req.znp.resp_len = 1;
    status = zb_znp_send_cmd_req(&znp_req);

    if (status == ZB_OK)
    {
        status = rsp_data;
    }

    return status;
}

static void
zb_znp_mt_app_cfg_process_bdb_commissioning_notification(const uint8_t *rpc_buff, uint8_t rpc_len)
{
    if (g_zb_znp_mt_app_cfg_cb.pfn_bdb_commissioning_notification_cb != NULL)
    {
        const s_zb_znp_mt_app_cfg_bdb_commissioning_notification_rsp_t *rsp =
            (const s_zb_znp_mt_app_cfg_bdb_commissioning_notification_rsp_t *)&rpc_buff[2];
        g_zb_znp_mt_app_cfg_cb.pfn_bdb_commissioning_notification_cb(rsp);
    }
}
