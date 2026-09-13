/*
 * zb_znp_mt_util.c
 * 
 * Author: Vo Van Buong (BRT-SG)
 */

#include "zb_znp.h"
#include "zb_znp_mt_util.h"

#define TAG "ZB_ZNP_UTIL"

void
zb_znp_mt_util_process(const uint8_t *rpc_buff, uint8_t rpc_len)
{
    ZB_LOGI(TAG, "MT_UTIL processing CMD: %02X%02X", rpc_buff[0], rpc_buff[1]);

    // Read CMD1 and processes the specific AREQ
    switch (rpc_buff[1])
    {
        default:
            ZB_LOGW(TAG, "Not handled MT_UTIL_ARSP CMD: %02X%02X", rpc_buff[0], rpc_buff[1]);
            break;
    }
}

int
zb_znp_mt_util_get_device_info(s_zb_znp_mt_util_get_device_info_rsp_t *rsp)
{
    int status;
    s_zb_znp_req_t znp_req;
    uint8_t rsp_data[sizeof(s_zb_znp_mt_util_get_device_info_rsp_t) + sizeof(uint16_t *)];
    znp_req.type = ZNP_CMD_REQ_TYPE_ZNP;
    znp_req.start_tick = zb_os_now_ms();
    znp_req.wants_response = true;
    znp_req.timeout_ms = ZNP_MT_SRSP_DEFAULT_TIMEOUT_MS;
    znp_req.znp.cmd0 = (ZNP_MT_CMD_SREQ | ZNP_MT_SYS_UTIL);
    znp_req.znp.cmd1 = ZNP_UTIL_GET_DEVICE_INFO;
    znp_req.znp.payload = NULL;
    znp_req.znp.payload_len = 0;
    znp_req.znp.resp_data = rsp_data;
    znp_req.znp.resp_len = sizeof(s_zb_znp_mt_util_get_device_info_rsp_t) + sizeof(uint16_t *);
    status = zb_znp_send_cmd_req(&znp_req);

    if (status == ZB_OK)
    {
        if (rsp)
        {
            status = rsp_data[0];
            memcpy(rsp, rsp_data + 1, sizeof(s_zb_znp_mt_util_get_device_info_rsp_t) - sizeof(uint16_t *));
            rsp->assoc_devices = (uint16_t *)ZB_MEM_MALLOC(rsp->num_assoc_devices * sizeof(uint16_t));
        }
    }

    return status;
}

int
zb_znp_mt_util_get_nv_info(s_zb_znp_mt_util_get_nv_info_rsp_t *rsp)
{
    int status;
    uint8_t rsp_data[sizeof(s_zb_znp_mt_util_get_nv_info_rsp_t) + 1];
    s_zb_znp_req_t znp_req;

    znp_req.type = ZNP_CMD_REQ_TYPE_ZNP;
    znp_req.start_tick = zb_os_now_ms();
    znp_req.wants_response = true;
    znp_req.timeout_ms = ZNP_MT_SRSP_DEFAULT_TIMEOUT_MS;
    znp_req.znp.cmd0 = (ZNP_MT_CMD_SREQ | ZNP_MT_SYS_UTIL);
    znp_req.znp.cmd1 = ZNP_UTIL_GET_NV_INFO;
    znp_req.znp.payload = NULL;
    znp_req.znp.payload_len = 0;
    znp_req.znp.resp_data = rsp_data;
    znp_req.znp.resp_len = sizeof(s_zb_znp_mt_util_get_nv_info_rsp_t) + 1;
    status = zb_znp_send_cmd_req(&znp_req);

    if (status == ZB_OK)
    {
        if (rsp)
        {
            status = rsp_data[0];
            memcpy(rsp, rsp_data + 1, sizeof(s_zb_znp_mt_util_get_nv_info_rsp_t));
        }
    }

    return status;
}