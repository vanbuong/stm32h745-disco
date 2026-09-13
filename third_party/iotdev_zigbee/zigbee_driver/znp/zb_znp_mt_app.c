/*
 * zb_znp_mt_app.c
 * 
 * Created on: 26 Jun 2025
 *     Author: Vo Van Buong (BRT-SG)
 */

#include "znp/zb_znp.h"
#include "znp/zb_znp_mt_app.h"

#define TAG "ZB_ZNP_APP"

/**************************************************************************************************
 * LOCAL VARIABLES
 *************************************************************************************************/
static s_zb_znp_mt_app_cb_t g_zb_znp_mt_app_cb = {0};

/**************************************************************************************************
 * LOCAL FUNCTIONS
 *************************************************************************************************/
static void zb_znp_mt_app_process_rs485_data_ind(const uint8_t *rpc_buff, uint8_t rpc_len);
static void zb_znp_mt_app_process_rs485_error_ind(const uint8_t *rpc_buff, uint8_t rpc_len);

void
zb_znp_mt_app_register_callback(s_zb_znp_mt_app_cb_t callbacks)
{
    memcpy(&g_zb_znp_mt_app_cb, &callbacks, sizeof(s_zb_znp_mt_app_cb_t));
}

void
zb_znp_mt_app_unregister_callback(void)
{
    memset(&g_zb_znp_mt_app_cb, 0, sizeof(s_zb_znp_mt_app_cb_t));
}

void
zb_znp_mt_app_process(const uint8_t *rpc_buff, uint8_t rpc_len)
{
    ZB_LOGI(TAG, "MT_APP processing CMD: %02X%02X", rpc_buff[0], rpc_buff[1]);

    // Read CMD1 and processes the specific AREQ
    switch (rpc_buff[1])
    {
        case ZNP_APP_RS485_DATA_IND:
            zb_znp_mt_app_process_rs485_data_ind(rpc_buff, rpc_len);
            break;
        case ZNP_APP_RS485_ERROR_IND:
            zb_znp_mt_app_process_rs485_error_ind(rpc_buff, rpc_len);
            break;
        default:
            ZB_LOGW(TAG, "Not handled MT_APP_AREQ CMD: %02X%02X", rpc_buff[0], rpc_buff[1]);
            break;
    }
}

int
zb_znp_mt_app_rs485_write_req(const uint8_t *data, uint8_t len)
{
    int status;
    uint8_t rsp_data;
    s_zb_znp_req_t znp_req;

    znp_req.type = ZNP_CMD_REQ_TYPE_ZNP;
    znp_req.start_tick = zb_os_now_ms();
    znp_req.wants_response = true;
    znp_req.timeout_ms = ZNP_MT_SRSP_DEFAULT_TIMEOUT_MS;
    znp_req.znp.cmd0 = (ZNP_MT_CMD_SREQ | ZNP_MT_SYS_APP);
    znp_req.znp.cmd1 = ZNP_APP_RS485_WRITE_REQ;
    znp_req.znp.payload = (uint8_t *)data;
    znp_req.znp.payload_len = len;
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
zb_znp_mt_app_rs485_config_req(const s_zb_znp_mt_app_config_t *config)
{
    int status;
    uint8_t data[8] = {0};
    data[0] = config->baud_rate & 0xFF;
    data[1] = (config->baud_rate >> 8) & 0xFF;
    data[2] = (config->baud_rate >> 16) & 0xFF;
    data[3] = (config->baud_rate >> 24) & 0xFF;
    data[4] = config->stop_bits;
    data[5] = config->parity;
    data[6] = config->idle_gap_ms & 0xFF;
    data[7] = (config->idle_gap_ms >> 8) & 0xFF;
    uint8_t rsp_data[9];
    s_zb_znp_req_t znp_req;

    znp_req.type = ZNP_CMD_REQ_TYPE_ZNP;
    znp_req.start_tick = zb_os_now_ms();
    znp_req.wants_response = true;
    znp_req.timeout_ms = ZNP_MT_SRSP_DEFAULT_TIMEOUT_MS;
    znp_req.znp.cmd0 = (ZNP_MT_CMD_SREQ | ZNP_MT_SYS_APP);
    znp_req.znp.cmd1 = ZNP_APP_RS485_CONFIG_REQ;
    znp_req.znp.payload = data;
    znp_req.znp.payload_len = 8;
    znp_req.znp.resp_data = &rsp_data[0];
    znp_req.znp.resp_len = 9;
    status = zb_znp_send_cmd_req(&znp_req);

    if (status == ZB_OK)
    {
        status = rsp_data[0];
    }

    return status;
}

static void
zb_znp_mt_app_process_rs485_data_ind(const uint8_t *rpc_buff, uint8_t rpc_len)
{
    if (g_zb_znp_mt_app_cb.pfn_rs485_data_ind_cb != NULL)
    {
        const s_zb_znp_mt_app_rs485_data_ind_t data = {.len = rpc_len - 2, .data = (uint8_t *)&rpc_buff[2]};
        g_zb_znp_mt_app_cb.pfn_rs485_data_ind_cb(&data);
    }
}

static void
zb_znp_mt_app_process_rs485_error_ind(const uint8_t *rpc_buff, uint8_t rpc_len)
{
    if (g_zb_znp_mt_app_cb.pfn_rs485_error_ind_cb != NULL)
    {
        const uint8_t *error = &rpc_buff[2];
        g_zb_znp_mt_app_cb.pfn_rs485_error_ind_cb(error);
    }
}