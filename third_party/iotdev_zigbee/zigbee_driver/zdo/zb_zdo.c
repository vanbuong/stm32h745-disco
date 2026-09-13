/*
 * zb_zdo.c
 * 
 * Author: Vo Van Buong (BRT-SG)
 */

#include "af/zb_af.h"
#include "znp/zb_znp.h"
#include "znp/zb_znp_mt_zdo.h"

#define TAG "ZB_ZDO"

int
zb_zdo_permit_join(uint8_t duration)
{
    s_zb_znp_mt_zdo_mgmt_permit_join_req_t permit_join_req;
    permit_join_req.dst_addr = 0xFFFC;
    permit_join_req.addr_mode = AF_ADDRESS_16BIT;
    permit_join_req.duration = duration;
    permit_join_req.tc_significance = 1;
    if (zb_znp_mt_zdo_mgmt_permit_join_req(&permit_join_req) == ZB_OK)
    {
        ZB_LOGI(TAG, "Permit join request sent");
        return ZB_OK;
    }
    else
    {
        ZB_LOGE(TAG, "Permit join request failed");
        return ZB_FAIL;
    }
}

int
zb_zdo_get_coordinator_info(s_zb_coordinator_info_t *info)
{
    s_zb_znp_mt_zdo_ext_nwk_info_srsp_t ext_nwk_info = {};
    int ret = zb_znp_mt_zdo_ext_nwk_info(&ext_nwk_info);
    if (ret == ZB_OK)
    {
        info->ieee_addr = ext_nwk_info.ext_pan_id;
        info->pan_id = ext_nwk_info.pan_id;
        info->channel = ext_nwk_info.channel;
        info->state = ext_nwk_info.device_state;
    }
    return ret;
}

int
zb_zdo_bind_request(
    uint16_t dst_addr, uint64_t src_ieee_addr, uint8_t src_endpoint,
    s_zb_af_address_t *dst_bind_addr, uint16_t cluster_id)
{
    if (dst_bind_addr->address_mode == AF_ADDRESS_GROUP)
    {
        s_zb_znp_mt_zdo_bind_unbind_group_req_t bind_req;
        bind_req.dst_addr = dst_addr;
        bind_req.src_address = src_ieee_addr;
        bind_req.src_endpoint = src_endpoint;
        bind_req.dst_addr_mode = dst_bind_addr->address_mode;
        bind_req.dst_address = dst_bind_addr->short_addr;
        bind_req.cluster_id = cluster_id;
        return zb_znp_mt_zdo_bind_group_req(&bind_req);
    }

    if (dst_bind_addr->address_mode == AF_ADDRESS_64BIT)
    {
        s_zb_znp_mt_zdo_bind_unbind_device_req_t bind_req;
        bind_req.dst_addr = dst_addr;
        bind_req.src_address = src_ieee_addr;
        bind_req.src_endpoint = src_endpoint;
        bind_req.dst_addr_mode = dst_bind_addr->address_mode;
        bind_req.dst_address = dst_bind_addr->long_addr;
        bind_req.dst_endpoint = dst_bind_addr->endpoint;
        bind_req.cluster_id = cluster_id;
        return zb_znp_mt_zdo_bind_device_req(&bind_req);
    }

    return ZB_INVALID_PARAMETER;
}

int
zb_zdo_unbind_request(
    uint16_t dst_addr, uint64_t src_ieee_addr, uint8_t src_endpoint,
    s_zb_af_address_t *dst_unbind_addr, uint16_t cluster_id)
{
    if (dst_unbind_addr->address_mode == AF_ADDRESS_GROUP)
    {
        s_zb_znp_mt_zdo_bind_unbind_group_req_t unbind_req;
        unbind_req.dst_addr = dst_addr;
        unbind_req.src_address = src_ieee_addr;
        unbind_req.src_endpoint = src_endpoint;
        unbind_req.dst_addr_mode = dst_unbind_addr->address_mode;
        unbind_req.dst_address = dst_unbind_addr->short_addr;
        unbind_req.cluster_id = cluster_id;
        return zb_znp_mt_zdo_unbind_group_req(&unbind_req);
    }

    if (dst_unbind_addr->address_mode == AF_ADDRESS_64BIT)
    {
        s_zb_znp_mt_zdo_bind_unbind_device_req_t unbind_req;
        unbind_req.dst_addr = dst_addr;
        unbind_req.src_address = src_ieee_addr;
        unbind_req.src_endpoint = src_endpoint;
        unbind_req.dst_addr_mode = dst_unbind_addr->address_mode;
        unbind_req.dst_address = dst_unbind_addr->long_addr;
        unbind_req.dst_endpoint = dst_unbind_addr->endpoint;
        unbind_req.cluster_id = cluster_id;
        return zb_znp_mt_zdo_unbind_device_req(&unbind_req);
    }

    return ZB_INVALID_PARAMETER;
}

zb_status_t zb_zdo_send_nwk_addr_req(uint64_t ieee_addr)
{
    s_zb_znp_mt_zdo_nwk_addr_req_t req_param = {};
    req_param.ieee_addr = ieee_addr;
    return zb_znp_mt_zdo_nwk_addr_req(&req_param);
}

zb_status_t zb_zdo_send_ieee_addr_req(uint16_t nwk_addr)
{
    s_zb_znp_mt_zdo_ieee_addr_req_t req_param = {};
    req_param.short_addr = nwk_addr;
    return zb_znp_mt_zdo_ieee_addr_req(&req_param);
}

zb_status_t zb_zdo_send_node_desc_req(uint16_t nwk_addr)
{
    s_zb_znp_mt_zdo_node_desc_req_t req_param = {};
    req_param.dst_addr = nwk_addr;
    req_param.nwk_addr_of_interest = nwk_addr;
    return zb_znp_mt_zdo_node_desc_req(&req_param);
}

zb_status_t zb_zdo_send_active_endpoint_req(uint16_t nwk_addr)
{
    s_zb_znp_mt_zdo_active_ep_req_t req_param = {};
    req_param.dst_addr = nwk_addr;
    req_param.nwk_addr_of_interest = nwk_addr;
    return zb_znp_mt_zdo_active_ep_req(&req_param);
}

zb_status_t zb_zdo_send_simple_desc_req(uint16_t nwk_addr, uint8_t endpoint)
{
    s_zb_znp_mt_zdo_simple_desc_req_t req_param = {};
    req_param.dst_addr = nwk_addr;
    req_param.nwk_addr_of_interest = nwk_addr;
    req_param.endpoint = endpoint;
    return zb_znp_mt_zdo_simple_desc_req(&req_param);
}

zb_status_t zb_zdo_send_mgmt_leave_req(uint16_t nwk_addr, uint64_t ieee_addr, bool remove_children, bool rejoin)
{
    s_zb_znp_mt_zdo_mgmt_leave_req_t req_param = {};
    req_param.dst_addr = nwk_addr;
    req_param.device_addr = ieee_addr;
    req_param.remove_children_flag = remove_children ? 1 : 0;
    req_param.rejoin_flag = rejoin ? 1 : 0;
    return zb_znp_mt_zdo_mgmt_leave_req(&req_param);
}

zb_status_t zb_zdo_send_mgmt_lqi_req(uint16_t dst_addr, uint8_t start_index)
{
    s_zb_znp_mt_zdo_mgmt_lqi_req_t req_param = {};
    req_param.dst_addr = dst_addr;
    req_param.start_index = start_index;
    return zb_znp_mt_zdo_mgmt_lqi_req(&req_param);
}
