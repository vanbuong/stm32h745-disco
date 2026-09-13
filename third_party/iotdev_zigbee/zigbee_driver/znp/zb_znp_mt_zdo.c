/*
 * zb_znp_mt_zdo.c
 *
 * This file contains the implementation of the MT ZDO interface.
 *  
 * Author: Vo Van Buong (BRT-SG)
 */

#include "zb_znp.h"
#include "zb_znp_mt_zdo.h"

#define TAG "ZB_ZNP_ZDO"

/**************************************************************************************************
 * LOCAL VARIABLES
 *************************************************************************************************/
static s_zb_znp_mt_zdo_cb_t g_zb_znp_mt_zdo_cb = {0};

/**************************************************************************************************
 * LOCAL FUNCTIONS
 *************************************************************************************************/
static void zb_znp_mt_zdo_process_nwk_addr_rsp(const uint8_t *rpc_buff, uint8_t rpc_len);
static void zb_znp_mt_zdo_process_ieee_addr_rsp(const uint8_t *rpc_buff, uint8_t rpc_len);
static void zb_znp_mt_zdo_process_node_desc_rsp(const uint8_t *rpc_buff, uint8_t rpc_len);
static void zb_znp_mt_zdo_process_power_desc_rsp(const uint8_t *rpc_buff, uint8_t rpc_len);
static void zb_znp_mt_zdo_process_simple_desc_rsp(const uint8_t *rpc_buff, uint8_t rpc_len);
static void zb_znp_mt_zdo_process_active_ep_rsp(const uint8_t *rpc_buff, uint8_t rpc_len);
static void zb_znp_mt_zdo_process_match_desc_rsp(const uint8_t *rpc_buff, uint8_t rpc_len);
static void zb_znp_mt_zdo_process_complex_desc_rsp(const uint8_t *rpc_buff, uint8_t rpc_len);
static void zb_znp_mt_zdo_process_user_desc_rsp(const uint8_t *rpc_buff, uint8_t rpc_len);
static void zb_znp_mt_zdo_process_user_desc_conf(const uint8_t *rpc_buff, uint8_t rpc_len);
static void zb_znp_mt_zdo_process_server_disc_rsp(const uint8_t *rpc_buff, uint8_t rpc_len);
static void zb_znp_mt_zdo_process_end_device_bind_rsp(const uint8_t *rpc_buff, uint8_t rpc_len);
static void zb_znp_mt_zdo_process_bind_rsp(const uint8_t *rpc_buff, uint8_t rpc_len);
static void zb_znp_mt_zdo_process_unbind_rsp(const uint8_t *rpc_buff, uint8_t rpc_len);
static void zb_znp_mt_zdo_process_mgmt_nwk_disc_rsp(const uint8_t *rpc_buff, uint8_t rpc_len);
static void zb_znp_mt_zdo_process_mgmt_lqi_rsp(const uint8_t *rpc_buff, uint8_t rpc_len);
static void zb_znp_mt_zdo_process_mgmt_rtg_rsp(const uint8_t *rpc_buff, uint8_t rpc_len);
static void zb_znp_mt_zdo_process_mgmt_bind_rsp(const uint8_t *rpc_buff, uint8_t rpc_len);
static void zb_znp_mt_zdo_process_mgmt_leave_rsp(const uint8_t *rpc_buff, uint8_t rpc_len);
static void zb_znp_mt_zdo_process_mgmt_direct_join_rsp(const uint8_t *rpc_buff, uint8_t rpc_len);
static void zb_znp_mt_zdo_process_mgmt_permit_join_rsp(const uint8_t *rpc_buff, uint8_t rpc_len);
static void zb_znp_mt_zdo_process_state_change_ind(const uint8_t *rpc_buff, uint8_t rpc_len);
static void zb_znp_mt_zdo_process_end_device_annce_ind(const uint8_t *rpc_buff, uint8_t rpc_len);
static void zb_znp_mt_zdo_process_match_desc_rsp_sent(const uint8_t *rpc_buff, uint8_t rpc_len);
static void zb_znp_mt_zdo_process_status_error_rsp(const uint8_t *rpc_buff, uint8_t rpc_len);
static void zb_znp_mt_zdo_process_src_rtg_ind(const uint8_t *rpc_buff, uint8_t rpc_len);
static void zb_znp_mt_zdo_process_beacon_notify_ind(const uint8_t *rpc_buff, uint8_t rpc_len);
static void zb_znp_mt_zdo_process_join_cnf(const uint8_t *rpc_buff, uint8_t rpc_len);
static void zb_znp_mt_zdo_process_nwk_discovery_cnf(const uint8_t *rpc_buff, uint8_t rpc_len);
static void zb_znp_mt_zdo_process_leave_ind(const uint8_t *rpc_buff, uint8_t rpc_len);
static void zb_znp_mt_zdo_process_msg_cb_incoming(const uint8_t *rpc_buff, uint8_t rpc_len);
static void zb_znp_mt_zdo_process_tc_device_ind(const uint8_t *rpc_buff, uint8_t rpc_len);

/**************************************************************************************************
 * FUNCTIONS IMPLEMENTATION
 *************************************************************************************************/
void
zb_znp_mt_zdo_register_callback(s_zb_znp_mt_zdo_cb_t callback)
{
    memcpy(&g_zb_znp_mt_zdo_cb, &callback, sizeof(s_zb_znp_mt_zdo_cb_t));
}

void
zb_znp_mt_zdo_unregister_callback(void)
{
    memset(&g_zb_znp_mt_zdo_cb, 0, sizeof(s_zb_znp_mt_zdo_cb_t));
}

int
zb_znp_mt_zdo_init(uint16_t start_delay)
{
    int status = ZB_OK;

    s_zb_znp_mt_zdo_startup_from_app_t req = {0};
    req.start_delay = start_delay;

    status = zb_znp_mt_zdo_startup_from_app(&req);

    return status;
}

void
zb_znp_mt_zdo_process(const uint8_t *rpc_buff, uint8_t rpc_len)
{
    ZB_LOGI(TAG, "MT_ZDO processing CMD: %02X%02X", rpc_buff[0], rpc_buff[1]);

    // Read CMD1 and processes the specific AREQ
    switch (rpc_buff[1])
    {
        case ZNP_ZDO_NWK_ADDR_RSP:
            zb_znp_mt_zdo_process_nwk_addr_rsp(rpc_buff, rpc_len);
            break;
        case ZNP_ZDO_IEEE_ADDR_RSP:
            zb_znp_mt_zdo_process_ieee_addr_rsp(rpc_buff, rpc_len);
            break;
        case ZNP_ZDO_NODE_DESC_RSP:
            zb_znp_mt_zdo_process_node_desc_rsp(rpc_buff, rpc_len);
            break;
        case ZNP_ZDO_POWER_DESC_RSP:
            zb_znp_mt_zdo_process_power_desc_rsp(rpc_buff, rpc_len);
            break;
        case ZNP_ZDO_SIMPLE_DESC_RSP:
            zb_znp_mt_zdo_process_simple_desc_rsp(rpc_buff, rpc_len);
            break;
        case ZNP_ZDO_ACTIVE_EP_RSP:
            zb_znp_mt_zdo_process_active_ep_rsp(rpc_buff, rpc_len);
            break;
        case ZNP_ZDO_MATCH_DESC_RSP:
            zb_znp_mt_zdo_process_match_desc_rsp(rpc_buff, rpc_len);
            break;
        case ZNP_ZDO_COMPLEX_DESC_RSP:
            zb_znp_mt_zdo_process_complex_desc_rsp(rpc_buff, rpc_len);
            break;
        case ZNP_ZDO_USER_DESC_RSP:
            zb_znp_mt_zdo_process_user_desc_rsp(rpc_buff, rpc_len);
            break;
        case ZNP_ZDO_USER_DESC_CONF:
            zb_znp_mt_zdo_process_user_desc_conf(rpc_buff, rpc_len);
            break;
        case ZNP_ZDO_SERVER_DISC_RSP:
            zb_znp_mt_zdo_process_server_disc_rsp(rpc_buff, rpc_len);
            break;
        case ZNP_ZDO_END_DEVICE_BIND_RSP:
            zb_znp_mt_zdo_process_end_device_bind_rsp(rpc_buff, rpc_len);
            break;
        case ZNP_ZDO_BIND_RSP:
            zb_znp_mt_zdo_process_bind_rsp(rpc_buff, rpc_len);
            break;
        case ZNP_ZDO_UNBIND_RSP:
            zb_znp_mt_zdo_process_unbind_rsp(rpc_buff, rpc_len);
            break;
        case ZNP_ZDO_MGMT_NWK_DISC_RSP:
            zb_znp_mt_zdo_process_mgmt_nwk_disc_rsp(rpc_buff, rpc_len);
            break;
        case ZNP_ZDO_MGMT_LQI_RSP:
            zb_znp_mt_zdo_process_mgmt_lqi_rsp(rpc_buff, rpc_len);
            break;
        case ZNP_ZDO_MGMT_RTG_RSP:
            zb_znp_mt_zdo_process_mgmt_rtg_rsp(rpc_buff, rpc_len);
            break;
        case ZNP_ZDO_MGMT_BIND_RSP:
            zb_znp_mt_zdo_process_mgmt_bind_rsp(rpc_buff, rpc_len);
            break;
        case ZNP_ZDO_MGMT_LEAVE_RSP:
            zb_znp_mt_zdo_process_mgmt_leave_rsp(rpc_buff, rpc_len);
            break;
        case ZNP_ZDO_MGMT_DIRECT_JOIN_RSP:
            zb_znp_mt_zdo_process_mgmt_direct_join_rsp(rpc_buff, rpc_len);
            break;
        case ZNP_ZDO_MGMT_PERMIT_JOIN_RSP:
            zb_znp_mt_zdo_process_mgmt_permit_join_rsp(rpc_buff, rpc_len);
            break;
        case ZNP_ZDO_STATE_CHANGE_IND:
            zb_znp_mt_zdo_process_state_change_ind(rpc_buff, rpc_len);
            break;
        case ZNP_ZDO_END_DEVICE_ANNCE_IND:
            zb_znp_mt_zdo_process_end_device_annce_ind(rpc_buff, rpc_len);
            break;
        case ZNP_ZDO_MATCH_DESC_RSP_SENT:
            zb_znp_mt_zdo_process_match_desc_rsp_sent(rpc_buff, rpc_len);
            break;
        case ZNP_ZDO_STATUS_ERROR_RSP:
            zb_znp_mt_zdo_process_status_error_rsp(rpc_buff, rpc_len);
            break;
        case ZNP_ZDO_SRC_RTG_IND:
            zb_znp_mt_zdo_process_src_rtg_ind(rpc_buff, rpc_len);
            break;
        case ZNP_ZDO_BEACON_NOTIFY_IND:
            zb_znp_mt_zdo_process_beacon_notify_ind(rpc_buff, rpc_len);
            break;
        case ZNP_ZDO_JOIN_CNF:
            zb_znp_mt_zdo_process_join_cnf(rpc_buff, rpc_len);
            break;
        case ZNP_ZDO_NWK_DISCOVERY_CNF:
            zb_znp_mt_zdo_process_nwk_discovery_cnf(rpc_buff, rpc_len);
            break;
        case ZNP_ZDO_LEAVE_IND:
            zb_znp_mt_zdo_process_leave_ind(rpc_buff, rpc_len);
            break;
        case ZNP_ZDO_MSG_CB_INCOMING:
            zb_znp_mt_zdo_process_msg_cb_incoming(rpc_buff, rpc_len);
            break;
        case ZNP_ZDO_TC_DEVICE_IND:
            zb_znp_mt_zdo_process_tc_device_ind(rpc_buff, rpc_len);
            break;
        default:
            ZB_LOGW(TAG, "Not handled MT_ZDO_RSP CMD: %02X%02X", rpc_buff[0], rpc_buff[1]);
            break;
    }
}

int
zb_znp_mt_zdo_nwk_addr_req(const s_zb_znp_mt_zdo_nwk_addr_req_t *req)
{
    int status;
    uint8_t rsp_data;
    s_zb_znp_req_t znp_req;

    znp_req.type = ZNP_CMD_REQ_TYPE_ZNP;
    znp_req.start_tick = zb_os_now_ms();
    znp_req.wants_response = true;
    znp_req.timeout_ms = ZNP_MT_SRSP_DEFAULT_TIMEOUT_MS;
    znp_req.znp.cmd0 = (ZNP_MT_CMD_SREQ | ZNP_MT_SYS_ZDO);
    znp_req.znp.cmd1 = ZNP_ZDO_NWK_ADDR_REQ;
    znp_req.znp.payload = (uint8_t *)req;
    znp_req.znp.payload_len = sizeof(s_zb_znp_mt_zdo_nwk_addr_req_t);
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
zb_znp_mt_zdo_process_nwk_addr_rsp(const uint8_t *rpc_buff, uint8_t rpc_len)
{
    if (g_zb_znp_mt_zdo_cb.pfn_zdo_nwk_addr_rsp_cb)
    {
        const s_zb_znp_mt_zdo_nwk_addr_rsp_t *rsp = (const s_zb_znp_mt_zdo_nwk_addr_rsp_t *)&rpc_buff[2];

        g_zb_znp_mt_zdo_cb.pfn_zdo_nwk_addr_rsp_cb(rsp);
    }
}

int
zb_znp_mt_zdo_ieee_addr_req(const s_zb_znp_mt_zdo_ieee_addr_req_t *req)
{
    int status;
    uint8_t rsp_data;
    s_zb_znp_req_t znp_req;

    znp_req.type = ZNP_CMD_REQ_TYPE_ZNP;
    znp_req.start_tick = zb_os_now_ms();
    znp_req.wants_response = true;
    znp_req.timeout_ms = ZNP_MT_SRSP_DEFAULT_TIMEOUT_MS;
    znp_req.znp.cmd0 = (ZNP_MT_CMD_SREQ | ZNP_MT_SYS_ZDO);
    znp_req.znp.cmd1 = ZNP_ZDO_IEEE_ADDR_REQ;
    znp_req.znp.payload = (uint8_t *)req;
    znp_req.znp.payload_len = sizeof(s_zb_znp_mt_zdo_ieee_addr_req_t);
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
zb_znp_mt_zdo_process_ieee_addr_rsp(const uint8_t *rpc_buff, uint8_t rpc_len)
{
    if (g_zb_znp_mt_zdo_cb.pfn_zdo_ieee_addr_rsp_cb)
    {
        const s_zb_znp_mt_zdo_ieee_addr_rsp_t *rsp = (const s_zb_znp_mt_zdo_ieee_addr_rsp_t *)&rpc_buff[2];

        g_zb_znp_mt_zdo_cb.pfn_zdo_ieee_addr_rsp_cb(rsp);
    }
}

int
zb_znp_mt_zdo_node_desc_req(const s_zb_znp_mt_zdo_node_desc_req_t *req)
{
    int status;
    uint8_t rsp_data;
    s_zb_znp_req_t znp_req;

    znp_req.type = ZNP_CMD_REQ_TYPE_ZNP;
    znp_req.start_tick = zb_os_now_ms();
    znp_req.wants_response = true;
    znp_req.timeout_ms = ZNP_MT_SRSP_DEFAULT_TIMEOUT_MS;
    znp_req.znp.cmd0 = (ZNP_MT_CMD_SREQ | ZNP_MT_SYS_ZDO);
    znp_req.znp.cmd1 = ZNP_ZDO_NODE_DESC_REQ;
    znp_req.znp.payload = (uint8_t *)req;
    znp_req.znp.payload_len = sizeof(s_zb_znp_mt_zdo_node_desc_req_t);
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
zb_znp_mt_zdo_process_node_desc_rsp(const uint8_t *rpc_buff, uint8_t rpc_len)
{
    if (g_zb_znp_mt_zdo_cb.pfn_zdo_node_desc_rsp_cb)
    {
        const s_zb_znp_mt_zdo_node_desc_rsp_t *rsp = (const s_zb_znp_mt_zdo_node_desc_rsp_t *)&rpc_buff[2];

        g_zb_znp_mt_zdo_cb.pfn_zdo_node_desc_rsp_cb(rsp);
    }
}

int
zb_znp_mt_zdo_power_desc_req(const s_zb_znp_mt_zdo_power_desc_req_t *req)
{
    int status;
    uint8_t rsp_data;
    s_zb_znp_req_t znp_req;

    znp_req.type = ZNP_CMD_REQ_TYPE_ZNP;
    znp_req.start_tick = zb_os_now_ms();
    znp_req.wants_response = true;
    znp_req.timeout_ms = ZNP_MT_SRSP_DEFAULT_TIMEOUT_MS;
    znp_req.znp.cmd0 = (ZNP_MT_CMD_SREQ | ZNP_MT_SYS_ZDO);
    znp_req.znp.cmd1 = ZNP_ZDO_POWER_DESC_REQ;
    znp_req.znp.payload = (uint8_t *)req;
    znp_req.znp.payload_len = sizeof(s_zb_znp_mt_zdo_power_desc_req_t);
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
zb_znp_mt_zdo_process_power_desc_rsp(const uint8_t *rpc_buff, uint8_t rpc_len)
{
    if (g_zb_znp_mt_zdo_cb.pfn_zdo_power_desc_rsp_cb)
    {
        const s_zb_znp_mt_zdo_power_desc_rsp_t *rsp = (const s_zb_znp_mt_zdo_power_desc_rsp_t *)&rpc_buff[2];

        g_zb_znp_mt_zdo_cb.pfn_zdo_power_desc_rsp_cb(rsp);
    }
}

int
zb_znp_mt_zdo_simple_desc_req(const s_zb_znp_mt_zdo_simple_desc_req_t *req)
{
    int status;
    uint8_t rsp_data;
    s_zb_znp_req_t znp_req;

    znp_req.type = ZNP_CMD_REQ_TYPE_ZNP;
    znp_req.start_tick = zb_os_now_ms();
    znp_req.wants_response = true;
    znp_req.timeout_ms = ZNP_MT_SRSP_DEFAULT_TIMEOUT_MS;
    znp_req.znp.cmd0 = (ZNP_MT_CMD_SREQ | ZNP_MT_SYS_ZDO);
    znp_req.znp.cmd1 = ZNP_ZDO_SIMPLE_DESC_REQ;
    znp_req.znp.payload = (uint8_t *)req;
    znp_req.znp.payload_len = sizeof(s_zb_znp_mt_zdo_simple_desc_req_t);
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
zb_znp_mt_zdo_process_simple_desc_rsp(const uint8_t *rpc_buff, uint8_t rpc_len)
{
    if (g_zb_znp_mt_zdo_cb.pfn_zdo_simple_desc_rsp_cb)
    {
        s_zb_znp_mt_zdo_simple_desc_rsp_t rsp;
        size_t copy_len = sizeof(s_zb_znp_mt_zdo_simple_desc_rsp_t) > rpc_len ? rpc_len : sizeof(s_zb_znp_mt_zdo_simple_desc_rsp_t);
        memcpy(&rsp, &rpc_buff[2], copy_len);
        uint8_t *num_out_clusters = (uint8_t *)(&rsp.num_in_clusters + 1 + 2 * rsp.num_in_clusters);
        rsp.num_out_clusters = *(num_out_clusters);
        memcpy(rsp.out_cluster_list, num_out_clusters + 1, rsp.num_out_clusters * 2);

        g_zb_znp_mt_zdo_cb.pfn_zdo_simple_desc_rsp_cb(&rsp);
    }
}

int
zb_znp_mt_zdo_active_ep_req(const s_zb_znp_mt_zdo_active_ep_req_t *req)
{
    int status;
    uint8_t rsp_data;
    s_zb_znp_req_t znp_req;

    znp_req.type = ZNP_CMD_REQ_TYPE_ZNP;
    znp_req.start_tick = zb_os_now_ms();
    znp_req.wants_response = true;
    znp_req.timeout_ms = ZNP_MT_SRSP_DEFAULT_TIMEOUT_MS;
    znp_req.znp.cmd0 = (ZNP_MT_CMD_SREQ | ZNP_MT_SYS_ZDO);
    znp_req.znp.cmd1 = ZNP_ZDO_ACTIVE_EP_REQ;
    znp_req.znp.payload = (uint8_t *)req;
    znp_req.znp.payload_len = sizeof(s_zb_znp_mt_zdo_active_ep_req_t);
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
zb_znp_mt_zdo_process_active_ep_rsp(const uint8_t *rpc_buff, uint8_t rpc_len)
{
    if (g_zb_znp_mt_zdo_cb.pfn_zdo_active_ep_rsp_cb)
    {
        const s_zb_znp_mt_zdo_active_ep_rsp_t *rsp = (const s_zb_znp_mt_zdo_active_ep_rsp_t *)&rpc_buff[2];

        g_zb_znp_mt_zdo_cb.pfn_zdo_active_ep_rsp_cb(rsp);
    }
}

int
zb_znp_mt_zdo_match_desc_req(const s_zb_znp_mt_zdo_match_desc_req_t *req)
{
    int status;
    uint8_t cmd_idx = 0;
    uint8_t cmd_len = req->num_in_clusters * 2 + req->num_out_clusters * 2 + 8;
    uint8_t *cmd_buff = (uint8_t *)ZB_MEM_MALLOC(cmd_len);
    uint8_t rsp_data;
    s_zb_znp_req_t znp_req;

    if (cmd_buff)
    {
        memcpy(cmd_buff, req, 7);
        cmd_idx = 7;
        memcpy(&cmd_buff[cmd_idx], req->in_cluster_list, req->num_in_clusters * 2);
        cmd_idx += req->num_in_clusters * 2;
        cmd_buff[cmd_idx++] = req->num_out_clusters;
        memcpy(&cmd_buff[cmd_idx], req->out_cluster_list, req->num_out_clusters * 2);

        znp_req.type = ZNP_CMD_REQ_TYPE_ZNP;
        znp_req.start_tick = zb_os_now_ms();
        znp_req.wants_response = true;
        znp_req.timeout_ms = ZNP_MT_SRSP_DEFAULT_TIMEOUT_MS;
        znp_req.znp.cmd0 = (ZNP_MT_CMD_SREQ | ZNP_MT_SYS_ZDO);
        znp_req.znp.cmd1 = ZNP_ZDO_MATCH_DESC_REQ;
        znp_req.znp.payload = cmd_buff;
        znp_req.znp.payload_len = cmd_len;
        znp_req.znp.resp_data = &rsp_data;
        znp_req.znp.resp_len = 1;
        status = zb_znp_send_cmd_req(&znp_req);
        
        if (status == ZB_OK)
        {
            status = rsp_data;
        }

        ZB_MEM_FREE(cmd_buff);
        return status;
    }
    else
    {
        ZB_LOGE(TAG, "%s(): Memory for cmd was not allocated", __func__);
        return ZB_MEM_ERROR;
    }
}

static void
zb_znp_mt_zdo_process_match_desc_rsp(const uint8_t *rpc_buff, uint8_t rpc_len)
{
    if (g_zb_znp_mt_zdo_cb.pfn_zdo_match_desc_rsp_cb)
    {
        const s_zb_znp_mt_zdo_match_desc_rsp_t *rsp = (const s_zb_znp_mt_zdo_match_desc_rsp_t *)&rpc_buff[2];

        g_zb_znp_mt_zdo_cb.pfn_zdo_match_desc_rsp_cb(rsp);
    }
}

int
zb_znp_mt_zdo_complex_desc_req(const s_zb_znp_mt_zdo_complex_desc_req_t *req)
{
    int status;
    uint8_t rsp_data;
    s_zb_znp_req_t znp_req;

    znp_req.type = ZNP_CMD_REQ_TYPE_ZNP;
    znp_req.start_tick = zb_os_now_ms();
    znp_req.wants_response = true;
    znp_req.timeout_ms = ZNP_MT_SRSP_DEFAULT_TIMEOUT_MS;
    znp_req.znp.cmd0 = (ZNP_MT_CMD_SREQ | ZNP_MT_SYS_ZDO);
    znp_req.znp.cmd1 = ZNP_ZDO_COMPLEX_DESC_REQ;
    znp_req.znp.payload = (uint8_t *)req;
    znp_req.znp.payload_len = sizeof(s_zb_znp_mt_zdo_complex_desc_req_t);
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
zb_znp_mt_zdo_process_complex_desc_rsp(const uint8_t *rpc_buff, uint8_t rpc_len)
{
    if (g_zb_znp_mt_zdo_cb.pfn_zdo_complex_desc_rsp_cb)
    {
        const s_zb_znp_mt_zdo_complex_desc_rsp_t *rsp = (const s_zb_znp_mt_zdo_complex_desc_rsp_t *)&rpc_buff[2];

        g_zb_znp_mt_zdo_cb.pfn_zdo_complex_desc_rsp_cb(rsp);
    }
}

int
zb_znp_mt_zdo_user_desc_req(const s_zb_znp_mt_zdo_user_desc_req_t *req)
{
    int status;
    uint8_t rsp_data;
    s_zb_znp_req_t znp_req;

    znp_req.type = ZNP_CMD_REQ_TYPE_ZNP;
    znp_req.start_tick = zb_os_now_ms();
    znp_req.wants_response = true;
    znp_req.timeout_ms = ZNP_MT_SRSP_DEFAULT_TIMEOUT_MS;
    znp_req.znp.cmd0 = (ZNP_MT_CMD_SREQ | ZNP_MT_SYS_ZDO);
    znp_req.znp.cmd1 = ZNP_ZDO_USER_DESC_REQ;
    znp_req.znp.payload = (uint8_t *)req;
    znp_req.znp.payload_len = sizeof(s_zb_znp_mt_zdo_user_desc_req_t);
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
zb_znp_mt_zdo_process_user_desc_rsp(const uint8_t *rpc_buff, uint8_t rpc_len)
{
    if (g_zb_znp_mt_zdo_cb.pfn_zdo_user_desc_rsp_cb)
    {
        const s_zb_znp_mt_zdo_user_desc_rsp_t *rsp = (const s_zb_znp_mt_zdo_user_desc_rsp_t *)&rpc_buff[2];

        g_zb_znp_mt_zdo_cb.pfn_zdo_user_desc_rsp_cb(rsp);
    }
}

int
zb_znp_mt_zdo_user_desc_set(const s_zb_znp_mt_zdo_user_desc_set_t *req)
{
    int status;
    uint8_t rsp_data;
    s_zb_znp_req_t znp_req;

    znp_req.type = ZNP_CMD_REQ_TYPE_ZNP;
    znp_req.start_tick = zb_os_now_ms();
    znp_req.wants_response = true;
    znp_req.timeout_ms = ZNP_MT_SRSP_DEFAULT_TIMEOUT_MS;
    znp_req.znp.cmd0 = (ZNP_MT_CMD_SREQ | ZNP_MT_SYS_ZDO);
    znp_req.znp.cmd1 = ZNP_ZDO_USER_DESC_SET;
    znp_req.znp.payload = (uint8_t *)req;
    znp_req.znp.payload_len = sizeof(s_zb_znp_mt_zdo_user_desc_set_t);
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
zb_znp_mt_zdo_process_user_desc_conf(const uint8_t *rpc_buff, uint8_t rpc_len)
{
    if (g_zb_znp_mt_zdo_cb.pfn_zdo_user_desc_conf_cb)
    {
        const s_zb_znp_mt_zdo_user_desc_conf_t *rsp = (const s_zb_znp_mt_zdo_user_desc_conf_t *)&rpc_buff[2];

        g_zb_znp_mt_zdo_cb.pfn_zdo_user_desc_conf_cb(rsp);
    }
}

int
zb_znp_mt_zdo_server_disc_req(const s_zb_znp_mt_zdo_server_disc_req_t *req)
{
    int status;
    uint8_t rsp_data;
    s_zb_znp_req_t znp_req;

    znp_req.type = ZNP_CMD_REQ_TYPE_ZNP;
    znp_req.start_tick = zb_os_now_ms();
    znp_req.wants_response = true;
    znp_req.timeout_ms = ZNP_MT_SRSP_DEFAULT_TIMEOUT_MS;
    znp_req.znp.cmd0 = (ZNP_MT_CMD_SREQ | ZNP_MT_SYS_ZDO);
    znp_req.znp.cmd1 = ZNP_ZDO_SERVER_DISC_REQ;
    znp_req.znp.payload = (uint8_t *)req;
    znp_req.znp.payload_len = sizeof(s_zb_znp_mt_zdo_server_disc_req_t);
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
zb_znp_mt_zdo_process_server_disc_rsp(const uint8_t *rpc_buff, uint8_t rpc_len)
{
    if (g_zb_znp_mt_zdo_cb.pfn_zdo_server_disc_rsp_cb)
    {
        const s_zb_znp_mt_zdo_server_disc_rsp_t *rsp = (const s_zb_znp_mt_zdo_server_disc_rsp_t *)&rpc_buff[2];

        g_zb_znp_mt_zdo_cb.pfn_zdo_server_disc_rsp_cb(rsp);
    }
}

int
zb_znp_mt_zdo_end_device_bind_req(const s_zb_znp_mt_zdo_end_device_bind_req_t *req)
{
    int status;
    uint8_t rsp_data;
    s_zb_znp_req_t znp_req;
    uint8_t cmd_idx = 0;
    uint8_t cmd_len = req->num_in_clusters * 2 + req->num_out_clusters * 2 + 17;
    uint8_t *cmd_buff = (uint8_t *)ZB_MEM_MALLOC(cmd_len);

    if (cmd_buff)
    {
        memcpy(cmd_buff, req, 16);
        cmd_idx = 16;
        memcpy(&cmd_buff[cmd_idx], req->in_cluster_list, req->num_in_clusters * 2);
        cmd_idx += req->num_in_clusters * 2;
        cmd_buff[cmd_idx++] = req->num_out_clusters;
        memcpy(&cmd_buff[cmd_idx], req->out_cluster_list, req->num_out_clusters * 2);    

        znp_req.type = ZNP_CMD_REQ_TYPE_ZNP;
        znp_req.start_tick = zb_os_now_ms();
        znp_req.wants_response = true;
        znp_req.timeout_ms = ZNP_MT_SRSP_DEFAULT_TIMEOUT_MS;
        znp_req.znp.cmd0 = (ZNP_MT_CMD_SREQ | ZNP_MT_SYS_ZDO);
        znp_req.znp.cmd1 = ZNP_ZDO_END_DEVICE_BIND_REQ;
        znp_req.znp.payload = cmd_buff;
        znp_req.znp.payload_len = cmd_len;
        znp_req.znp.resp_data = &rsp_data;
        znp_req.znp.resp_len = 1;
        status = zb_znp_send_cmd_req(&znp_req);

        if (status == ZB_OK)
        {
            status = rsp_data;
        }

        ZB_MEM_FREE(cmd_buff);
        return status;
    }
    else
    {
        ZB_LOGE(TAG, "%s(): Memory for cmd was not allocated", __func__);
        return ZB_MEM_ERROR;
    }
}

static void
zb_znp_mt_zdo_process_end_device_bind_rsp(const uint8_t *rpc_buff, uint8_t rpc_len)
{
    if (g_zb_znp_mt_zdo_cb.pfn_zdo_end_device_bind_rsp_cb)
    {
        const s_zb_znp_mt_zdo_end_device_bind_rsp_t *rsp = (const s_zb_znp_mt_zdo_end_device_bind_rsp_t *)&rpc_buff[2];

        g_zb_znp_mt_zdo_cb.pfn_zdo_end_device_bind_rsp_cb(rsp);
    }
}

int
zb_znp_mt_zdo_bind_device_req(const s_zb_znp_mt_zdo_bind_unbind_device_req_t *req)
{
    int status;
    uint8_t rsp_data;
    s_zb_znp_req_t znp_req;

    znp_req.type = ZNP_CMD_REQ_TYPE_ZNP;
    znp_req.start_tick = zb_os_now_ms();
    znp_req.wants_response = true;
    znp_req.timeout_ms = ZNP_MT_SRSP_DEFAULT_TIMEOUT_MS;
    znp_req.znp.cmd0 = (ZNP_MT_CMD_SREQ | ZNP_MT_SYS_ZDO);
    znp_req.znp.cmd1 = ZNP_ZDO_BIND_REQ;
    znp_req.znp.payload = (uint8_t *)req;
    znp_req.znp.payload_len = sizeof(s_zb_znp_mt_zdo_bind_unbind_device_req_t);
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
zb_znp_mt_zdo_bind_group_req(const s_zb_znp_mt_zdo_bind_unbind_group_req_t *req)
{
    int status;
    uint8_t rsp_data;
    s_zb_znp_req_t znp_req;

    znp_req.type = ZNP_CMD_REQ_TYPE_ZNP;
    znp_req.start_tick = zb_os_now_ms();
    znp_req.wants_response = true;
    znp_req.timeout_ms = ZNP_MT_SRSP_DEFAULT_TIMEOUT_MS;
    znp_req.znp.cmd0 = (ZNP_MT_CMD_SREQ | ZNP_MT_SYS_ZDO);
    znp_req.znp.cmd1 = ZNP_ZDO_BIND_REQ;
    znp_req.znp.payload = (uint8_t *)req;
    znp_req.znp.payload_len = sizeof(s_zb_znp_mt_zdo_bind_unbind_group_req_t);
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
zb_znp_mt_zdo_process_bind_rsp(const uint8_t *rpc_buff, uint8_t rpc_len)
{
    if (g_zb_znp_mt_zdo_cb.pfn_zdo_bind_rsp_cb)
    {
        const s_zb_znp_mt_zdo_bind_unbind_rsp_t *rsp = (const s_zb_znp_mt_zdo_bind_unbind_rsp_t *)&rpc_buff[2];

        g_zb_znp_mt_zdo_cb.pfn_zdo_bind_rsp_cb(rsp);
    }
}

int
zb_znp_mt_zdo_unbind_device_req(const s_zb_znp_mt_zdo_bind_unbind_device_req_t *req)
{
    int status;
    uint8_t rsp_data;
    s_zb_znp_req_t znp_req;

    znp_req.type = ZNP_CMD_REQ_TYPE_ZNP;
    znp_req.start_tick = zb_os_now_ms();
    znp_req.wants_response = true;
    znp_req.timeout_ms = ZNP_MT_SRSP_DEFAULT_TIMEOUT_MS;
    znp_req.znp.cmd0 = (ZNP_MT_CMD_SREQ | ZNP_MT_SYS_ZDO);
    znp_req.znp.cmd1 = ZNP_ZDO_UNBIND_REQ;
    znp_req.znp.payload = (uint8_t *)req;
    znp_req.znp.payload_len = sizeof(s_zb_znp_mt_zdo_bind_unbind_device_req_t);
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
zb_znp_mt_zdo_unbind_group_req(const s_zb_znp_mt_zdo_bind_unbind_group_req_t *req)
{
    int status;
    uint8_t rsp_data;
    s_zb_znp_req_t znp_req;

    znp_req.type = ZNP_CMD_REQ_TYPE_ZNP;
    znp_req.start_tick = zb_os_now_ms();
    znp_req.wants_response = true;
    znp_req.timeout_ms = ZNP_MT_SRSP_DEFAULT_TIMEOUT_MS;
    znp_req.znp.cmd0 = (ZNP_MT_CMD_SREQ | ZNP_MT_SYS_ZDO);
    znp_req.znp.cmd1 = ZNP_ZDO_UNBIND_REQ;
    znp_req.znp.payload = (uint8_t *)req;
    znp_req.znp.payload_len = sizeof(s_zb_znp_mt_zdo_bind_unbind_group_req_t);
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
zb_znp_mt_zdo_process_unbind_rsp(const uint8_t *rpc_buff, uint8_t rpc_len)
{
    if (g_zb_znp_mt_zdo_cb.pfn_zdo_unbind_rsp_cb)
    {
        const s_zb_znp_mt_zdo_bind_unbind_rsp_t *rsp = (const s_zb_znp_mt_zdo_bind_unbind_rsp_t *)&rpc_buff[2];

        g_zb_znp_mt_zdo_cb.pfn_zdo_unbind_rsp_cb(rsp);
    }
}

int
zb_znp_mt_zdo_mgmt_nwk_disc_req(const s_zb_znp_mt_zdo_mgmt_nwk_disc_req_t *req)
{
    int status;
    uint8_t rsp_data;
    s_zb_znp_req_t znp_req;

    znp_req.type = ZNP_CMD_REQ_TYPE_ZNP;
    znp_req.start_tick = zb_os_now_ms();
    znp_req.wants_response = true;
    znp_req.timeout_ms = ZNP_MT_SRSP_DEFAULT_TIMEOUT_MS;
    znp_req.znp.cmd0 = (ZNP_MT_CMD_SREQ | ZNP_MT_SYS_ZDO);
    znp_req.znp.cmd1 = ZNP_ZDO_MGMT_NWK_DISC_REQ;
    znp_req.znp.payload = (uint8_t *)req;
    znp_req.znp.payload_len = sizeof(s_zb_znp_mt_zdo_mgmt_nwk_disc_req_t);
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
zb_znp_mt_zdo_process_mgmt_nwk_disc_rsp(const uint8_t *rpc_buff, uint8_t rpc_len)
{
    if (g_zb_znp_mt_zdo_cb.pfn_zdo_mgmt_nwk_disc_rsp_cb)
    {
        const s_zb_znp_mt_zdo_mgmt_nwk_disc_rsp_t *rsp = (const s_zb_znp_mt_zdo_mgmt_nwk_disc_rsp_t *)&rpc_buff[2];

        g_zb_znp_mt_zdo_cb.pfn_zdo_mgmt_nwk_disc_rsp_cb(rsp);
    }
}

int
zb_znp_mt_zdo_mgmt_lqi_req(const s_zb_znp_mt_zdo_mgmt_lqi_req_t *req)
{
    int status;
    uint8_t rsp_data;
    s_zb_znp_req_t znp_req;

    znp_req.type = ZNP_CMD_REQ_TYPE_ZNP;
    znp_req.start_tick = zb_os_now_ms();
    znp_req.wants_response = true;
    znp_req.timeout_ms = ZNP_MT_SRSP_DEFAULT_TIMEOUT_MS;
    znp_req.znp.cmd0 = (ZNP_MT_CMD_SREQ | ZNP_MT_SYS_ZDO);
    znp_req.znp.cmd1 = ZNP_ZDO_MGMT_LQI_REQ;
    znp_req.znp.payload = (uint8_t *)req;
    znp_req.znp.payload_len = sizeof(s_zb_znp_mt_zdo_mgmt_lqi_req_t);
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
zb_znp_mt_zdo_process_mgmt_lqi_rsp(const uint8_t *rpc_buff, uint8_t rpc_len)
{
    if (g_zb_znp_mt_zdo_cb.pfn_zdo_mgmt_lqi_rsp_cb)
    {
        const s_zb_znp_mt_zdo_mgmt_lqi_rsp_t *rsp = (const s_zb_znp_mt_zdo_mgmt_lqi_rsp_t *)&rpc_buff[2];

        g_zb_znp_mt_zdo_cb.pfn_zdo_mgmt_lqi_rsp_cb(rsp);
    }
}

int
zb_znp_mt_zdo_mgmt_rtg_req(const s_zb_znp_mt_zdo_mgmt_rtg_req_t *req)
{
    int status;
    uint8_t rsp_data;
    s_zb_znp_req_t znp_req;

    znp_req.type = ZNP_CMD_REQ_TYPE_ZNP;
    znp_req.start_tick = zb_os_now_ms();
    znp_req.wants_response = true;
    znp_req.timeout_ms = ZNP_MT_SRSP_DEFAULT_TIMEOUT_MS;
    znp_req.znp.cmd0 = (ZNP_MT_CMD_SREQ | ZNP_MT_SYS_ZDO);
    znp_req.znp.cmd1 = ZNP_ZDO_MGMT_RTG_REQ;
    znp_req.znp.payload = (uint8_t *)req;
    znp_req.znp.payload_len = sizeof(s_zb_znp_mt_zdo_mgmt_rtg_req_t);
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
zb_znp_mt_zdo_process_mgmt_rtg_rsp(const uint8_t *rpc_buff, uint8_t rpc_len)
{
    if (g_zb_znp_mt_zdo_cb.pfn_zdo_mgmt_rtg_rsp_cb)
    {
        const s_zb_znp_mt_zdo_mgmt_rtg_rsp_t *rsp = (const s_zb_znp_mt_zdo_mgmt_rtg_rsp_t *)&rpc_buff[2];

        g_zb_znp_mt_zdo_cb.pfn_zdo_mgmt_rtg_rsp_cb(rsp);
    }
}

int
zb_znp_mt_zdo_mgmt_bind_req(const s_zb_znp_mt_zdo_mgmt_bind_req_t *req)
{
    int status;
    uint8_t rsp_data;
    s_zb_znp_req_t znp_req;

    znp_req.type = ZNP_CMD_REQ_TYPE_ZNP;
    znp_req.start_tick = zb_os_now_ms();
    znp_req.wants_response = true;
    znp_req.timeout_ms = ZNP_MT_SRSP_DEFAULT_TIMEOUT_MS;
    znp_req.znp.cmd0 = (ZNP_MT_CMD_SREQ | ZNP_MT_SYS_ZDO);
    znp_req.znp.cmd1 = ZNP_ZDO_MGMT_BIND_REQ;
    znp_req.znp.payload = (uint8_t *)req;
    znp_req.znp.payload_len = sizeof(s_zb_znp_mt_zdo_mgmt_bind_req_t);
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
zb_znp_mt_zdo_process_mgmt_bind_rsp(const uint8_t *rpc_buff, uint8_t rpc_len)
{
    if (g_zb_znp_mt_zdo_cb.pfn_zdo_mgmt_bind_rsp_cb)
    {
        const s_zb_znp_mt_zdo_mgmt_bind_rsp_t *rsp = (const s_zb_znp_mt_zdo_mgmt_bind_rsp_t *)&rpc_buff[2];

        g_zb_znp_mt_zdo_cb.pfn_zdo_mgmt_bind_rsp_cb(rsp);
    }
}

int
zb_znp_mt_zdo_mgmt_leave_req(const s_zb_znp_mt_zdo_mgmt_leave_req_t *req)
{
    int status;
    uint8_t rsp_data;
    s_zb_znp_req_t znp_req;

    znp_req.type = ZNP_CMD_REQ_TYPE_ZNP;
    znp_req.start_tick = zb_os_now_ms();
    znp_req.wants_response = true;
    znp_req.timeout_ms = ZNP_MT_SRSP_DEFAULT_TIMEOUT_MS;
    znp_req.znp.cmd0 = (ZNP_MT_CMD_SREQ | ZNP_MT_SYS_ZDO);
    znp_req.znp.cmd1 = ZNP_ZDO_MGMT_LEAVE_REQ;
    znp_req.znp.payload = (uint8_t *)req;
    znp_req.znp.payload_len = sizeof(s_zb_znp_mt_zdo_mgmt_leave_req_t);
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
zb_znp_mt_zdo_process_mgmt_leave_rsp(const uint8_t *rpc_buff, uint8_t rpc_len)
{
    if (g_zb_znp_mt_zdo_cb.pfn_zdo_mgmt_leave_rsp_cb)
    {
        const s_zb_znp_mt_zdo_mgmt_leave_rsp_t *rsp = (const s_zb_znp_mt_zdo_mgmt_leave_rsp_t *)&rpc_buff[2];

        g_zb_znp_mt_zdo_cb.pfn_zdo_mgmt_leave_rsp_cb(rsp);
    }
}

int
zb_znp_mt_zdo_mgmt_direct_join_req(const s_zb_znp_mt_zdo_mgmt_direct_join_req_t *req)
{
    int status;
    uint8_t rsp_data;
    s_zb_znp_req_t znp_req;

    znp_req.type = ZNP_CMD_REQ_TYPE_ZNP;
    znp_req.start_tick = zb_os_now_ms();
    znp_req.wants_response = true;
    znp_req.timeout_ms = ZNP_MT_SRSP_DEFAULT_TIMEOUT_MS;
    znp_req.znp.cmd0 = (ZNP_MT_CMD_SREQ | ZNP_MT_SYS_ZDO);
    znp_req.znp.cmd1 = ZNP_ZDO_MGMT_DIRECT_JOIN_REQ;
    znp_req.znp.payload = (uint8_t *)req;
    znp_req.znp.payload_len = sizeof(s_zb_znp_mt_zdo_mgmt_direct_join_req_t);
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
zb_znp_mt_zdo_process_mgmt_direct_join_rsp(const uint8_t *rpc_buff, uint8_t rpc_len)
{
    if (g_zb_znp_mt_zdo_cb.pfn_zdo_mgmt_direct_join_rsp_cb)
    {
        const s_zb_znp_mt_zdo_mgmt_direct_join_rsp_t *rsp = (const s_zb_znp_mt_zdo_mgmt_direct_join_rsp_t *)&rpc_buff[2];

        g_zb_znp_mt_zdo_cb.pfn_zdo_mgmt_direct_join_rsp_cb(rsp);
    }
}

int
zb_znp_mt_zdo_mgmt_permit_join_req(const s_zb_znp_mt_zdo_mgmt_permit_join_req_t *req)
{
    int status;
    uint8_t rsp_data;
    s_zb_znp_req_t znp_req;

    znp_req.type = ZNP_CMD_REQ_TYPE_ZNP;
    znp_req.start_tick = zb_os_now_ms();
    znp_req.wants_response = true;
    znp_req.timeout_ms = ZNP_MT_SRSP_DEFAULT_TIMEOUT_MS;
    znp_req.znp.cmd0 = (ZNP_MT_CMD_SREQ | ZNP_MT_SYS_ZDO);
    znp_req.znp.cmd1 = ZNP_ZDO_MGMT_PERMIT_JOIN_REQ;
    znp_req.znp.payload = (uint8_t *)req;
    znp_req.znp.payload_len = sizeof(s_zb_znp_mt_zdo_mgmt_permit_join_req_t);
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
zb_znp_mt_zdo_process_mgmt_permit_join_rsp(const uint8_t *rpc_buff, uint8_t rpc_len)
{
    if (g_zb_znp_mt_zdo_cb.pfn_zdo_mgmt_permit_join_rsp_cb)
    {
        const s_zb_znp_mt_zdo_mgmt_permit_join_rsp_t *rsp = (const s_zb_znp_mt_zdo_mgmt_permit_join_rsp_t *)&rpc_buff[2];

        g_zb_znp_mt_zdo_cb.pfn_zdo_mgmt_permit_join_rsp_cb(rsp);
    }
}

int
zb_znp_mt_zdo_mgmt_nwk_update_req(const s_zb_znp_mt_zdo_mgmt_nwk_update_req_t *req)
{
    int status;
    uint8_t rsp_data;
    s_zb_znp_req_t znp_req;

    znp_req.type = ZNP_CMD_REQ_TYPE_ZNP;
    znp_req.start_tick = zb_os_now_ms();
    znp_req.wants_response = true;
    znp_req.timeout_ms = ZNP_MT_SRSP_DEFAULT_TIMEOUT_MS;
    znp_req.znp.cmd0 = (ZNP_MT_CMD_SREQ | ZNP_MT_SYS_ZDO);
    znp_req.znp.cmd1 = ZNP_ZDO_MGMT_NWK_UPDATE_REQ;
    znp_req.znp.payload = (uint8_t *)req;
    znp_req.znp.payload_len = sizeof(s_zb_znp_mt_zdo_mgmt_nwk_update_req_t);
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
zb_znp_mt_zdo_msg_cb_register(const s_zb_znp_mt_zdo_msg_cb_register_t *req)
{
    int status;
    uint8_t rsp_data;
    s_zb_znp_req_t znp_req;

    znp_req.type = ZNP_CMD_REQ_TYPE_ZNP;
    znp_req.start_tick = zb_os_now_ms();
    znp_req.wants_response = true;
    znp_req.timeout_ms = ZNP_MT_SRSP_DEFAULT_TIMEOUT_MS;
    znp_req.znp.cmd0 = (ZNP_MT_CMD_SREQ | ZNP_MT_SYS_ZDO);
    znp_req.znp.cmd1 = ZNP_ZDO_MSG_CB_REGISTER;
    znp_req.znp.payload = (uint8_t *)req;
    znp_req.znp.payload_len = sizeof(s_zb_znp_mt_zdo_msg_cb_register_t);
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
zb_znp_mt_zdo_msg_cb_remove(const s_zb_znp_mt_zdo_msg_cb_remove_t *req)
{
    int status;
    uint8_t rsp_data;
    s_zb_znp_req_t znp_req;

    znp_req.type = ZNP_CMD_REQ_TYPE_ZNP;
    znp_req.start_tick = zb_os_now_ms();
    znp_req.wants_response = true;
    znp_req.timeout_ms = ZNP_MT_SRSP_DEFAULT_TIMEOUT_MS;
    znp_req.znp.cmd0 = (ZNP_MT_CMD_SREQ | ZNP_MT_SYS_ZDO);
    znp_req.znp.cmd1 = ZNP_ZDO_MSG_CB_REMOVE;
    znp_req.znp.payload = (uint8_t *)req;
    znp_req.znp.payload_len = sizeof(s_zb_znp_mt_zdo_msg_cb_remove_t);
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
zb_znp_mt_zdo_startup_from_app(const s_zb_znp_mt_zdo_startup_from_app_t *req)
{
    int status;
    uint8_t rsp_data;
    s_zb_znp_req_t znp_req;

    znp_req.type = ZNP_CMD_REQ_TYPE_ZNP;
    znp_req.start_tick = zb_os_now_ms();
    znp_req.wants_response = true;
    znp_req.timeout_ms = ZNP_MT_SRSP_DEFAULT_TIMEOUT_MS;
    znp_req.znp.cmd0 = (ZNP_MT_CMD_SREQ | ZNP_MT_SYS_ZDO);
    znp_req.znp.cmd1 = ZNP_ZDO_STARTUP_FROM_APP;
    znp_req.znp.payload = (uint8_t *)req;
    znp_req.znp.payload_len = sizeof(s_zb_znp_mt_zdo_startup_from_app_t);
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
zb_znp_mt_zdo_ext_nwk_info(s_zb_znp_mt_zdo_ext_nwk_info_srsp_t *rsp)
{
    int status;
    uint8_t rsp_data[sizeof(s_zb_znp_mt_zdo_ext_nwk_info_srsp_t)] = { 0 };
    s_zb_znp_req_t znp_req;

    znp_req.type = ZNP_CMD_REQ_TYPE_ZNP;
    znp_req.start_tick = zb_os_now_ms();
    znp_req.wants_response = true;
    znp_req.timeout_ms = ZNP_MT_SRSP_DEFAULT_TIMEOUT_MS;
    znp_req.znp.cmd0 = (ZNP_MT_CMD_SREQ | ZNP_MT_SYS_ZDO);
    znp_req.znp.cmd1 = ZNP_ZDO_EXT_NWK_INFO;
    znp_req.znp.payload = NULL;
    znp_req.znp.payload_len = 0;
    znp_req.znp.resp_data = rsp_data;
    znp_req.znp.resp_len = sizeof(s_zb_znp_mt_zdo_ext_nwk_info_srsp_t);
    status = zb_znp_send_cmd_req(&znp_req);

    if (status == ZB_OK)
    {
        if (rsp)
        {
            memcpy(rsp, rsp_data, sizeof(s_zb_znp_mt_zdo_ext_nwk_info_srsp_t));
        }
    }

    return status;
}

int
zb_znp_mt_zdo_set_link_key(const s_zb_znp_mt_zdo_set_link_key_t *req)
{
    int status;
    uint8_t rsp_data;
    s_zb_znp_req_t znp_req;

    znp_req.type = ZNP_CMD_REQ_TYPE_ZNP;
    znp_req.start_tick = zb_os_now_ms();
    znp_req.wants_response = true;
    znp_req.timeout_ms = ZNP_MT_SRSP_DEFAULT_TIMEOUT_MS;
    znp_req.znp.cmd0 = (ZNP_MT_CMD_SREQ | ZNP_MT_SYS_ZDO);
    znp_req.znp.cmd1 = ZNP_ZDO_SET_LINK_KEY;
    znp_req.znp.payload = (uint8_t *)req;
    znp_req.znp.payload_len = sizeof(s_zb_znp_mt_zdo_set_link_key_t);
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
zb_znp_mt_zdo_remove_link_key(const s_zb_znp_mt_zdo_remove_link_key_t *req)
{
    int status;
    uint8_t rsp_data;
    s_zb_znp_req_t znp_req;

    znp_req.type = ZNP_CMD_REQ_TYPE_ZNP;
    znp_req.start_tick = zb_os_now_ms();
    znp_req.wants_response = true;
    znp_req.timeout_ms = ZNP_MT_SRSP_DEFAULT_TIMEOUT_MS;
    znp_req.znp.cmd0 = (ZNP_MT_CMD_SREQ | ZNP_MT_SYS_ZDO);
    znp_req.znp.cmd1 = ZNP_ZDO_REMOVE_LINK_KEY;
    znp_req.znp.payload = (uint8_t *)req;
    znp_req.znp.payload_len = sizeof(s_zb_znp_mt_zdo_remove_link_key_t);
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
zb_znp_mt_zdo_get_link_key(const s_zb_znp_mt_zdo_get_link_key_t *req, s_zb_znp_mt_zdo_get_link_key_srsp_t *rsp)
{
    int status;
    uint8_t rsp_data[sizeof(s_zb_znp_mt_zdo_get_link_key_srsp_t) + 1];
    s_zb_znp_req_t znp_req;

    znp_req.type = ZNP_CMD_REQ_TYPE_ZNP;
    znp_req.start_tick = zb_os_now_ms();
    znp_req.wants_response = true;
    znp_req.timeout_ms = ZNP_MT_SRSP_DEFAULT_TIMEOUT_MS;
    znp_req.znp.cmd0 = (ZNP_MT_CMD_SREQ | ZNP_MT_SYS_ZDO);
    znp_req.znp.cmd1 = ZNP_ZDO_GET_LINK_KEY;
    znp_req.znp.payload = (uint8_t *)req;
    znp_req.znp.payload_len = sizeof(s_zb_znp_mt_zdo_get_link_key_t);
    znp_req.znp.resp_data = rsp_data;
    znp_req.znp.resp_len = sizeof(s_zb_znp_mt_zdo_get_link_key_srsp_t) + 1;
    status = zb_znp_send_cmd_req(&znp_req);

    if (status == ZB_OK)
    {
        status = rsp_data[0];
        memcpy(rsp, rsp_data + 1, sizeof(s_zb_znp_mt_zdo_get_link_key_srsp_t));
    }

    return status;
}

int
zb_znp_mt_zdo_nwk_discovery_req(const s_zb_znp_mt_zdo_nwk_discovery_req_t *req)
{
    int status;
    uint8_t rsp_data;
    s_zb_znp_req_t znp_req;

    znp_req.type = ZNP_CMD_REQ_TYPE_ZNP;
    znp_req.start_tick = zb_os_now_ms();
    znp_req.wants_response = true;
    znp_req.timeout_ms = ZNP_MT_SRSP_DEFAULT_TIMEOUT_MS;
    znp_req.znp.cmd0 = (ZNP_MT_CMD_SREQ | ZNP_MT_SYS_ZDO);
    znp_req.znp.cmd1 = ZNP_ZDO_NWK_DISCOVERY_REQ;
    znp_req.znp.payload = (uint8_t *)req;
    znp_req.znp.payload_len = sizeof(s_zb_znp_mt_zdo_nwk_discovery_req_t);
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
zb_znp_mt_zdo_join_req(const s_zb_znp_mt_zdo_join_req_t *req)
{
    int status;
    uint8_t rsp_data;
    s_zb_znp_req_t znp_req;

    znp_req.type = ZNP_CMD_REQ_TYPE_ZNP;
    znp_req.start_tick = zb_os_now_ms();
    znp_req.wants_response = true;
    znp_req.timeout_ms = ZNP_MT_SRSP_DEFAULT_TIMEOUT_MS;
    znp_req.znp.cmd0 = (ZNP_MT_CMD_SREQ | ZNP_MT_SYS_ZDO);
    znp_req.znp.cmd1 = ZNP_ZDO_JOIN_REQ;
    znp_req.znp.payload = (uint8_t *)req;
    znp_req.znp.payload_len = sizeof(s_zb_znp_mt_zdo_join_req_t);
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
zb_znp_mt_zdo_process_state_change_ind(const uint8_t *rpc_buff, uint8_t rpc_len)
{
    e_zb_znp_mt_zdo_state_t dev_state = rpc_buff[2];
    if (g_zb_znp_mt_zdo_cb.pfn_zdo_state_change_ind_cb)
    {
        g_zb_znp_mt_zdo_cb.pfn_zdo_state_change_ind_cb(dev_state);
    }
}

static void
zb_znp_mt_zdo_process_end_device_annce_ind(const uint8_t *rpc_buff, uint8_t rpc_len)
{
    if (g_zb_znp_mt_zdo_cb.pfn_zdo_end_device_annce_ind_cb)
    {
        const s_zb_znp_mt_zdo_end_device_annce_ind_t *ind = (const s_zb_znp_mt_zdo_end_device_annce_ind_t *)&rpc_buff[2];

        g_zb_znp_mt_zdo_cb.pfn_zdo_end_device_annce_ind_cb(ind);
    }
}

static void
zb_znp_mt_zdo_process_match_desc_rsp_sent(const uint8_t *rpc_buff, uint8_t rpc_len)
{
    if (g_zb_znp_mt_zdo_cb.pfn_zdo_match_desc_rsp_sent_cb)
    {
        const s_zb_znp_mt_zdo_match_desc_rsp_sent_t *ind = (const s_zb_znp_mt_zdo_match_desc_rsp_sent_t *)&rpc_buff[2];

        g_zb_znp_mt_zdo_cb.pfn_zdo_match_desc_rsp_sent_cb(ind);
    }
}

static void
zb_znp_mt_zdo_process_status_error_rsp(const uint8_t *rpc_buff, uint8_t rpc_len)
{
    if (g_zb_znp_mt_zdo_cb.pfn_zdo_status_error_rsp_cb)
    {
        const s_zb_znp_mt_zdo_status_error_rsp_t *rsp = (const s_zb_znp_mt_zdo_status_error_rsp_t *)&rpc_buff[2];

        g_zb_znp_mt_zdo_cb.pfn_zdo_status_error_rsp_cb(rsp);
    }
}

static void
zb_znp_mt_zdo_process_src_rtg_ind(const uint8_t *rpc_buff, uint8_t rpc_len)
{
    if (g_zb_znp_mt_zdo_cb.pfn_zdo_src_rtg_ind_cb)
    {
        const s_zb_znp_mt_zdo_src_rtg_ind_t *ind = (const s_zb_znp_mt_zdo_src_rtg_ind_t *)&rpc_buff[2];

        g_zb_znp_mt_zdo_cb.pfn_zdo_src_rtg_ind_cb(ind);
    }
}

static void
zb_znp_mt_zdo_process_beacon_notify_ind(const uint8_t *rpc_buff, uint8_t rpc_len)
{
    if (g_zb_znp_mt_zdo_cb.pfn_zdo_beacon_notify_ind_cb)
    {
        const s_zb_znp_mt_zdo_beacon_notify_ind_t *ind = (const s_zb_znp_mt_zdo_beacon_notify_ind_t *)&rpc_buff[2];

        g_zb_znp_mt_zdo_cb.pfn_zdo_beacon_notify_ind_cb(ind);
    }
}

static void
zb_znp_mt_zdo_process_join_cnf(const uint8_t *rpc_buff, uint8_t rpc_len)
{
    if (g_zb_znp_mt_zdo_cb.pfn_zdo_join_cnf_cb)
    {
        const s_zb_znp_mt_zdo_join_cnf_t *cnf = (const s_zb_znp_mt_zdo_join_cnf_t *)&rpc_buff[2];

        g_zb_znp_mt_zdo_cb.pfn_zdo_join_cnf_cb(cnf);
    }
}

static void
zb_znp_mt_zdo_process_nwk_discovery_cnf(const uint8_t *rpc_buff, uint8_t rpc_len)
{
    if (g_zb_znp_mt_zdo_cb.pfn_zdo_nwk_discovery_cnf_cb)
    {
        const s_zb_znp_mt_zdo_nwk_discovery_cnf_t *cnf = (const s_zb_znp_mt_zdo_nwk_discovery_cnf_t *)&rpc_buff[2];

        g_zb_znp_mt_zdo_cb.pfn_zdo_nwk_discovery_cnf_cb(cnf);
    }
}

static void
zb_znp_mt_zdo_process_leave_ind(const uint8_t *rpc_buff, uint8_t rpc_len)
{
    if (g_zb_znp_mt_zdo_cb.pfn_zdo_leave_ind_cb)
    {
        const s_zb_znp_mt_zdo_leave_ind_t *ind = (const s_zb_znp_mt_zdo_leave_ind_t *)&rpc_buff[2];

        g_zb_znp_mt_zdo_cb.pfn_zdo_leave_ind_cb(ind);
    }    
}

static void
zb_znp_mt_zdo_process_msg_cb_incoming(const uint8_t *rpc_buff, uint8_t rpc_len)
{
    if (g_zb_znp_mt_zdo_cb.pfn_zdo_msg_cb_incoming_cb)
    {
        const s_zb_znp_mt_zdo_msg_cb_incoming_t *ind = (const s_zb_znp_mt_zdo_msg_cb_incoming_t *)&rpc_buff[2];

        g_zb_znp_mt_zdo_cb.pfn_zdo_msg_cb_incoming_cb(ind);
    }
}

static void
zb_znp_mt_zdo_process_tc_device_ind(const uint8_t *rpc_buff, uint8_t rpc_len)
{
    if (g_zb_znp_mt_zdo_cb.pfn_zdo_tc_device_ind_cb)
    {
        const s_zb_znp_mt_zdo_tc_device_ind_t *ind = (const s_zb_znp_mt_zdo_tc_device_ind_t *)&rpc_buff[2];

        g_zb_znp_mt_zdo_cb.pfn_zdo_tc_device_ind_cb(ind);
    }
}
