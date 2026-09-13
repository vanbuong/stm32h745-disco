/*
 * zb_znp_mt_af.c
 *
 * This file contains the implementation of the MT AF Interface.
 *
 * Author: Vo Van Buong (BRT-SG)
 */

#include "znp/zb_znp.h"
#include "znp/zb_znp_mt_af.h"

#define TAG "ZB_ZNP_AF"

/**************************************************************************************************
 * LOCAL VARIABLES
 *************************************************************************************************/
static s_zb_znp_mt_af_cb_t g_zb_znp_mt_af_cb = {0};

/**************************************************************************************************
 * LOCAL FUNCTIONS
 *************************************************************************************************/
static void zb_znp_mt_af_process_data_confirm(const uint8_t *rpc_buff, uint8_t rpc_len);
static void zb_znp_mt_af_process_incoming_msg(const uint8_t *rpc_buff, uint8_t rpc_len);

void
zb_znp_mt_af_register_callback(s_zb_znp_mt_af_cb_t callbacks)
{
    memcpy(&g_zb_znp_mt_af_cb, &callbacks, sizeof(s_zb_znp_mt_af_cb_t));
}

void
zb_znp_mt_af_unregister_callback(void)
{
    memset(&g_zb_znp_mt_af_cb, 0, sizeof(s_zb_znp_mt_af_cb_t));
}

void
zb_znp_mt_af_process(const uint8_t *rpc_buff, uint8_t rpc_len)
{
    ZB_LOGI(TAG, "MT_AF processing CMD: %02X%02X", rpc_buff[0], rpc_buff[1]);

    // Read CMD1 and processes the specific AREQ
    switch (rpc_buff[1])
    {
        case ZNP_AF_DATA_CONFIRM:
            zb_znp_mt_af_process_data_confirm(rpc_buff, rpc_len);
            break;
        case ZNP_AF_INCOMING_MSG:
            zb_znp_mt_af_process_incoming_msg(rpc_buff, rpc_len);
            break;
        default:
            ZB_LOGW(TAG, "Not handled MT_AF_ARSP CMD: %02X%02X", rpc_buff[0], rpc_buff[1]);
            break;
    }
}

int
zb_znp_mt_af_register(const s_zb_znp_mt_af_register_cmd_t *req_cmd)
{
    int status;
    uint8_t cmd_idx = 0;
    uint8_t cmd_len = 9 + (req_cmd->num_in_clusters * 2)
                      + (req_cmd->num_out_clusters * 2);
    uint8_t *cmd = ZB_MEM_MALLOC(cmd_len);
    uint8_t rsp_data;
    s_zb_znp_req_t znp_req;

    if (cmd)
    {
        cmd[cmd_idx++] = req_cmd->endpoint;
        cmd[cmd_idx++] = (uint8_t)(req_cmd->profile_id & 0xFF);
        cmd[cmd_idx++] = (uint8_t)((req_cmd->profile_id >> 8) & 0xFF);
        cmd[cmd_idx++] = (uint8_t)(req_cmd->device_id & 0xFF);
        cmd[cmd_idx++] = (uint8_t)((req_cmd->device_id >> 8) & 0xFF);
        cmd[cmd_idx++] = req_cmd->device_version;
        cmd[cmd_idx++] = req_cmd->latency;
        cmd[cmd_idx++] = req_cmd->num_in_clusters;
        for (uint8_t idx = 0; idx < req_cmd->num_in_clusters; idx++)
        {
            cmd[cmd_idx++] = (uint8_t)(req_cmd->in_cluster_list[idx] & 0xFF);
            cmd[cmd_idx++] = (uint8_t)((req_cmd->in_cluster_list[idx] >> 8) & 0xFF);
        }
        cmd[cmd_idx++] = req_cmd->num_out_clusters;
        for (uint8_t idx = 0; idx < req_cmd->num_out_clusters; idx++)
        {
            cmd[cmd_idx++] = (uint8_t)(req_cmd->out_cluster_list[idx] & 0xFF);
            cmd[cmd_idx++] = (uint8_t)((req_cmd->out_cluster_list[idx] >> 8) & 0xFF);
        }
        znp_req.type = ZNP_CMD_REQ_TYPE_ZNP;
        znp_req.start_tick = zb_os_now_ms();
        znp_req.wants_response = true;
        znp_req.timeout_ms = ZNP_MT_SRSP_DEFAULT_TIMEOUT_MS;
        znp_req.znp.cmd0 = (ZNP_MT_CMD_SREQ | ZNP_MT_SYS_AF);
        znp_req.znp.cmd1 = ZNP_AF_REGISTER;
        znp_req.znp.payload = cmd;
        znp_req.znp.payload_len = cmd_len;
        znp_req.znp.resp_data = &rsp_data;
        znp_req.znp.resp_len = 1;
        status = zb_znp_send_cmd_req(&znp_req);

        if (status == ZB_OK)
        {
            status = rsp_data;
        }

        ZB_MEM_FREE(cmd);
        return status;
    }
    else
    {
        ZB_LOGE(TAG, "%s(): Memory for cmd was not allocated\n", __func__);
        return ZB_MEM_ERROR;
    }
}

int
zb_znp_mt_af_data_request(const s_zb_znp_mt_af_data_request_cmd_t *req_cmd)
{
    int status;
    uint8_t cmd_len = 10 + req_cmd->len;
    uint8_t rsp_data;
    s_zb_znp_req_t znp_req;

    znp_req.type = ZNP_CMD_REQ_TYPE_ZNP;
    znp_req.start_tick = zb_os_now_ms();
    znp_req.wants_response = true;
    znp_req.timeout_ms = ZNP_MT_SRSP_DEFAULT_TIMEOUT_MS;
    znp_req.znp.cmd0 = (ZNP_MT_CMD_SREQ | ZNP_MT_SYS_AF);
    znp_req.znp.cmd1 = ZNP_AF_DATA_REQUEST;
    znp_req.znp.payload = (uint8_t *)req_cmd;
    znp_req.znp.payload_len = cmd_len;
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
zb_znp_mt_af_data_request_ext(const s_zb_znp_mt_af_data_request_ext_cmd_t *req_cmd)
{
    int status;
    uint8_t cmd_len = 20 + req_cmd->len;
    uint8_t rsp_data;
    s_zb_znp_req_t znp_req;

    znp_req.type = ZNP_CMD_REQ_TYPE_ZNP;
    znp_req.start_tick = zb_os_now_ms();
    znp_req.wants_response = true;
    znp_req.timeout_ms = ZNP_MT_SRSP_DEFAULT_TIMEOUT_MS;
    znp_req.znp.cmd0 = (ZNP_MT_CMD_SREQ | ZNP_MT_SYS_AF);
    znp_req.znp.cmd1 = ZNP_AF_DATA_REQUEST_EXT;
    znp_req.znp.payload = (uint8_t *)req_cmd;
    znp_req.znp.payload_len = cmd_len;
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
zb_znp_mt_af_data_store(const s_zb_znp_mt_af_data_store_cmd_t *req_cmd)
{
    int status = ZB_OK;
    uint8_t cmd_len = 3 + req_cmd->length;
    uint8_t *cmd = ZB_MEM_MALLOC(cmd_len);
    uint8_t rsp_data;
    s_zb_znp_req_t znp_req;

    if (cmd)
    {
        cmd[0] = LO_UINT16(req_cmd->index);
        cmd[1] = HI_UINT16(req_cmd->index);
        cmd[2] = req_cmd->length;
        if (req_cmd->length > 0)
        {
            memcpy(&cmd[3], req_cmd->data, req_cmd->length);
        }

        znp_req.type = ZNP_CMD_REQ_TYPE_ZNP;
        znp_req.start_tick = zb_os_now_ms();
        znp_req.wants_response = true;
        znp_req.timeout_ms = ZNP_MT_SRSP_DEFAULT_TIMEOUT_MS;
        znp_req.znp.cmd0 = (ZNP_MT_CMD_SREQ | ZNP_MT_SYS_AF);
        znp_req.znp.cmd1 = ZNP_AF_DATA_STORE;
        znp_req.znp.payload = cmd;
        znp_req.znp.payload_len = cmd_len;
        znp_req.znp.resp_data = &rsp_data;
        znp_req.znp.resp_len = 1;
        status = zb_znp_send_cmd_req(&znp_req);

        if (status == ZB_OK)
        {
            status = rsp_data;
        }

        ZB_MEM_FREE(cmd);
        return status;
    }
    else
    {
        ZB_LOGE(TAG, "%s(): Memory for cmd was not allocated\n", __func__);
        return ZB_MEM_ERROR;
    }
}

static void
zb_znp_mt_af_process_data_confirm(const uint8_t *rpc_buff, uint8_t rpc_len)
{
    if (g_zb_znp_mt_af_cb.pfn_af_data_confirm_cb)
    {
        const s_zb_znp_mt_af_data_confirm_msg_t *msg = (const s_zb_znp_mt_af_data_confirm_msg_t *)&rpc_buff[2];
        if (rpc_len < sizeof(s_zb_znp_mt_af_data_confirm_msg_t) + 2)
        {
            ZB_LOGE(TAG, "MT_AF_DATA_CONFIRM: MT_RPC_ERR_LENGTH rpc_len:%d\n", rpc_len);
            return;
        }

        g_zb_znp_mt_af_cb.pfn_af_data_confirm_cb(msg);
    }
}

static void
zb_znp_mt_af_process_incoming_msg(const uint8_t *rpc_buff, uint8_t rpc_len)
{
    if (g_zb_znp_mt_af_cb.pfn_af_incoming_msg_cb)
    {
        const s_zb_znp_mt_af_incoming_msg_t *msg = (const s_zb_znp_mt_af_incoming_msg_t *)&rpc_buff[2];
        if (rpc_len < msg->len + 20)
        {
            ZB_LOGE(TAG, "MT_AF_INCOMING_MSG: MT_RPC_ERR_LENGTH rpc_len:%d\n", rpc_len);
            return;
        }

        g_zb_znp_mt_af_cb.pfn_af_incoming_msg_cb(msg);
    }
}
