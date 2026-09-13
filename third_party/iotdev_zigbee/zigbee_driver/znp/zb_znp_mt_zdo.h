/*
 * zb_znp_zdo.h
 *
 * This module contains the definitions and function prototypes for the ZNP MT ZDO Interface.
 * 
 * Created on: 22 May 2025
 *     Author: Vo Van Buong (BRT-SG)
 */

#ifndef ZB_ZNP_MT_ZDO_H_
#define ZB_ZNP_MT_ZDO_H_

#ifdef __cplusplus
extern "C"
{
#endif /* __cplusplus */

#include "common/zb_common.h"

/**************************************************************************************************
 * ZDO COMMANDS
 *************************************************************************************************/

/* SREQ/SRSP */
#define ZNP_ZDO_NWK_ADDR_REQ                  0x00
#define ZNP_ZDO_IEEE_ADDR_REQ                 0x01
#define ZNP_ZDO_NODE_DESC_REQ                 0x02
#define ZNP_ZDO_POWER_DESC_REQ                0x03
#define ZNP_ZDO_SIMPLE_DESC_REQ               0x04
#define ZNP_ZDO_ACTIVE_EP_REQ                 0x05
#define ZNP_ZDO_MATCH_DESC_REQ                0x06
#define ZNP_ZDO_COMPLEX_DESC_REQ              0x07
#define ZNP_ZDO_USER_DESC_REQ                 0x08
#define ZNP_ZDO_END_DEVICE_ANNCE              0x0A
#define ZNP_ZDO_USER_DESC_SET                 0x0B
#define ZNP_ZDO_SERVER_DISC_REQ               0x0C
#define ZNP_ZDO_END_DEVICE_BIND_REQ           0x20
#define ZNP_ZDO_BIND_REQ                      0x21
#define ZNP_ZDO_UNBIND_REQ                    0x22

#define ZNP_ZDO_SET_LINK_KEY                  0x23
#define ZNP_ZDO_REMOVE_LINK_KEY               0x24
#define ZNP_ZDO_GET_LINK_KEY                  0x25
#define ZNP_ZDO_NWK_DISCOVERY_REQ             0x26
#define ZNP_ZDO_JOIN_REQ                      0x27

#define ZNP_ZDO_MGMT_NWK_DISC_REQ             0x30
#define ZNP_ZDO_MGMT_LQI_REQ                  0x31
#define ZNP_ZDO_MGMT_RTG_REQ                  0x32
#define ZNP_ZDO_MGMT_BIND_REQ                 0x33
#define ZNP_ZDO_MGMT_LEAVE_REQ                0x34
#define ZNP_ZDO_MGMT_DIRECT_JOIN_REQ          0x35
#define ZNP_ZDO_MGMT_PERMIT_JOIN_REQ          0x36
#define ZNP_ZDO_MGMT_NWK_UPDATE_REQ           0x37

/* AREQ optional, but no AREQ response. */
#define ZNP_ZDO_MSG_CB_REGISTER               0x3E
#define ZNP_ZDO_MSG_CB_REMOVE                 0x3F
#define ZNP_ZDO_STARTUP_FROM_APP              0x40

#define ZNP_ZDO_EXT_NWK_INFO                  0x50

/* AREQ to host */
#define ZNP_ZDO_NWK_ADDR_RSP                  0x80
#define ZNP_ZDO_IEEE_ADDR_RSP                 0x81
#define ZNP_ZDO_NODE_DESC_RSP                 0x82
#define ZNP_ZDO_POWER_DESC_RSP                0x83
#define ZNP_ZDO_SIMPLE_DESC_RSP               0x84
#define ZNP_ZDO_ACTIVE_EP_RSP                 0x85
#define ZNP_ZDO_MATCH_DESC_RSP                0x86
#define ZNP_ZDO_COMPLEX_DESC_RSP              0x87
#define ZNP_ZDO_USER_DESC_RSP                 0x88
#define ZNP_ZDO_USER_DESC_CONF                0x89
#define ZNP_ZDO_SERVER_DISC_RSP               0x8A
#define ZNP_ZDO_END_DEVICE_BIND_RSP           0xA0
#define ZNP_ZDO_BIND_RSP                      0xA1
#define ZNP_ZDO_UNBIND_RSP                    0xA2
#define ZNP_ZDO_MGMT_NWK_DISC_RSP             0xB0
#define ZNP_ZDO_MGMT_LQI_RSP                  0xB1
#define ZNP_ZDO_MGMT_RTG_RSP                  0xB2
#define ZNP_ZDO_MGMT_BIND_RSP                 0xB3
#define ZNP_ZDO_MGMT_LEAVE_RSP                0xB4
#define ZNP_ZDO_MGMT_DIRECT_JOIN_RSP          0xB5
#define ZNP_ZDO_MGMT_PERMIT_JOIN_RSP          0xB6

#define ZNP_ZDO_STATE_CHANGE_IND              0xC0
#define ZNP_ZDO_END_DEVICE_ANNCE_IND          0xC1
#define ZNP_ZDO_MATCH_DESC_RSP_SENT           0xC2
#define ZNP_ZDO_STATUS_ERROR_RSP              0xC3
#define ZNP_ZDO_SRC_RTG_IND                   0xC4
#define ZNP_ZDO_BEACON_NOTIFY_IND             0xC5
#define ZNP_ZDO_JOIN_CNF                      0xC6
#define ZNP_ZDO_NWK_DISCOVERY_CNF             0xC7
#define ZNP_ZDO_LEAVE_IND                     0xC9
#define ZNP_ZDO_TC_DEVICE_IND                 0xCA

#define ZNP_ZDO_MSG_CB_INCOMING               0xFF

/* ZDO Status Responses Definitions for ZDO Startup from App*/
#define ZNP_RESTORED_NETWORK                  0x00
#define ZNP_NEW_NETWORK                       0x01
#define ZNP_LEAVEANDNOTSTARTED                0x02

/**************************************************************************************************
 * TYPDEFS
 *************************************************************************************************/

TYPEDEF_STRUCT_PACKED s_zb_znp_mt_zdo_network_list_item
{
    uint64_t pan_id;
    uint8_t logical_channel;
    uint8_t stack_profile : 4;
    uint8_t zigbee_version : 4;
    uint8_t beacon_order : 4;
    uint8_t superframe_order : 4;
    uint8_t permit_join;
} s_zb_znp_mt_zdo_network_list_item_t;

TYPEDEF_STRUCT_PACKED s_zb_znp_mt_zdo_neighbor_lqi_list_item
{
    uint64_t ext_pan_id;
    uint64_t ext_addr;
    uint16_t network_addr;
    uint8_t device_type : 2;
    uint8_t rx_on_when_idle : 2;
    uint8_t relationship : 2;
    uint8_t reserved : 2;
    uint8_t permit_joining;
    uint8_t depth;
    uint8_t lqi;
} s_zb_znp_mt_zdo_neighbor_lqi_list_item_t;

TYPEDEF_STRUCT_PACKED s_zb_znp_mt_zdo_routing_table_list_item
{
    uint16_t dst_addr;
    uint8_t status;
    uint16_t next_hop;
} s_zb_znp_mt_zdo_routing_table_list_item_t;

TYPEDEF_STRUCT_PACKED s_zb_znp_mt_zdo_binding_table_list_item
{
    uint64_t src_ieee_addr;
    uint8_t src_endpoint;
    uint16_t cluster_id;
    uint8_t dst_addr_mode;
    uint64_t dst_ieee_addr;
    uint8_t dst_endpoint;
} s_zb_znp_mt_zdo_binding_table_list_item_t;

TYPEDEF_STRUCT_PACKED s_zb_znp_mt_zdo_beacon_list_item
{
    uint16_t src_addr;
    uint16_t pan_id;
    uint8_t logical_channel;
    uint8_t permit_joining;
    uint8_t router_cap;
    uint8_t device_cap;
    uint8_t protocol_ver;
    uint8_t stack_profile;
    uint8_t lqi;
    uint8_t depth;
    uint8_t update_id;
    uint64_t ext_pan_id;
} s_zb_znp_mt_zdo_beacon_list_item_t;

TYPEDEF_STRUCT_PACKED s_zb_znp_mt_zdo_nwk_addr_req
{
    uint64_t ieee_addr;
    uint8_t req_type;
    uint8_t start_index;
} s_zb_znp_mt_zdo_nwk_addr_req_t;

TYPEDEF_STRUCT_PACKED s_zb_znp_mt_zdo_nwk_addr_rsp
{
    uint8_t status;
    uint64_t ieee_addr;
    uint16_t nwk_addr;
    uint8_t start_index;
    uint8_t num_assoc_dev;
    uint16_t assoc_dev_list[];
} s_zb_znp_mt_zdo_nwk_addr_rsp_t;

TYPEDEF_STRUCT_PACKED s_zb_znp_mt_zdo_ieee_addr_req
{
    uint16_t short_addr;
    uint8_t req_type;
    uint8_t start_index;
} s_zb_znp_mt_zdo_ieee_addr_req_t;

TYPEDEF_STRUCT_PACKED s_zb_znp_mt_zdo_ieee_addr_rsp
{
    uint8_t status;
    uint64_t ieee_addr;
    uint16_t nwk_addr;
    uint8_t start_index;
    uint8_t num_assoc_dev;
    uint16_t assoc_dev_list[];
} s_zb_znp_mt_zdo_ieee_addr_rsp_t;

TYPEDEF_STRUCT_PACKED s_zb_znp_mt_zdo_node_desc_req
{
    uint16_t dst_addr;
    uint16_t nwk_addr_of_interest;
} s_zb_znp_mt_zdo_node_desc_req_t;

TYPEDEF_STRUCT_PACKED s_zb_znp_mt_zdo_node_desc_rsp
{
    uint16_t src_addr;
    uint8_t status;
    uint16_t nwk_addr;
    uint8_t logical_type : 3;
    uint8_t complex_desc_avail : 1;
    uint8_t user_desc_avail : 1;
    uint8_t reserved : 3;
    uint8_t aps_flag : 3;
    uint8_t freq_band : 5;
    uint8_t mac_cap_flg;
    uint16_t manufacturer_code;
    uint8_t max_buffer_size;
    uint16_t max_transfer_size;
    uint16_t server_mask;
    uint16_t max_out_transfer_size;
    uint8_t descriptor_capabilities;
} s_zb_znp_mt_zdo_node_desc_rsp_t;

TYPEDEF_STRUCT_PACKED s_zb_znp_mt_zdo_power_desc_req
{
    uint16_t dst_addr;
    uint16_t nwk_addr_of_interest;
} s_zb_znp_mt_zdo_power_desc_req_t;

TYPEDEF_STRUCT_PACKED s_zb_znp_mt_zdo_power_desc_rsp
{
    uint16_t src_addr;
    uint8_t status;
    uint16_t nwk_addr;
    uint8_t current_power_mode : 4;
    uint8_t aval_power_sources : 4;
    uint8_t current_power_source : 4;
    uint8_t current_power_source_level : 4;
} s_zb_znp_mt_zdo_power_desc_rsp_t;

TYPEDEF_STRUCT_PACKED s_zb_znp_mt_zdo_simple_desc_req
{
    uint16_t dst_addr;
    uint16_t nwk_addr_of_interest;
    uint8_t endpoint;
} s_zb_znp_mt_zdo_simple_desc_req_t;

TYPEDEF_STRUCT_PACKED s_zb_znp_mt_zdo_simple_desc_rsp
{
    uint16_t src_addr;
    uint8_t status;
    uint16_t nwk_addr;
    uint8_t len;
    uint8_t endpoint;
    uint16_t profile_id;
    uint16_t device_id;
    uint8_t device_version;
    uint8_t num_in_clusters;
    uint16_t in_cluster_list[16];
    uint8_t num_out_clusters;
    uint16_t out_cluster_list[16];
} s_zb_znp_mt_zdo_simple_desc_rsp_t;

TYPEDEF_STRUCT_PACKED s_zb_znp_mt_zdo_active_ep_req
{
    uint16_t dst_addr;
    uint16_t nwk_addr_of_interest;
} s_zb_znp_mt_zdo_active_ep_req_t;

TYPEDEF_STRUCT_PACKED s_zb_znp_mt_zdo_active_ep_rsp
{
    uint16_t src_addr;
    uint8_t status;
    uint16_t nwk_addr;
    uint8_t active_ep_count;
    uint8_t active_ep_list[77];
} s_zb_znp_mt_zdo_active_ep_rsp_t;

TYPEDEF_STRUCT_PACKED s_zb_znp_mt_zdo_match_desc_req
{
    uint16_t dst_addr;
    uint16_t nwk_addr_of_interest;
    uint16_t profile_id;
    uint8_t num_in_clusters;
    uint16_t in_cluster_list[16];
    uint8_t num_out_clusters;
    uint16_t out_cluster_list[16];
} s_zb_znp_mt_zdo_match_desc_req_t;

TYPEDEF_STRUCT_PACKED s_zb_znp_mt_zdo_match_desc_rsp
{
    uint16_t src_addr;
    uint8_t status;
    uint16_t nwk_addr;
    uint8_t match_length;
    uint8_t match_list[77];
} s_zb_znp_mt_zdo_match_desc_rsp_t;

TYPEDEF_STRUCT_PACKED s_zb_znp_mt_zdo_complex_desc_req
{
    uint16_t dst_addr;
    uint16_t nwk_addr_of_interest;
} s_zb_znp_mt_zdo_complex_desc_req_t;

TYPEDEF_STRUCT_PACKED s_zb_znp_mt_zdo_complex_desc_rsp
{
    uint16_t src_addr;
    uint8_t status;
    uint16_t nwk_addr;
    uint8_t complex_length;
    uint8_t complex_list[77];
} s_zb_znp_mt_zdo_complex_desc_rsp_t;

TYPEDEF_STRUCT_PACKED s_zb_znp_mt_zdo_user_desc_req
{
    uint16_t dst_addr;
    uint16_t nwk_addr_of_interest;
} s_zb_znp_mt_zdo_user_desc_req_t;

TYPEDEF_STRUCT_PACKED s_zb_znp_mt_zdo_user_desc_rsp
{
    uint16_t src_addr;
    uint8_t status;
    uint16_t nwk_addr;
    uint8_t len;
    uint8_t user_descriptor[77];
} s_zb_znp_mt_zdo_user_desc_rsp_t;

TYPEDEF_STRUCT_PACKED s_zb_znp_mt_zdo_end_device_annce
{
    uint16_t nwk_addr;
    uint64_t ieee_addr;
    uint8_t capabilities;
} s_zb_znp_mt_zdo_end_device_annce_t;

TYPEDEF_STRUCT_PACKED s_zb_znp_mt_zdo_user_desc_set
{
    uint16_t dst_addr;
    uint16_t nwk_addr_of_interest;
    uint8_t len;
    uint8_t user_descriptor[16];
} s_zb_znp_mt_zdo_user_desc_set_t;

TYPEDEF_STRUCT_PACKED s_zb_znp_mt_zdo_user_desc_set_rsp
{
    uint16_t src_addr;
    uint8_t status;
    uint16_t nwk_addr;
    uint8_t len;
    uint8_t user_descriptor[16];
} s_zb_znp_mt_zdo_user_desc_set_rsp_t;

TYPEDEF_STRUCT_PACKED s_zb_znp_mt_zdo_user_desc_conf
{
    uint16_t src_addr;
    uint8_t status;
    uint16_t nwk_addr;
} s_zb_znp_mt_zdo_user_desc_conf_t;

TYPEDEF_STRUCT_PACKED s_zb_znp_mt_zdo_server_disc_req
{
    uint16_t server_mask;
} s_zb_znp_mt_zdo_server_disc_req_t;

TYPEDEF_STRUCT_PACKED s_zb_znp_mt_zdo_server_disc_rsp
{
    uint16_t src_addr;
    uint8_t status;
    uint16_t server_mask;
} s_zb_znp_mt_zdo_server_disc_rsp_t;

TYPEDEF_STRUCT_PACKED s_zb_znp_mt_zdo_end_device_bind_req
{
    uint16_t dst_addr;
    uint16_t local_coordinator;
    uint64_t coordinator_ieee;
    uint8_t end_point;
    uint16_t profile_id;
    uint8_t num_in_clusters;
    uint16_t in_cluster_list[16];
    uint8_t num_out_clusters;
    uint16_t out_cluster_list[16];
} s_zb_znp_mt_zdo_end_device_bind_req_t;

TYPEDEF_STRUCT_PACKED s_zb_znp_mt_zdo_end_device_bind_rsp
{
    uint16_t src_addr;
    uint8_t status;
} s_zb_znp_mt_zdo_end_device_bind_rsp_t;

TYPEDEF_STRUCT_PACKED s_zb_znp_mt_zdo_bind_unbind_device_req
{
    uint16_t dst_addr;
    uint64_t src_address;
    uint8_t src_endpoint;
    uint16_t cluster_id;
    uint8_t dst_addr_mode;
    uint64_t dst_address;
    uint8_t dst_endpoint;
} s_zb_znp_mt_zdo_bind_unbind_device_req_t;

TYPEDEF_STRUCT_PACKED s_zb_znp_mt_zdo_bind_unbind_group_req
{
    uint16_t dst_addr;
    uint64_t src_address;
    uint8_t src_endpoint;
    uint16_t cluster_id;
    uint8_t dst_addr_mode;
    uint16_t dst_address;
} s_zb_znp_mt_zdo_bind_unbind_group_req_t;

TYPEDEF_STRUCT_PACKED s_zb_znp_mt_zdo_bind_unbind_rsp
{
    uint16_t src_addr;
    uint8_t status;
} s_zb_znp_mt_zdo_bind_unbind_rsp_t;

TYPEDEF_STRUCT_PACKED s_zb_znp_mt_zdo_mgmt_nwk_disc_req
{
    uint16_t dst_addr;
    uint32_t scan_channels;
    uint8_t scan_duration;
    uint8_t start_index;
} s_zb_znp_mt_zdo_mgmt_nwk_disc_req_t;

TYPEDEF_STRUCT_PACKED s_zb_znp_mt_zdo_mgmt_nwk_disc_rsp
{
    uint16_t src_addr;
    uint8_t status;
    uint8_t nwk_count;
    uint8_t start_index;
    uint8_t nwk_list_count;
    s_zb_znp_mt_zdo_network_list_item_t nwk_list[72];
} s_zb_znp_mt_zdo_mgmt_nwk_disc_rsp_t;

TYPEDEF_STRUCT_PACKED s_zb_znp_mt_zdo_mgmt_lqi_req
{
    uint16_t dst_addr;
    uint8_t start_index;
} s_zb_znp_mt_zdo_mgmt_lqi_req_t;

TYPEDEF_STRUCT_PACKED s_zb_znp_mt_zdo_mgmt_lqi_rsp
{
    uint16_t src_addr;
    uint8_t status;
    uint8_t neighbor_table_entries;
    uint8_t start_index;
    uint8_t neighbor_lqi_list_count;
    s_zb_znp_mt_zdo_neighbor_lqi_list_item_t neighbor_lqi_list[66];
} s_zb_znp_mt_zdo_mgmt_lqi_rsp_t;

TYPEDEF_STRUCT_PACKED s_zb_znp_mt_zdo_mgmt_rtg_req
{
    uint16_t dst_addr;
    uint8_t start_index;
} s_zb_znp_mt_zdo_mgmt_rtg_req_t;

TYPEDEF_STRUCT_PACKED s_zb_znp_mt_zdo_mgmt_rtg_rsp
{
    uint16_t src_addr;
    uint8_t status;
    uint8_t routing_table_entries;
    uint8_t start_index;
    uint8_t routing_table_list_count;
    s_zb_znp_mt_zdo_routing_table_list_item_t routing_table_list[75];
} s_zb_znp_mt_zdo_mgmt_rtg_rsp_t;

TYPEDEF_STRUCT_PACKED s_zb_znp_mt_zdo_mgmt_bind_req
{
    uint16_t dst_addr;
    uint8_t start_index;
} s_zb_znp_mt_zdo_mgmt_bind_req_t;

TYPEDEF_STRUCT_PACKED s_zb_znp_mt_zdo_mgmt_bind_rsp
{
    uint16_t src_addr;
    uint8_t status;
    uint8_t binding_table_entries;
    uint8_t start_index;
    uint8_t binding_table_list_count;
    s_zb_znp_mt_zdo_binding_table_list_item_t binding_table_list[75];
} s_zb_znp_mt_zdo_mgmt_bind_rsp_t;

TYPEDEF_STRUCT_PACKED s_zb_znp_mt_zdo_mgmt_leave_req
{
    uint16_t dst_addr;
    uint64_t device_addr;
    uint8_t rejoin_flag : 1;
    uint8_t remove_children_flag : 1;
    uint8_t reserved : 6;
} s_zb_znp_mt_zdo_mgmt_leave_req_t;

TYPEDEF_STRUCT_PACKED s_zb_znp_mt_zdo_mgmt_leave_rsp
{
    uint16_t src_addr;
    uint8_t status;
} s_zb_znp_mt_zdo_mgmt_leave_rsp_t;

TYPEDEF_STRUCT_PACKED s_zb_znp_mt_zdo_mgmt_direct_join_req
{
    uint16_t dst_addr;
    uint64_t device_addr;
    uint8_t cap_info;
} s_zb_znp_mt_zdo_mgmt_direct_join_req_t;

TYPEDEF_STRUCT_PACKED s_zb_znp_mt_zdo_mgmt_direct_join_rsp
{
    uint16_t src_addr;
    uint8_t status;
} s_zb_znp_mt_zdo_mgmt_direct_join_rsp_t;

TYPEDEF_STRUCT_PACKED s_zb_znp_mt_zdo_mgmt_permit_join_req
{
    uint8_t addr_mode;
    uint16_t dst_addr;
    uint8_t duration;
    uint8_t tc_significance;
} s_zb_znp_mt_zdo_mgmt_permit_join_req_t;

TYPEDEF_STRUCT_PACKED s_zb_znp_mt_zdo_mgmt_permit_join_rsp
{
    uint16_t src_addr;
    uint8_t status;
} s_zb_znp_mt_zdo_mgmt_permit_join_rsp_t;

TYPEDEF_STRUCT_PACKED s_zb_znp_mt_zdo_mgmt_nwk_update_req
{
    uint16_t dst_addr;
    uint8_t dst_addr_mode;
    uint32_t channel_mask;
    uint8_t scan_duration;
    uint8_t scan_count;
    uint16_t nwk_manager_addr;
} s_zb_znp_mt_zdo_mgmt_nwk_update_req_t;

TYPEDEF_STRUCT_PACKED s_zb_znp_mt_zdo_msg_cb_register
{
    uint16_t cluster_id;
} s_zb_znp_mt_zdo_msg_cb_register_t;

TYPEDEF_STRUCT_PACKED s_zb_znp_mt_zdo_msg_cb_remove_t
{
    uint16_t cluster_id;
} s_zb_znp_mt_zdo_msg_cb_remove_t;

TYPEDEF_STRUCT_PACKED s_zb_znp_mt_zdo_startup_from_app_t
{
    uint16_t start_delay;
} s_zb_znp_mt_zdo_startup_from_app_t;

TYPEDEF_STRUCT_PACKED s_zb_znp_mt_zdo_ext_nwk_info_srsp_t
{
    uint16_t short_addr;
    uint8_t device_state;
    uint16_t pan_id;
    uint16_t parent_addr;
    uint64_t ext_pan_id;
    uint64_t ext_parent_addr;
    uint8_t channel;
} s_zb_znp_mt_zdo_ext_nwk_info_srsp_t;

TYPEDEF_STRUCT_PACKED s_zb_znp_mt_zdo_set_link_key_t
{
    uint16_t short_addr;
    uint64_t ieee_addr;
    uint8_t link_key_data[16];
} s_zb_znp_mt_zdo_set_link_key_t;

TYPEDEF_STRUCT_PACKED s_zb_znp_mt_zdo_remove_link_key_t
{
    uint64_t ieee_addr;
} s_zb_znp_mt_zdo_remove_link_key_t;

TYPEDEF_STRUCT_PACKED s_zb_znp_mt_zdo_get_link_key_t
{
    uint64_t ieee_addr;
} s_zb_znp_mt_zdo_get_link_key_t;

TYPEDEF_STRUCT_PACKED s_zb_znp_mt_zdo_get_link_key_srsp_t
{
    uint64_t ieee_addr;
    uint8_t link_key_data[16];
} s_zb_znp_mt_zdo_get_link_key_srsp_t;

TYPEDEF_STRUCT_PACKED s_zb_znp_mt_zdo_nwk_discovery_req_t
{
    uint32_t scan_channels;
    uint8_t scan_duration;
} s_zb_znp_mt_zdo_nwk_discovery_req_t;

TYPEDEF_STRUCT_PACKED s_zb_znp_mt_zdo_join_req_t
{
    uint8_t logical_channel;
    uint16_t pan_id;
    uint64_t ext_pan_id;
    uint16_t chosen_parent;
    uint8_t parent_depth;
    uint8_t stack_profile;
} s_zb_znp_mt_zdo_join_req_t;

TYPEDEF_STRUCT_PACKED s_zb_znp_mt_zdo_end_device_annce_ind_t
{
    uint16_t src_addr;
    uint16_t nwk_addr;
    uint64_t ieee_addr;
    uint8_t capabilities;
} s_zb_znp_mt_zdo_end_device_annce_ind_t;

TYPEDEF_STRUCT_PACKED s_zb_znp_mt_zdo_match_desc_rsp_sent_t
{
    uint16_t src_addr;
    uint8_t num_in_clusters;
    uint16_t in_cluster_list[16];
    uint8_t num_out_clusters;
    uint16_t out_cluster_list[16];
} s_zb_znp_mt_zdo_match_desc_rsp_sent_t;

TYPEDEF_STRUCT_PACKED s_zb_znp_mt_zdo_status_error_rsp_t
{
    uint16_t src_addr;
    uint8_t status;
} s_zb_znp_mt_zdo_status_error_rsp_t;

TYPEDEF_STRUCT_PACKED s_zb_znp_mt_zdo_src_rtg_ind_t
{
    uint16_t dst_addr;
    uint8_t relay_count;
    uint16_t relay_list[255];
} s_zb_znp_mt_zdo_src_rtg_ind_t;

TYPEDEF_STRUCT_PACKED s_zb_znp_mt_zdo_beacon_notify_ind_t
{
    uint8_t beacon_count;
    s_zb_znp_mt_zdo_beacon_list_item_t beacon_list[21];
} s_zb_znp_mt_zdo_beacon_notify_ind_t;

TYPEDEF_STRUCT_PACKED s_zb_znp_mt_zdo_join_cnf_t
{
    uint8_t status;
    uint16_t dev_addr;
    uint16_t parent_addr;
} s_zb_znp_mt_zdo_join_cnf_t;

TYPEDEF_STRUCT_PACKED s_zb_znp_mt_zdo_nwk_discovery_cnf_t
{
    uint8_t status;
} s_zb_znp_mt_zdo_nwk_discovery_cnf_t;

TYPEDEF_STRUCT_PACKED s_zb_znp_mt_zdo_leave_ind_t
{
    uint16_t src_addr;
    uint64_t ext_addr;
    uint8_t request;
    uint8_t remove;
    uint8_t rejoin;
} s_zb_znp_mt_zdo_leave_ind_t;

TYPEDEF_STRUCT_PACKED s_zb_znp_mt_zdo_msg_cb_incoming_t
{
    uint16_t src_addr;
    uint8_t was_broadcast;
    uint16_t cluster_id;
    uint8_t security_use;
    uint8_t seq_num;
    uint16_t mac_dst_addr;
    uint8_t data[];
} s_zb_znp_mt_zdo_msg_cb_incoming_t;

TYPEDEF_STRUCT_PACKED s_zb_znp_mt_zdo_tc_device_ind_t
{
    uint16_t src_addr;
    uint64_t src_ieee_addr;
    uint16_t parent_addr;
} s_zb_znp_mt_zdo_tc_device_ind_t;

typedef enum
{
    ZNP_ZDO_STATE_HOLD,              // Initialized - not started automaticaly
    ZNP_ZDO_STATE_INIT,              // Initialized - not connected to anything
    ZNP_ZDO_STATE_NWK_DISC,          // Discovering PAN's to join
    ZNP_ZDO_STATE_NWK_JOINING,       // Joining a PAN
    ZNP_ZDO_STATE_NWK_REJOIN,        // Rejoining a PAN, only for end devices
    ZNP_ZDO_STATE_END_DEVICE_UNAUTH, // Joined but not yet authenticated by trust center
    ZNP_ZDO_STATE_END_DEVICE,        // Started as end device after authentication
    ZNP_ZDO_STATE_ROUTER,            // Device joined, authenticated and acting as a router
    ZNP_ZDO_STATE_COORD_STARTING,    // Starting as Zigbee Coordinator
    ZNP_ZDO_STATE_ZB_COORD,          // Started as Zigbee Coordinator
    ZNP_ZDO_STATE_NWK_ORPHAN         // Device has lost information about its parent..
} e_zb_znp_mt_zdo_state_t;

typedef int (*zb_znp_mt_zdo_nwk_addr_rsp_cb_t)(const s_zb_znp_mt_zdo_nwk_addr_rsp_t *rsp);
typedef int (*zb_znp_mt_zdo_ieee_addr_rsp_cb_t)(const s_zb_znp_mt_zdo_ieee_addr_rsp_t *rsp);
typedef int (*zb_znp_mt_zdo_node_desc_rsp_cb_t)(const s_zb_znp_mt_zdo_node_desc_rsp_t *rsp);
typedef int (*zb_znp_mt_zdo_power_desc_rsp_cb_t)(const s_zb_znp_mt_zdo_power_desc_rsp_t *rsp);
typedef int (*zb_znp_mt_zdo_simple_desc_rsp_cb_t)(const s_zb_znp_mt_zdo_simple_desc_rsp_t *rsp);
typedef int (*zb_znp_mt_zdo_active_ep_rsp_cb_t)(const s_zb_znp_mt_zdo_active_ep_rsp_t *rsp);
typedef int (*zb_znp_mt_zdo_match_desc_rsp_cb_t)(const s_zb_znp_mt_zdo_match_desc_rsp_t *rsp);
typedef int (*zb_znp_mt_zdo_complex_desc_rsp_cb_t)(const s_zb_znp_mt_zdo_complex_desc_rsp_t *rsp);
typedef int (*zb_znp_mt_zdo_user_desc_rsp_cb_t)(const s_zb_znp_mt_zdo_user_desc_rsp_t *rsp);
typedef int (*zb_znp_mt_zdo_user_desc_conf_cb_t)(const s_zb_znp_mt_zdo_user_desc_conf_t *conf);
typedef int (*zb_znp_mt_zdo_server_disc_rsp_cb_t)(const s_zb_znp_mt_zdo_server_disc_rsp_t *rsp);
typedef int (*zb_znp_mt_zdo_end_device_bind_rsp_cb_t)(const s_zb_znp_mt_zdo_end_device_bind_rsp_t *rsp);
typedef int (*zb_znp_mt_zdo_bind_rsp_cb_t)(const s_zb_znp_mt_zdo_bind_unbind_rsp_t *rsp);
typedef int (*zb_znp_mt_zdo_unbind_rsp_cb_t)(const s_zb_znp_mt_zdo_bind_unbind_rsp_t *rsp);
typedef int (*zb_znp_mt_zdo_mgmt_nwk_disc_rsp_cb_t)(const s_zb_znp_mt_zdo_mgmt_nwk_disc_rsp_t *rsp);
typedef int (*zb_znp_mt_zdo_mgmt_lqi_rsp_cb_t)(const s_zb_znp_mt_zdo_mgmt_lqi_rsp_t *rsp);
typedef int (*zb_znp_mt_zdo_mgmt_rtg_rsp_cb_t)(const s_zb_znp_mt_zdo_mgmt_rtg_rsp_t *rsp);
typedef int (*zb_znp_mt_zdo_mgmt_bind_rsp_cb_t)(const s_zb_znp_mt_zdo_mgmt_bind_rsp_t *rsp);
typedef int (*zb_znp_mt_zdo_mgmt_leave_rsp_cb_t)(const s_zb_znp_mt_zdo_mgmt_leave_rsp_t *rsp);
typedef int (*zb_znp_mt_zdo_mgmt_direct_join_rsp_cb_t)(const s_zb_znp_mt_zdo_mgmt_direct_join_rsp_t *rsp);
typedef int (*zb_znp_mt_zdo_mgmt_permit_join_rsp_cb_t)(const s_zb_znp_mt_zdo_mgmt_permit_join_rsp_t *rsp);
typedef int (*zb_znp_mt_zdo_state_change_ind_cb_t)(const e_zb_znp_mt_zdo_state_t dev_state);
typedef int (*zb_znp_mt_zdo_end_device_annce_ind_cb_t)(const s_zb_znp_mt_zdo_end_device_annce_ind_t *ind);
typedef int (*zb_znp_mt_zdo_match_desc_rsp_sent_cb_t)(const s_zb_znp_mt_zdo_match_desc_rsp_sent_t *sent);
typedef int (*zb_znp_mt_zdo_status_error_rsp_cb_t)(const s_zb_znp_mt_zdo_status_error_rsp_t *rsp);
typedef int (*zb_znp_mt_zdo_src_rtg_ind_cb_t)(const s_zb_znp_mt_zdo_src_rtg_ind_t *ind);
typedef int (*zb_znp_mt_zdo_beacon_notify_ind_cb_t)(const s_zb_znp_mt_zdo_beacon_notify_ind_t *ind);
typedef int (*zb_znp_mt_zdo_join_cnf_cb_t)(const s_zb_znp_mt_zdo_join_cnf_t *cnf);
typedef int (*zb_znp_mt_zdo_nwk_discovery_cnf_cb_t)(const s_zb_znp_mt_zdo_nwk_discovery_cnf_t *cnf);
typedef int (*zb_znp_mt_zdo_leave_ind_cb_t)(const s_zb_znp_mt_zdo_leave_ind_t *ind);
typedef int (*zb_znp_mt_zdo_msg_cb_incoming_cb_t)(const s_zb_znp_mt_zdo_msg_cb_incoming_t *ind);
typedef int (*zb_znp_mt_zdo_tc_device_ind_cb_t)(const s_zb_znp_mt_zdo_tc_device_ind_t *ind);

typedef struct
{
    zb_znp_mt_zdo_nwk_addr_rsp_cb_t pfn_zdo_nwk_addr_rsp_cb;
    zb_znp_mt_zdo_ieee_addr_rsp_cb_t pfn_zdo_ieee_addr_rsp_cb;
    zb_znp_mt_zdo_node_desc_rsp_cb_t pfn_zdo_node_desc_rsp_cb;
    zb_znp_mt_zdo_power_desc_rsp_cb_t pfn_zdo_power_desc_rsp_cb;
    zb_znp_mt_zdo_simple_desc_rsp_cb_t pfn_zdo_simple_desc_rsp_cb;
    zb_znp_mt_zdo_active_ep_rsp_cb_t pfn_zdo_active_ep_rsp_cb;
    zb_znp_mt_zdo_match_desc_rsp_cb_t pfn_zdo_match_desc_rsp_cb;
    zb_znp_mt_zdo_complex_desc_rsp_cb_t pfn_zdo_complex_desc_rsp_cb;
    zb_znp_mt_zdo_user_desc_rsp_cb_t pfn_zdo_user_desc_rsp_cb;
    zb_znp_mt_zdo_user_desc_conf_cb_t pfn_zdo_user_desc_conf_cb;
    zb_znp_mt_zdo_server_disc_rsp_cb_t pfn_zdo_server_disc_rsp_cb;
    zb_znp_mt_zdo_end_device_bind_rsp_cb_t pfn_zdo_end_device_bind_rsp_cb;
    zb_znp_mt_zdo_bind_rsp_cb_t pfn_zdo_bind_rsp_cb;
    zb_znp_mt_zdo_unbind_rsp_cb_t pfn_zdo_unbind_rsp_cb;
    zb_znp_mt_zdo_mgmt_nwk_disc_rsp_cb_t pfn_zdo_mgmt_nwk_disc_rsp_cb;
    zb_znp_mt_zdo_mgmt_lqi_rsp_cb_t pfn_zdo_mgmt_lqi_rsp_cb;
    zb_znp_mt_zdo_mgmt_rtg_rsp_cb_t pfn_zdo_mgmt_rtg_rsp_cb;
    zb_znp_mt_zdo_mgmt_bind_rsp_cb_t pfn_zdo_mgmt_bind_rsp_cb;
    zb_znp_mt_zdo_mgmt_leave_rsp_cb_t pfn_zdo_mgmt_leave_rsp_cb;
    zb_znp_mt_zdo_mgmt_direct_join_rsp_cb_t pfn_zdo_mgmt_direct_join_rsp_cb;
    zb_znp_mt_zdo_mgmt_permit_join_rsp_cb_t pfn_zdo_mgmt_permit_join_rsp_cb;
    zb_znp_mt_zdo_state_change_ind_cb_t pfn_zdo_state_change_ind_cb;
    zb_znp_mt_zdo_end_device_annce_ind_cb_t pfn_zdo_end_device_annce_ind_cb;
    zb_znp_mt_zdo_match_desc_rsp_sent_cb_t pfn_zdo_match_desc_rsp_sent_cb;
    zb_znp_mt_zdo_status_error_rsp_cb_t pfn_zdo_status_error_rsp_cb;
    zb_znp_mt_zdo_src_rtg_ind_cb_t pfn_zdo_src_rtg_ind_cb;
    zb_znp_mt_zdo_beacon_notify_ind_cb_t pfn_zdo_beacon_notify_ind_cb;
    zb_znp_mt_zdo_join_cnf_cb_t pfn_zdo_join_cnf_cb;
    zb_znp_mt_zdo_nwk_discovery_cnf_cb_t pfn_zdo_nwk_discovery_cnf_cb;
    zb_znp_mt_zdo_leave_ind_cb_t pfn_zdo_leave_ind_cb;
    zb_znp_mt_zdo_msg_cb_incoming_cb_t pfn_zdo_msg_cb_incoming_cb;
    zb_znp_mt_zdo_tc_device_ind_cb_t pfn_zdo_tc_device_ind_cb;
} s_zb_znp_mt_zdo_cb_t;

void zb_znp_mt_zdo_register_callback(s_zb_znp_mt_zdo_cb_t callback);
void zb_znp_mt_zdo_unregister_callback(void);

int zb_znp_mt_zdo_init(uint16_t start_delay);
void zb_znp_mt_zdo_process(const uint8_t *rpc_buff, uint8_t rpc_len);

int zb_znp_mt_zdo_nwk_addr_req(const s_zb_znp_mt_zdo_nwk_addr_req_t *req);
int zb_znp_mt_zdo_ieee_addr_req(const s_zb_znp_mt_zdo_ieee_addr_req_t *req);
int zb_znp_mt_zdo_node_desc_req(const s_zb_znp_mt_zdo_node_desc_req_t *req);
int zb_znp_mt_zdo_power_desc_req(const s_zb_znp_mt_zdo_power_desc_req_t *req);
int zb_znp_mt_zdo_simple_desc_req(const s_zb_znp_mt_zdo_simple_desc_req_t *req);
int zb_znp_mt_zdo_active_ep_req(const s_zb_znp_mt_zdo_active_ep_req_t *req);
int zb_znp_mt_zdo_match_desc_req(const s_zb_znp_mt_zdo_match_desc_req_t *req);
int zb_znp_mt_zdo_complex_desc_req(const s_zb_znp_mt_zdo_complex_desc_req_t *req);
int zb_znp_mt_zdo_user_desc_req(const s_zb_znp_mt_zdo_user_desc_req_t *req);
int zb_znp_mt_zdo_end_device_annce(const s_zb_znp_mt_zdo_end_device_annce_t *req);
int zb_znp_mt_zdo_user_desc_set(const s_zb_znp_mt_zdo_user_desc_set_t *req);
int zb_znp_mt_zdo_server_disc_req(const s_zb_znp_mt_zdo_server_disc_req_t *req);
int zb_znp_mt_zdo_end_device_bind_req(const s_zb_znp_mt_zdo_end_device_bind_req_t *req);
int zb_znp_mt_zdo_bind_device_req(const s_zb_znp_mt_zdo_bind_unbind_device_req_t *req);
int zb_znp_mt_zdo_bind_group_req(const s_zb_znp_mt_zdo_bind_unbind_group_req_t *req);
int zb_znp_mt_zdo_unbind_device_req(const s_zb_znp_mt_zdo_bind_unbind_device_req_t *req);
int zb_znp_mt_zdo_unbind_group_req(const s_zb_znp_mt_zdo_bind_unbind_group_req_t *req);
int zb_znp_mt_zdo_mgmt_nwk_disc_req(const s_zb_znp_mt_zdo_mgmt_nwk_disc_req_t *req);
int zb_znp_mt_zdo_mgmt_lqi_req(const s_zb_znp_mt_zdo_mgmt_lqi_req_t *req);
int zb_znp_mt_zdo_mgmt_rtg_req(const s_zb_znp_mt_zdo_mgmt_rtg_req_t *req);
int zb_znp_mt_zdo_mgmt_bind_req(const s_zb_znp_mt_zdo_mgmt_bind_req_t *req);
int zb_znp_mt_zdo_mgmt_leave_req(const s_zb_znp_mt_zdo_mgmt_leave_req_t *req);
int zb_znp_mt_zdo_mgmt_direct_join_req(const s_zb_znp_mt_zdo_mgmt_direct_join_req_t *req);
int zb_znp_mt_zdo_mgmt_permit_join_req(const s_zb_znp_mt_zdo_mgmt_permit_join_req_t *req);
int zb_znp_mt_zdo_mgmt_nwk_update_req(const s_zb_znp_mt_zdo_mgmt_nwk_update_req_t *req);
int zb_znp_mt_zdo_msg_cb_register(const s_zb_znp_mt_zdo_msg_cb_register_t *req);
int zb_znp_mt_zdo_msg_cb_remove(const s_zb_znp_mt_zdo_msg_cb_remove_t *req);
int zb_znp_mt_zdo_startup_from_app(const s_zb_znp_mt_zdo_startup_from_app_t *req);
int zb_znp_mt_zdo_ext_nwk_info(s_zb_znp_mt_zdo_ext_nwk_info_srsp_t *rsp);
int zb_znp_mt_zdo_set_link_key(const s_zb_znp_mt_zdo_set_link_key_t *req);
int zb_znp_mt_zdo_remove_link_key(const s_zb_znp_mt_zdo_remove_link_key_t *req);
int zb_znp_mt_zdo_get_link_key(
    const s_zb_znp_mt_zdo_get_link_key_t *req,
    s_zb_znp_mt_zdo_get_link_key_srsp_t *rsp);
int zb_znp_mt_zdo_nwk_discovery_req(const s_zb_znp_mt_zdo_nwk_discovery_req_t *req);
int zb_znp_mt_zdo_join_req(const s_zb_znp_mt_zdo_join_req_t *req);

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* ZB_ZNP_MT_ZDO_H_ */
