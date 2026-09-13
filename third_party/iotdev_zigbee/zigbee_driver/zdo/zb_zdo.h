/*
 * zb_zdo.h
 * 
 * Author: Vo Van Buong (BRT-SG)
 */

#ifndef ZB_ZDO_H_
#define ZB_ZDO_H_

#ifdef __cplusplus
extern "C"
{
#endif /* __cplusplus */

#include "af/zb_af.h"
#include "common/zb_common.h"

int zb_zdo_permit_join(uint8_t duration);
int zb_zdo_get_coordinator_info(s_zb_coordinator_info_t *info); /* prefer zb_core_get_coordinator_info() */
int zb_zdo_bind_request(
    uint16_t dst_addr, uint64_t src_ieee_addr, uint8_t src_endpoint,
    s_zb_af_address_t *dst_bind_addr, uint16_t cluster_id);
int zb_zdo_unbind_request(
    uint16_t dst_addr, uint64_t src_ieee_addr, uint8_t src_endpoint,
    s_zb_af_address_t *dst_unbind_addr, uint16_t cluster_id);

zb_status_t zb_zdo_send_nwk_addr_req(uint64_t ieee_addr);
zb_status_t zb_zdo_send_ieee_addr_req(uint16_t nwk_addr);
zb_status_t zb_zdo_send_node_desc_req(uint16_t nwk_addr);
zb_status_t zb_zdo_send_active_endpoint_req(uint16_t nwk_addr);
zb_status_t zb_zdo_send_simple_desc_req(uint16_t nwk_addr, uint8_t endpoint);
zb_status_t zb_zdo_send_mgmt_leave_req(uint16_t nwk_addr, uint64_t ieee_addr, bool remove_children, bool rejoin);

/**
 * @brief Ask a single node for its neighbour (LQI) table (ZDO Mgmt_Lqi_req).
 *
 * The node whose neighbour table is requested is @p dst_addr. It replies
 * asynchronously with a Mgmt_Lqi_rsp that lists, for each neighbour, the
 * relationship (parent / child / sibling) and the measured LQI of that radio
 * link. The response is delivered to the registered
 * pfn_zdo_mgmt_lqi_rsp_cb (zb_core logs it).
 *
 * The neighbour table can be larger than one response; @p start_index selects
 * the first entry to return. Call again with a higher index to page through
 * the rest (the response carries the total entry count).
 *
 * @param dst_addr     16-bit network address of the node to query.
 * @param start_index  First neighbour-table index to return (0 for the start).
 * @return ZB_OK if the request was accepted by the ZNP, ZB_FAIL otherwise.
 */
zb_status_t zb_zdo_send_mgmt_lqi_req(uint16_t dst_addr, uint8_t start_index);

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* ZB_ZDO_H_ */