/*
 * zb_znp_mt_sys.c
 *
 * This file contains the implementation of the MT SYS Interface.
 *
 * Author: Vo Van Buong (BRT-SG)
 */

#include "znp/zb_znp.h"
#include "znp/zb_znp_mt_sys.h"

#define TAG "ZB_ZNP_SYS"

/**************************************************************************************************
 * LOCAL VARIABLES
 *************************************************************************************************/
static s_zb_znp_mt_sys_cb_t g_zb_znp_mt_sys_cb = {0};

/**************************************************************************************************
 * LOCAL FUNCTIONS
 *************************************************************************************************/
static void zb_znp_mt_sys_process_reset_ind(const uint8_t *rpc_buff, uint8_t rpc_len);
static void zb_znp_mt_sys_process_osal_timer_expired_ind(const uint8_t *rpc_buff, uint8_t rpc_len);

/**************************************************************************************************
 * FUNCTIONS
 *************************************************************************************************/
void
zb_znp_mt_sys_register_callback(s_zb_znp_mt_sys_cb_t callback)
{
    memcpy(&g_zb_znp_mt_sys_cb, &callback, sizeof(s_zb_znp_mt_sys_cb_t));
}

void
zb_znp_mt_sys_unregister_callback(void)
{
    memset(&g_zb_znp_mt_sys_cb, 0, sizeof(s_zb_znp_mt_sys_cb_t));
}

void
zb_znp_mt_sys_process(const uint8_t *rpc_buff, uint8_t rpc_len)
{
    ZB_LOGI(TAG, "MT_SYS processing CMD: %02X%02X", rpc_buff[0], rpc_buff[1]);

    // Read CMD1 and processes the specific AREQ
    switch (rpc_buff[1])
    {
        case ZNP_SYS_RESET_IND:
            zb_znp_mt_sys_process_reset_ind(rpc_buff, rpc_len);
            break;
        case ZNP_SYS_OSAL_TIMER_EXPIRED:
            zb_znp_mt_sys_process_osal_timer_expired_ind(rpc_buff, rpc_len);
            break;
        default:
            ZB_LOGW(TAG, "Not handled MT_SYS_ARSP CMD: %02X%02X", rpc_buff[0], rpc_buff[1]);
            break;
    }
}

int
zb_znp_mt_sys_reset_req(const s_zb_znp_mt_sys_reset_req_t *req_cmd)
{
    int status;
    s_zb_znp_req_t znp_req;
    znp_req.type = ZNP_CMD_REQ_TYPE_ZNP;
    znp_req.start_tick = zb_os_now_ms();
    znp_req.wants_response = true;
    znp_req.timeout_ms = ZNP_MT_SYS_RESET_REQ_TIMEOUT_MS;
    znp_req.znp.cmd0 = (ZNP_MT_CMD_AREQ | ZNP_MT_SYS_SYS);
    znp_req.znp.cmd1 = ZNP_SYS_RESET_REQ;
    znp_req.znp.payload = (uint8_t *)req_cmd;
    znp_req.znp.payload_len = sizeof(s_zb_znp_mt_sys_reset_req_t);
    znp_req.znp.resp_data = NULL;
    znp_req.znp.resp_len = 0;
    status = zb_znp_send_cmd_req(&znp_req);

    return status;
}

static void
zb_znp_mt_sys_process_reset_ind(const uint8_t *rpc_buff, uint8_t rpc_len)
{
    if (g_zb_znp_mt_sys_cb.pfn_sys_reset_ind_cb)
    {
        const s_zb_znp_mt_sys_reset_ind_t *rsp = (const s_zb_znp_mt_sys_reset_ind_t *)&rpc_buff[2];
        if (rpc_len < (sizeof(s_zb_znp_mt_sys_reset_ind_t) + 2))
        {
            ZB_LOGE(TAG, "MT_SYS_RESET_IND: MT_RPC_ERR_LENGTH rpc_len:%d\n", rpc_len);
            return;
        }

        g_zb_znp_mt_sys_cb.pfn_sys_reset_ind_cb(rsp);
    }
}

int
zb_znp_mt_sys_ping(s_zb_znp_mt_sys_ping_srsp_t *rsp)
{
    int status;
    uint8_t rsp_data[2];
    s_zb_znp_req_t znp_req;

    znp_req.type = ZNP_CMD_REQ_TYPE_ZNP;
    znp_req.start_tick = zb_os_now_ms();
    znp_req.wants_response = true;
    znp_req.timeout_ms = ZNP_MT_SRSP_DEFAULT_TIMEOUT_MS;
    znp_req.znp.cmd0 = (ZNP_MT_CMD_SREQ | ZNP_MT_SYS_SYS);
    znp_req.znp.cmd1 = ZNP_SYS_PING;
    znp_req.znp.payload = NULL;
    znp_req.znp.payload_len = 0;
    znp_req.znp.resp_data = rsp_data;
    znp_req.znp.resp_len = 2;
    status = zb_znp_send_cmd_req(&znp_req);

    if (status == ZB_OK)
    {
        if (rsp)
        {
            rsp->capabilities = BUILD_UINT16(rsp_data[0], rsp_data[1]);
        }
    }

    return status;
}

int
zb_znp_mt_sys_version(s_zb_znp_mt_sys_version_srsp_t *rsp)
{
    int status;
    uint8_t rsp_data[8];
    s_zb_znp_req_t znp_req;

    znp_req.type = ZNP_CMD_REQ_TYPE_ZNP;
    znp_req.start_tick = zb_os_now_ms();
    znp_req.wants_response = true;
    znp_req.timeout_ms = ZNP_MT_SRSP_DEFAULT_TIMEOUT_MS;
    znp_req.znp.cmd0 = (ZNP_MT_CMD_SREQ | ZNP_MT_SYS_SYS);
    znp_req.znp.cmd1 = ZNP_SYS_VERSION;
    znp_req.znp.payload = NULL;
    znp_req.znp.payload_len = 0;
    znp_req.znp.resp_data = rsp_data;
    znp_req.znp.resp_len = 8;
    status = zb_znp_send_cmd_req(&znp_req);

    if (status == ZB_OK)
    {
        if (rsp)
        {
            rsp->transport_revision = rsp_data[0];
            rsp->product_id = rsp_data[1];
            rsp->major_rel = rsp_data[2];
            rsp->minor_rel = rsp_data[3];
            rsp->maint_rel = rsp_data[4];
            rsp->fw_major_rel = rsp_data[5];
            rsp->fw_minor_rel = rsp_data[6];
            rsp->fw_maint_rel = rsp_data[7];
        }
    }

    return status;
}

int
zb_znp_mt_sys_set_extaddr(const s_zb_znp_mt_sys_set_extaddr_cmd_t *req_cmd)
{
    int status;
    uint8_t rsp_data;
    s_zb_znp_req_t znp_req;

    znp_req.type = ZNP_CMD_REQ_TYPE_ZNP;
    znp_req.start_tick = zb_os_now_ms();
    znp_req.wants_response = true;
    znp_req.timeout_ms = ZNP_MT_SRSP_DEFAULT_TIMEOUT_MS;
    znp_req.znp.cmd0 = (ZNP_MT_CMD_SREQ | ZNP_MT_SYS_SYS);
    znp_req.znp.cmd1 = ZNP_SYS_SET_EXTADDR;
    znp_req.znp.payload = (uint8_t *)req_cmd;
    znp_req.znp.payload_len = sizeof(s_zb_znp_mt_sys_set_extaddr_cmd_t);
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
zb_znp_mt_sys_get_extaddr(s_zb_znp_mt_sys_get_extaddr_srsp_t *rsp)
{
    int status;
    uint8_t rsp_data[8];
    s_zb_znp_req_t znp_req;

    znp_req.type = ZNP_CMD_REQ_TYPE_ZNP;
    znp_req.start_tick = zb_os_now_ms();
    znp_req.wants_response = true;
    znp_req.timeout_ms = ZNP_MT_SRSP_DEFAULT_TIMEOUT_MS;
    znp_req.znp.cmd0 = (ZNP_MT_CMD_SREQ | ZNP_MT_SYS_SYS);
    znp_req.znp.cmd1 = ZNP_SYS_GET_EXTADDR;
    znp_req.znp.payload = NULL;
    znp_req.znp.payload_len = 0;
    znp_req.znp.resp_data = rsp_data;
    znp_req.znp.resp_len = 8;
    status = zb_znp_send_cmd_req(&znp_req);

    if (status == ZB_OK)
    {
        if (rsp)
        {
            rsp->ext_addr = *(uint64_t *)&rsp_data[0];
        }
    }

    return status;
}

int
zb_znp_mt_sys_ram_read(
    const s_zb_znp_mt_sys_ram_read_cmd_t *req_cmd,
    s_zb_znp_mt_sys_ram_read_srsp_t *rsp)
{
    int status;
    uint8_t *rsp_data = ZB_MEM_MALLOC(2 + req_cmd->length);
    s_zb_znp_req_t znp_req;

    if (!rsp_data)
    {
        ZB_LOGE(TAG, "MT_SYS_RAM_READ: ZB_MEM_MALLOC failed\n");
        return ZB_MEM_ERROR;
    }

    znp_req.type = ZNP_CMD_REQ_TYPE_ZNP;
    znp_req.start_tick = zb_os_now_ms();
    znp_req.wants_response = true;
    znp_req.timeout_ms = ZNP_MT_SRSP_DEFAULT_TIMEOUT_MS;
    znp_req.znp.cmd0 = (ZNP_MT_CMD_SREQ | ZNP_MT_SYS_SYS);
    znp_req.znp.cmd1 = ZNP_SYS_RAM_READ;
    znp_req.znp.payload = (uint8_t *)req_cmd;
    znp_req.znp.payload_len = sizeof(s_zb_znp_mt_sys_ram_read_cmd_t);
    znp_req.znp.resp_data = rsp_data;
    znp_req.znp.resp_len = 2 + req_cmd->length;
    status = zb_znp_send_cmd_req(&znp_req);

    if (status == ZB_OK)
    {
        if (rsp)
        {
            status = rsp_data[0];
            rsp->len = rsp_data[1];
            memcpy(rsp->value, rsp_data + 2, rsp->len);
        }
    }
    ZB_MEM_FREE(rsp_data);

    return status;
}

int
zb_znp_mt_sys_ram_write(const s_zb_znp_mt_sys_ram_write_cmd_t *req_cmd)
{
    int status;
    uint8_t rsp_data;
    uint8_t cmd_len = 3 + req_cmd->len;
    s_zb_znp_req_t znp_req;

    znp_req.type = ZNP_CMD_REQ_TYPE_ZNP;
    znp_req.start_tick = zb_os_now_ms();
    znp_req.wants_response = true;
    znp_req.timeout_ms = ZNP_MT_SRSP_DEFAULT_TIMEOUT_MS;
    znp_req.znp.cmd0 = (ZNP_MT_CMD_SREQ | ZNP_MT_SYS_SYS);
    znp_req.znp.cmd1 = ZNP_SYS_RAM_WRITE;
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
zb_znp_mt_sys_osal_nv_item_init(const s_zb_znp_mt_sys_osal_nv_item_init_cmd_t *req_cmd)
{
    int status;
    uint8_t rsp_data;
    uint8_t cmd_len = 5 + req_cmd->init_len;
    s_zb_znp_req_t znp_req;

    znp_req.type = ZNP_CMD_REQ_TYPE_ZNP;
    znp_req.start_tick = zb_os_now_ms();
    znp_req.wants_response = true;
    znp_req.timeout_ms = ZNP_MT_SRSP_DEFAULT_TIMEOUT_MS;
    znp_req.znp.cmd0 = (ZNP_MT_CMD_SREQ | ZNP_MT_SYS_SYS);
    znp_req.znp.cmd1 = ZNP_SYS_OSAL_NV_ITEM_INIT;
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
zb_znp_mt_sys_osal_nv_read(
    const s_zb_znp_mt_sys_osal_nv_read_cmd_t *req_cmd,
    s_zb_znp_mt_sys_osal_nv_read_srsp_t *rsp)
{
    int status;
    uint8_t *rsp_data = ZB_MEM_MALLOC(2 + 128);
    s_zb_znp_req_t znp_req;

    if (!rsp_data)
    {
        ZB_LOGE(TAG, "MT_SYS_OSAL_NV_READ: ZB_MEM_MALLOC failed\n");
        return ZB_MEM_ERROR;
    }

    znp_req.type = ZNP_CMD_REQ_TYPE_ZNP;
    znp_req.start_tick = zb_os_now_ms();
    znp_req.wants_response = true;
    znp_req.timeout_ms = ZNP_MT_SRSP_DEFAULT_TIMEOUT_MS;
    znp_req.znp.cmd0 = (ZNP_MT_CMD_SREQ | ZNP_MT_SYS_SYS);
    znp_req.znp.cmd1 = ZNP_SYS_OSAL_NV_READ;
    znp_req.znp.payload = (uint8_t *)req_cmd;
    znp_req.znp.payload_len = sizeof(s_zb_znp_mt_sys_osal_nv_read_cmd_t);
    znp_req.znp.resp_data = rsp_data;
    znp_req.znp.resp_len = 2 + 128;
    status = zb_znp_send_cmd_req(&znp_req);

    if (status == ZB_OK)
    {
        if (rsp)
        {
            status = rsp_data[0];
            rsp->len = rsp_data[1];
            memcpy(rsp->value, rsp_data + 2, rsp->len);
        }
    }
    ZB_MEM_FREE(rsp_data);
    return status;
}

int
zb_znp_mt_sys_osal_nv_write(const s_zb_znp_mt_sys_osal_nv_write_cmd_t *req_cmd)
{
    int status;
    uint8_t cmd_len = 4 + req_cmd->len;
    uint8_t rsp_data;
    s_zb_znp_req_t znp_req;

    znp_req.type = ZNP_CMD_REQ_TYPE_ZNP;
    znp_req.start_tick = zb_os_now_ms();
    znp_req.wants_response = true;
    znp_req.timeout_ms = ZNP_MT_SRSP_DEFAULT_TIMEOUT_MS;
    znp_req.znp.cmd0 = (ZNP_MT_CMD_SREQ | ZNP_MT_SYS_SYS);
    znp_req.znp.cmd1 = ZNP_SYS_OSAL_NV_WRITE;
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
zb_znp_mt_sys_osal_nv_delete(const s_zb_znp_mt_sys_osal_nv_delete_cmd_t *req_cmd)
{
    int status;
    uint8_t rsp_data;
    s_zb_znp_req_t znp_req;

    znp_req.type = ZNP_CMD_REQ_TYPE_ZNP;
    znp_req.start_tick = zb_os_now_ms();
    znp_req.wants_response = true;
    znp_req.timeout_ms = ZNP_MT_SRSP_DEFAULT_TIMEOUT_MS;
    znp_req.znp.cmd0 = (ZNP_MT_CMD_SREQ | ZNP_MT_SYS_SYS);
    znp_req.znp.cmd1 = ZNP_SYS_OSAL_NV_DELETE;
    znp_req.znp.payload = (uint8_t *)req_cmd;
    znp_req.znp.payload_len = sizeof(s_zb_znp_mt_sys_osal_nv_delete_cmd_t);
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
zb_znp_mt_sys_osal_nv_length(
    const s_zb_znp_mt_sys_osal_nv_length_cmd_t *req_cmd,
    s_zb_znp_mt_sys_osal_nv_length_srsp_t *rsp)
{
    int status;
    uint8_t rsp_data[2];
    s_zb_znp_req_t znp_req;

    znp_req.type = ZNP_CMD_REQ_TYPE_ZNP;
    znp_req.start_tick = zb_os_now_ms();
    znp_req.wants_response = true;
    znp_req.timeout_ms = ZNP_MT_SRSP_DEFAULT_TIMEOUT_MS;
    znp_req.znp.cmd0 = (ZNP_MT_CMD_SREQ | ZNP_MT_SYS_SYS);
    znp_req.znp.cmd1 = ZNP_SYS_OSAL_NV_LENGTH;
    znp_req.znp.payload = (uint8_t *)req_cmd;
    znp_req.znp.payload_len = sizeof(s_zb_znp_mt_sys_osal_nv_length_cmd_t);
    znp_req.znp.resp_data = rsp_data;
    znp_req.znp.resp_len = 2;
    status = zb_znp_send_cmd_req(&znp_req);

    if (status == ZB_OK)
    {
        if (rsp)
        {
            rsp->item_len = BUILD_UINT16(rsp_data[0], rsp_data[1]);
        }
    }

    return status;
}

int
zb_znp_mt_sys_osal_start_timer(const s_zb_znp_mt_sys_osal_start_timer_cmd_t *req_cmd)
{
    int status;
    uint8_t rsp_data;
    s_zb_znp_req_t znp_req;

    znp_req.type = ZNP_CMD_REQ_TYPE_ZNP;
    znp_req.start_tick = zb_os_now_ms();
    znp_req.wants_response = true;
    znp_req.timeout_ms = ZNP_MT_SRSP_DEFAULT_TIMEOUT_MS;
    znp_req.znp.cmd0 = (ZNP_MT_CMD_SREQ | ZNP_MT_SYS_SYS);
    znp_req.znp.cmd1 = ZNP_SYS_OSAL_START_TIMER;
    znp_req.znp.payload = (uint8_t *)req_cmd;
    znp_req.znp.payload_len = sizeof(s_zb_znp_mt_sys_osal_start_timer_cmd_t);
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
zb_znp_mt_sys_osal_stop_timer(const s_zb_znp_mt_sys_osal_stop_timer_cmd_t *req_cmd)
{
    int status;
    uint8_t rsp_data;
    s_zb_znp_req_t znp_req;

    znp_req.type = ZNP_CMD_REQ_TYPE_ZNP;
    znp_req.start_tick = zb_os_now_ms();
    znp_req.wants_response = true;
    znp_req.timeout_ms = ZNP_MT_SRSP_DEFAULT_TIMEOUT_MS;
    znp_req.znp.cmd0 = (ZNP_MT_CMD_SREQ | ZNP_MT_SYS_SYS);
    znp_req.znp.cmd1 = ZNP_SYS_OSAL_STOP_TIMER;
    znp_req.znp.payload = (uint8_t *)req_cmd;
    znp_req.znp.payload_len = sizeof(s_zb_znp_mt_sys_osal_stop_timer_cmd_t);
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
zb_znp_mt_sys_process_osal_timer_expired_ind(const uint8_t *rpc_buff, uint8_t rpc_len)
{
    if (g_zb_znp_mt_sys_cb.pfn_sys_osal_timer_expired_ind_cb)
    {
        const s_zb_znp_mt_sys_osal_timer_expired_ind_t *rsp =
            (const s_zb_znp_mt_sys_osal_timer_expired_ind_t *)&rpc_buff[2];
        if (rpc_len < sizeof(s_zb_znp_mt_sys_osal_timer_expired_ind_t) + 2)
        {
            ZB_LOGE(TAG, "MT_SYS_OSAL_TIMER_EXPIRED_IND: MT_RPC_ERR_LENGTH rpc_len:%d\n", rpc_len);
            return;
        }

        g_zb_znp_mt_sys_cb.pfn_sys_osal_timer_expired_ind_cb(rsp);
    }
}

int
zb_znp_mt_sys_random(s_zb_znp_mt_sys_random_srsp_t *rsp)
{
    int status;
    uint8_t rsp_data[2];
    s_zb_znp_req_t znp_req;

    znp_req.type = ZNP_CMD_REQ_TYPE_ZNP;
    znp_req.start_tick = zb_os_now_ms();
    znp_req.wants_response = true;
    znp_req.timeout_ms = ZNP_MT_SRSP_DEFAULT_TIMEOUT_MS;
    znp_req.znp.cmd0 = (ZNP_MT_CMD_SREQ | ZNP_MT_SYS_SYS);
    znp_req.znp.cmd1 = ZNP_SYS_RANDOM;
    znp_req.znp.payload = NULL;
    znp_req.znp.payload_len = 0;
    znp_req.znp.resp_data = rsp_data;
    znp_req.znp.resp_len = 2;
    status = zb_znp_send_cmd_req(&znp_req);

    if (status == ZB_OK)
    {
        if (rsp)
        {
            rsp->value = BUILD_UINT16(rsp_data[0], rsp_data[1]);
        }
    }

    return status;
}

int
zb_znp_mt_sys_adc_read(
    const s_zb_znp_mt_sys_adc_read_cmd_t *req_cmd,
    s_zb_znp_mt_sys_adc_read_srsp_t *rsp)
{
    int status;
    uint8_t rsp_data[2];
    s_zb_znp_req_t znp_req;

    znp_req.type = ZNP_CMD_REQ_TYPE_ZNP;
    znp_req.start_tick = zb_os_now_ms();
    znp_req.wants_response = true;
    znp_req.timeout_ms = ZNP_MT_SRSP_DEFAULT_TIMEOUT_MS;
    znp_req.znp.cmd0 = (ZNP_MT_CMD_SREQ | ZNP_MT_SYS_SYS);
    znp_req.znp.cmd1 = ZNP_SYS_ADC_READ;
    znp_req.znp.payload = (uint8_t *)req_cmd;
    znp_req.znp.payload_len = sizeof(s_zb_znp_mt_sys_adc_read_cmd_t);
    znp_req.znp.resp_data = rsp_data;
    znp_req.znp.resp_len = 2;
    status = zb_znp_send_cmd_req(&znp_req);

    if (status == ZB_OK)
    {
        if (rsp)
        {
            rsp->value = BUILD_UINT16(rsp_data[0], rsp_data[1]);
        }
    }

    return status;
}

int
zb_znp_mt_sys_gpio(const s_zb_znp_mt_sys_gpio_cmd_t *req_cmd, s_zb_znp_mt_sys_gpio_srsp_t *rsp)
{
    int status;
    uint8_t rsp_data;
    s_zb_znp_req_t znp_req;

    znp_req.type = ZNP_CMD_REQ_TYPE_ZNP;
    znp_req.start_tick = zb_os_now_ms();
    znp_req.wants_response = true;
    znp_req.timeout_ms = ZNP_MT_SRSP_DEFAULT_TIMEOUT_MS;
    znp_req.znp.cmd0 = (ZNP_MT_CMD_SREQ | ZNP_MT_SYS_SYS);
    znp_req.znp.cmd1 = ZNP_SYS_GPIO;
    znp_req.znp.payload = (uint8_t *)req_cmd;
    znp_req.znp.payload_len = sizeof(s_zb_znp_mt_sys_gpio_cmd_t);
    znp_req.znp.resp_data = &rsp_data;
    znp_req.znp.resp_len = 1;
    status = zb_znp_send_cmd_req(&znp_req);

    if (status == ZB_OK)
    {
        if (rsp)
        {
            rsp->value = rsp_data;
        }
    }

    return status;
}

int
zb_znp_mt_sys_stack_tune(
    const s_zb_znp_mt_sys_stack_tune_cmd_t *req_cmd,
    s_zb_znp_mt_sys_stack_tune_srsp_t *rsp)
{
    int status;
    uint8_t rsp_data;
    s_zb_znp_req_t znp_req;

    znp_req.type = ZNP_CMD_REQ_TYPE_ZNP;
    znp_req.start_tick = zb_os_now_ms();
    znp_req.wants_response = true;
    znp_req.timeout_ms = ZNP_MT_SRSP_DEFAULT_TIMEOUT_MS;
    znp_req.znp.cmd0 = (ZNP_MT_CMD_SREQ | ZNP_MT_SYS_SYS);
    znp_req.znp.cmd1 = ZNP_SYS_STACK_TUNE;
    znp_req.znp.payload = (uint8_t *)req_cmd;
    znp_req.znp.payload_len = sizeof(s_zb_znp_mt_sys_stack_tune_cmd_t);
    znp_req.znp.resp_data = &rsp_data;
    znp_req.znp.resp_len = 1;
    status = zb_znp_send_cmd_req(&znp_req);

    if (status == ZB_OK)
    {
        if (rsp)
        {
            rsp->value = rsp_data;
        }
    }

    return status;
}

int
zb_znp_mt_sys_set_tx_power(const s_zb_znp_mt_sys_set_tx_power_cmd_t *req_cmd)
{
    int status;
    uint8_t rsp_data;
    s_zb_znp_req_t znp_req;

    znp_req.type = ZNP_CMD_REQ_TYPE_ZNP;
    znp_req.start_tick = zb_os_now_ms();
    znp_req.wants_response = true;
    znp_req.timeout_ms = ZNP_MT_SRSP_DEFAULT_TIMEOUT_MS;
    znp_req.znp.cmd0 = (ZNP_MT_CMD_SREQ | ZNP_MT_SYS_SYS);
    znp_req.znp.cmd1 = ZNP_SYS_SET_TX_POWER;
    znp_req.znp.payload = (uint8_t *)req_cmd;
    znp_req.znp.payload_len = sizeof(s_zb_znp_mt_sys_set_tx_power_cmd_t);
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
zb_znp_mt_sys_get_tx_power(s_zb_znp_mt_sys_get_tx_power_srsp_t *rsp)
{
    int status;
    uint8_t rsp_data[2];
    s_zb_znp_req_t znp_req;

    znp_req.type = ZNP_CMD_REQ_TYPE_ZNP;
    znp_req.start_tick = zb_os_now_ms();
    znp_req.wants_response = true;
    znp_req.timeout_ms = ZNP_MT_SRSP_DEFAULT_TIMEOUT_MS;
    znp_req.znp.cmd0 = (ZNP_MT_CMD_SREQ | ZNP_MT_SYS_SYS);
    znp_req.znp.cmd1 = ZNP_SYS_GET_TX_POWER;
    znp_req.znp.payload = NULL;
    znp_req.znp.payload_len = 0;
    znp_req.znp.resp_data = rsp_data;
    znp_req.znp.resp_len = 2;
    status = zb_znp_send_cmd_req(&znp_req);

    if (status == ZB_OK)
    {
        if (rsp)
        {
            status = rsp_data[0];
            rsp->tx_power = rsp_data[1];
        }
    }

    return status;
}

int
zb_znp_mt_sys_get_temperature(int16_t *rsp)
{
    int status;
    uint8_t rsp_data[2];
    s_zb_znp_req_t znp_req;

    znp_req.type = ZNP_CMD_REQ_TYPE_ZNP;
    znp_req.start_tick = zb_os_now_ms();
    znp_req.wants_response = true;
    znp_req.timeout_ms = ZNP_MT_SRSP_DEFAULT_TIMEOUT_MS;
    znp_req.znp.cmd0 = (ZNP_MT_CMD_SREQ | ZNP_MT_SYS_SYS);
    znp_req.znp.cmd1 = ZNP_SYS_GET_TEMPERATURE;
    znp_req.znp.payload = NULL;
    znp_req.znp.payload_len = 0;
    znp_req.znp.resp_data = rsp_data;
    znp_req.znp.resp_len = 2;
    status = zb_znp_send_cmd_req(&znp_req);

    if (status == ZB_OK)
    {
        if (rsp)
        {
            *rsp = (int16_t)(rsp_data[0] | (rsp_data[1] << 8));
        }
    }

    return status;
}

int
zb_znp_mt_sys_get_heap_statistics(uint32_t *free_heap, uint32_t *total_heap)
{
    int status;
    uint8_t rsp_data[8];
    s_zb_znp_req_t znp_req;

    znp_req.type = ZNP_CMD_REQ_TYPE_ZNP;
    znp_req.start_tick = zb_os_now_ms();
    znp_req.wants_response = true;
    znp_req.timeout_ms = ZNP_MT_SRSP_DEFAULT_TIMEOUT_MS;
    znp_req.znp.cmd0 = (ZNP_MT_CMD_SREQ | ZNP_MT_SYS_SYS);
    znp_req.znp.cmd1 = ZNP_SYS_GET_HEAP_STATISTICS;
    znp_req.znp.payload = NULL;
    znp_req.znp.payload_len = 0;
    znp_req.znp.resp_data = rsp_data;
    znp_req.znp.resp_len = 8;
    status = zb_znp_send_cmd_req(&znp_req);

    ZB_LOG_BUFFER_HEX(TAG, rsp_data, 8);

    if (status == ZB_OK)
    {
        if (free_heap && total_heap)
        {
            *free_heap = BUILD_UINT32(rsp_data[0], rsp_data[1], rsp_data[2], rsp_data[3]);
            *total_heap = BUILD_UINT32(rsp_data[4], rsp_data[5], rsp_data[6], rsp_data[7]);
        }
    }

    return status;
}
int
zb_znp_mt_sys_get_time(s_zb_znp_mt_sys_get_time_srsp_t *rsp)
{
    int status;
    uint8_t rsp_data[sizeof(s_zb_znp_mt_sys_get_time_srsp_t)];
    s_zb_znp_req_t znp_req;

    znp_req.type = ZNP_CMD_REQ_TYPE_ZNP;
    znp_req.start_tick = zb_os_now_ms();
    znp_req.wants_response = true;
    znp_req.timeout_ms = ZNP_MT_SRSP_DEFAULT_TIMEOUT_MS;
    znp_req.znp.cmd0 = (ZNP_MT_CMD_SREQ | ZNP_MT_SYS_SYS);
    znp_req.znp.cmd1 = ZNP_SYS_GET_TIME;
    znp_req.znp.payload = NULL;
    znp_req.znp.payload_len = 0;
    znp_req.znp.resp_data = rsp_data;
    znp_req.znp.resp_len = sizeof(s_zb_znp_mt_sys_get_time_srsp_t);
    status = zb_znp_send_cmd_req(&znp_req);

    if (status == ZB_OK)
    {
        if (rsp)
        {
            memcpy(rsp, rsp_data, sizeof(s_zb_znp_mt_sys_get_time_srsp_t));
        }
    }

    return status;
}
