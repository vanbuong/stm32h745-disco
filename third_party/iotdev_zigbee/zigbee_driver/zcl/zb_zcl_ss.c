/*
 * zb_zcl_ss.c
 *
 * Author: Vo Van Buong (BRT-SG)
 */

#include "common/zb_common.h"
#include "zcl/zb_zcl.h"
#include "zcl/zb_zcl_general.h"
#include "zcl/zb_zcl_ss.h"

#define TAG "ZCL_SS"

/*******************************************************************************
 * MACROS
 */

#define ZCL_SS_ZONE_TYPE_SUPPORT(zone_type) (zone_type == SS_IAS_ZONE_TYPE_STANDARD_CIE             || \
                                            zone_type == SS_IAS_ZONE_TYPE_MOTION_SENSOR             || \
                                            zone_type == SS_IAS_ZONE_TYPE_CONTACT_SWITCH            || \
                                            zone_type == SS_IAS_ZONE_TYPE_DOOR_WINDOW_HANDLE        || \
                                            zone_type == SS_IAS_ZONE_TYPE_FIRE_SENSOR               || \
                                            zone_type == SS_IAS_ZONE_TYPE_WATER_SENSOR              || \
                                            zone_type == SS_IAS_ZONE_TYPE_CO_SENSOR                 || \
                                            zone_type == SS_IAS_ZONE_TYPE_PERSONAL_EMERGENCY_DEVICE || \
                                            zone_type == SS_IAS_ZONE_TYPE_VIBRATION_MOVEMENT_SENSOR || \
                                            zone_type == SS_IAS_ZONE_TYPE_REMOTE_CONTROL            || \
                                            zone_type == SS_IAS_ZONE_TYPE_KEY_FOB                   || \
                                            zone_type == SS_IAS_ZONE_TYPE_KEYPAD                    || \
                                            zone_type == SS_IAS_ZONE_TYPE_STANDARD_WARNING_DEVICE   || \
                                            zone_type == SS_IAS_ZONE_TYPE_GLASS_BREAK_SENSOR        || \
                                            zone_type == SS_IAS_ZONE_TYPE_SECURITY_REPEATER)

/*******************************************************************************
 * TYPEDEFS
 */
typedef struct s_zb_zcl_ss_cb_rec
{
    struct s_zb_zcl_ss_cb_rec *next;
    uint8_t endpoint;
    s_zb_zcl_ss_app_callbacks_t *cb;
} s_zb_zcl_ss_cb_rec_t;

typedef struct s_zb_zcl_ss_zone_item
{
  struct s_zb_zcl_ss_zone_item *next;
  uint8_t endpoint;
  s_zb_zcl_ias_ace_zone_table_t zone;
} s_zb_zcl_ss_zone_item_t;

/*******************************************************************************
 * GLOBAL VARIABLES
 */
const uint64_t g_zcl_ss_unknown_ieee_address = 0xFFFFFFFFFFFFFFFF;

/*******************************************************************************
 * LOCAL VARIABLES
 */
static s_zb_zcl_ss_cb_rec_t *g_zcl_ss_cb_list = NULL;
static uint8_t g_zcl_ss_plugin_registered = false;
static s_zb_zcl_ss_zone_item_t *g_zcl_ss_zone_list = NULL;

/*******************************************************************************
 * LOCAL FUNCTIONS
 */
static zb_status_t zcl_ss_handle_incoming(s_zb_zcl_incoming_msg_t *msg);
static zb_status_t zcl_ss_handle_in_specific_commands(s_zb_zcl_incoming_msg_t *msg);
static s_zb_zcl_ss_app_callbacks_t *zcl_ss_find_app_callbacks(uint8_t endpoint);

// ZCL IAS Zone Commands
static zb_status_t zcl_ss_process_in_zone_status_cmds_client(
    s_zb_zcl_incoming_msg_t *msg, s_zb_zcl_ss_app_callbacks_t *cb);
static zb_status_t zcl_ss_process_in_zone_status_change_notification(
    s_zb_zcl_incoming_msg_t *msg, s_zb_zcl_ss_app_callbacks_t *cb);
static zb_status_t zcl_ss_process_in_zone_status_enroll_request(
    s_zb_zcl_incoming_msg_t *msg, s_zb_zcl_ss_app_callbacks_t *cb);

// ZCL ACE Commands
static zb_status_t zcl_ss_process_in_ace_cmds_server(
    s_zb_zcl_incoming_msg_t *msg, s_zb_zcl_ss_app_callbacks_t *cb);
static zb_status_t zcl_ss_process_in_ace_cmds_client(
    s_zb_zcl_incoming_msg_t *msg, s_zb_zcl_ss_app_callbacks_t *cb);
static zb_status_t zcl_ss_process_in_cmd_ace_arm(
    s_zb_zcl_incoming_msg_t *msg, s_zb_zcl_ss_app_callbacks_t *cb);
static zb_status_t zcl_ss_process_in_cmd_ace_bypass(
    s_zb_zcl_incoming_msg_t *msg, s_zb_zcl_ss_app_callbacks_t *cb);
static zb_status_t zcl_ss_process_in_cmd_ace_emergency(
    s_zb_zcl_incoming_msg_t *msg, s_zb_zcl_ss_app_callbacks_t *cb);
static zb_status_t zcl_ss_process_in_cmd_ace_fire(
    s_zb_zcl_incoming_msg_t *msg, s_zb_zcl_ss_app_callbacks_t *cb);
static zb_status_t zcl_ss_process_in_cmd_ace_panic(
    s_zb_zcl_incoming_msg_t *msg, s_zb_zcl_ss_app_callbacks_t *cb);
static zb_status_t zcl_ss_process_in_cmd_ace_get_zone_id_map(
    s_zb_zcl_incoming_msg_t *msg, s_zb_zcl_ss_app_callbacks_t *cb);
static zb_status_t zcl_ss_process_in_cmd_ace_get_zone_information(
    s_zb_zcl_incoming_msg_t *msg, s_zb_zcl_ss_app_callbacks_t *cb);
static zb_status_t zcl_ss_process_in_cmd_ace_get_panel_status(
    s_zb_zcl_incoming_msg_t *msg, s_zb_zcl_ss_app_callbacks_t *cb);
static zb_status_t zcl_ss_process_in_cmd_ace_get_bypassed_zone_list(
    s_zb_zcl_incoming_msg_t *msg, s_zb_zcl_ss_app_callbacks_t *cb);
static zb_status_t zcl_ss_process_in_cmd_ace_get_zone_status(
    s_zb_zcl_incoming_msg_t *msg, s_zb_zcl_ss_app_callbacks_t *cb);
static zb_status_t zcl_ss_process_in_cmd_ace_arm_response(
    s_zb_zcl_incoming_msg_t *msg, s_zb_zcl_ss_app_callbacks_t *cb);
static zb_status_t zcl_ss_process_in_cmd_ace_get_zone_id_map_response(
    s_zb_zcl_incoming_msg_t *msg, s_zb_zcl_ss_app_callbacks_t *cb);
static zb_status_t zcl_ss_process_in_cmd_ace_get_zone_information_response(
    s_zb_zcl_incoming_msg_t *msg, s_zb_zcl_ss_app_callbacks_t *cb);
static zb_status_t zcl_ss_process_in_cmd_ace_zone_status_changed(
    s_zb_zcl_incoming_msg_t *msg, s_zb_zcl_ss_app_callbacks_t *cb);
static zb_status_t zcl_ss_process_in_cmd_ace_panel_status_changed(
    s_zb_zcl_incoming_msg_t *msg, s_zb_zcl_ss_app_callbacks_t *cb);
static zb_status_t zcl_ss_process_in_cmd_ace_get_panel_status_response(
    s_zb_zcl_incoming_msg_t *msg, s_zb_zcl_ss_app_callbacks_t *cb);
static zb_status_t zcl_ss_process_in_cmd_ace_set_bypassed_zone_list(
    s_zb_zcl_incoming_msg_t *msg, s_zb_zcl_ss_app_callbacks_t *cb);
static zb_status_t zcl_ss_process_in_cmd_ace_bypass_response(
    s_zb_zcl_incoming_msg_t *msg, s_zb_zcl_ss_app_callbacks_t *cb);
static zb_status_t zcl_ss_process_in_cmd_ace_get_zone_status_response(
    s_zb_zcl_incoming_msg_t *msg, s_zb_zcl_ss_app_callbacks_t *cb);

// ZCL IAS Zone Helper Functions
static uint8_t zcl_ss_get_next_free_zone_id(void);
static zb_status_t zcl_ss_add_zone(uint8_t endpoint, s_zb_zcl_ias_ace_zone_table_t *zone);
static uint8_t zcl_ss_count_all_zones(void);
static uint8_t zcl_ss_zone_id_available(uint8_t zone_id);

// ZCL ACE Helper Functions
static uint8_t zcl_ss_parse_utf8_string(uint8_t *buf, s_utf8_string_t *str, uint8_t max_len);

zb_status_t
zb_zcl_ss_register_command_callbacks(uint8_t endpoint, s_zb_zcl_ss_app_callbacks_t *callbacks)
{
    s_zb_zcl_ss_cb_rec_t *p_new_item;
    s_zb_zcl_ss_cb_rec_t *p_loop;

    if (!g_zcl_ss_plugin_registered)
    {
        zb_zcl_register_plugin(
            ZCL_CLUSTER_ID_SS_IAS_ZONE,
            ZCL_CLUSTER_ID_SS_IAS_WD,
            zcl_ss_handle_incoming);
        g_zcl_ss_plugin_registered = true;
    }

    p_new_item = (s_zb_zcl_ss_cb_rec_t *)ZB_MEM_MALLOC(sizeof(s_zb_zcl_ss_cb_rec_t));
    if (p_new_item == NULL)
    {
        return ZB_MEM_ERROR;
    }
    p_new_item->next = NULL;
    p_new_item->endpoint = endpoint;
    p_new_item->cb = callbacks;

    if (g_zcl_ss_cb_list == NULL)
    {
        g_zcl_ss_cb_list = p_new_item;
    }
    else
    {
        p_loop = g_zcl_ss_cb_list;
        while (p_loop->next != NULL)
        {
            p_loop = p_loop->next;
        }
        p_loop->next = p_new_item;
    }
    return ZB_SUCCESS;
}

/**
 * @brief Send IAS Zone Enroll Command Response
 *
 * @param src_ep - Sending Application's Endpoint
 * @param dst_addr - Where to send the command
 * @param response_code - Response Code
 * @param zone_id - Zone ID
 * @param disable_default_rsp - Toggle for enabling/disabling default response
 * @param seq_num - Command Sequence Number
 * @return int - 0 if success, otherwise error code
 */
zb_status_t
zb_zcl_ss_ias_send_zone_status_enroll_response_cmd(
    uint8_t src_ep, s_zb_af_address_t *dst_addr,
    uint8_t response_code,uint8_t zone_id,
    uint8_t disable_default_rsp, uint8_t seq_num)
{
    uint8_t buf[PAYLOAD_LEN_ZONE_STATUS_ENROLL_RSP];

    buf[0] = response_code;
    buf[1] = zone_id;

    return zb_zcl_send_cmd(
                src_ep, dst_addr, ZCL_CLUSTER_ID_SS_IAS_ZONE,
                COMMAND_IAS_ZONE_ZONE_ENROLL_RESPONSE, true,
                ZCL_FRAME_CLIENT_SERVER_DIR, disable_default_rsp, 0,
                seq_num, PAYLOAD_LEN_ZONE_STATUS_ENROLL_RSP, buf);
}

// ZCL ACE Cluster Client Commands
zb_status_t
zb_zcl_ss_send_ias_ace_arm_cmd(uint8_t src_ep, s_zb_af_address_t *dst_addr,
                                            s_zb_zcl_ace_arm_t *cmd,
                                            uint8_t disable_default_rsp, uint8_t seq_num)
{
    uint8_t *pbuf;
    uint8_t *pout_buf;
    uint8_t len = sizeof(s_zb_zcl_ace_arm_t) -
                  sizeof(s_utf8_string_t) + cmd->arm_disarm_code.str_len + 1;
    zb_status_t status;

    pbuf = (uint8_t *)ZB_MEM_MALLOC(len);
    if (pbuf == NULL)
    {
        return ZB_MEM_ERROR;
    }
    pout_buf = pbuf;
    *pout_buf++ = cmd->arm_mode;
    *pout_buf++ = cmd->arm_disarm_code.str_len;
    if (cmd->arm_disarm_code.str_len > 0)
    {
        memcpy(pout_buf, cmd->arm_disarm_code.pstr, cmd->arm_disarm_code.str_len);
        pout_buf += cmd->arm_disarm_code.str_len;
    }
    *pout_buf++ = cmd->zone_id;

    status = zb_zcl_send_cmd(
                src_ep, dst_addr, ZCL_CLUSTER_ID_SS_IAS_ACE,
                COMMAND_IASACE_ARM, true,
                ZCL_FRAME_CLIENT_SERVER_DIR, disable_default_rsp, 0,
                seq_num, len, pbuf);
    ZB_MEM_FREE(pbuf);
    return status;
}

zb_status_t
zb_zcl_ss_send_ias_ace_bypass_cmd(
    uint8_t src_ep, s_zb_af_address_t *dst_addr,
    s_zb_zcl_ace_bypass_t *cmd,
    uint8_t disable_default_rsp, uint8_t seq_num)
{
    uint8_t *pbuf;
    uint8_t *pout_buf;
    uint8_t len = 1 + cmd->number_of_zones + cmd->arm_disarm_code.str_len + 1;
    zb_status_t status;

    pbuf = (uint8_t *)ZB_MEM_MALLOC(len);
    if (pbuf == NULL)
    {
        return ZB_MEM_ERROR;
    }
    pout_buf = pbuf;
    *pout_buf++ = cmd->number_of_zones;
    if (cmd->number_of_zones > 0)
    {
        memcpy(pout_buf, cmd->bypass_buf, cmd->number_of_zones);
        pout_buf += cmd->number_of_zones;
    }
    *pout_buf++ = cmd->arm_disarm_code.str_len;
    if (cmd->arm_disarm_code.str_len > 0)
    {
        memcpy(pout_buf, cmd->arm_disarm_code.pstr, cmd->arm_disarm_code.str_len);
        pout_buf += cmd->arm_disarm_code.str_len;
    }

    status = zb_zcl_send_cmd(
                src_ep, dst_addr, ZCL_CLUSTER_ID_SS_IAS_ACE,
                COMMAND_IASACE_BYPASS, true,
                ZCL_FRAME_CLIENT_SERVER_DIR, disable_default_rsp, 0,
                seq_num, len, pbuf);
    ZB_MEM_FREE(pbuf);
    return status;
}

zb_status_t
zb_zcl_ss_send_ias_ace_get_zone_information_cmd(
    uint8_t src_ep, s_zb_af_address_t *dst_addr,
    uint8_t zone_id, uint8_t disable_default_rsp, uint8_t seq_num)
{
  uint8_t buf[1];
  buf[0] = zone_id;
  return zb_zcl_send_cmd(
            src_ep, dst_addr, ZCL_CLUSTER_ID_SS_IAS_ACE,
            COMMAND_IASACE_GET_ZONE_INFORMATION, true,
            ZCL_FRAME_CLIENT_SERVER_DIR, disable_default_rsp, 0,
            seq_num, 1, buf);
}

zb_status_t
zb_zcl_ss_send_ias_ace_get_zone_status_cmd(
    uint8_t src_ep, s_zb_af_address_t *dst_addr,
    s_zb_zcl_ace_get_zone_status_t *cmd,
    uint8_t disable_default_rsp, uint8_t seq_num)
{
    uint8_t buf[PAYLOAD_LEN_GET_ZONE_STATUS];
    buf[0] = cmd->starting_zone_id;
    buf[1] = cmd->max_num_zone_ids;
    buf[2] = cmd->zone_status_mask_flag;
    buf[3] = LO_UINT16(cmd->zone_status_mask);
    buf[4] = HI_UINT16(cmd->zone_status_mask);
    return zb_zcl_send_cmd(
                src_ep, dst_addr, ZCL_CLUSTER_ID_SS_IAS_ACE,
                COMMAND_IASACE_GET_ZONE_STATUS, true,
                ZCL_FRAME_CLIENT_SERVER_DIR, disable_default_rsp, 0,
                seq_num, PAYLOAD_LEN_GET_ZONE_STATUS, buf);
}

zb_status_t
zb_zcl_ss_send_ias_wd_start_warning_cmd(
    uint8_t src_ep, s_zb_af_address_t *dst_addr,
    s_zb_zcl_wd_start_warning_t *cmd,
    uint8_t disable_default_rsp, uint8_t seq_num)
{
    uint8_t buf[5]; // [warning_mode + strobe + siren_level] + warning_duration + strobe_duty_cycle + strobe_level

    buf[0] = cmd->warning_message.warning_byte;
    buf[1] = LO_UINT16(cmd->warning_duration);
    buf[2] = HI_UINT16(cmd->warning_duration);
    buf[3] = cmd->strobe_duty_cycle;
    buf[4] = cmd->strobe_level;
    return zb_zcl_send_cmd(
                src_ep, dst_addr, ZCL_CLUSTER_ID_SS_IAS_WD,
                COMMAND_IAS_WD_START_WARNING, true,
                ZCL_FRAME_CLIENT_SERVER_DIR, disable_default_rsp, 0,
                seq_num, 5, buf);
}

zb_status_t
zb_zcl_ss_send_ias_wd_squawk_cmd(
    uint8_t src_ep, s_zb_af_address_t *dst_addr,
    s_zb_zcl_wd_squawk_t *cmd,
    uint8_t disable_default_rsp, uint8_t seq_num)
{
    uint8_t buf[1];
    buf[0] = cmd->squawk_byte;
    return zb_zcl_send_cmd(
                src_ep, dst_addr, ZCL_CLUSTER_ID_SS_IAS_WD,
                COMMAND_IAS_WD_SQUAWK, true,
                ZCL_FRAME_CLIENT_SERVER_DIR, disable_default_rsp, 0,
                seq_num, 1, buf);
}

// ZCL ACE Cluster Server Commands
/**
 * @brief Send IAS ACE Arm Response Command
 * 
 * @param src_ep - Source Endpoint
 * @param dst_addr - Destination Address
 * @param arm_notification - Arm Notification
 * @param disable_default_rsp - Disable Default Response
 * @param seq_num - Sequence Number
 * @return zb_status_t - Status
 */
zb_status_t zb_zcl_ss_send_ias_ace_arm_response_cmd(
    uint8_t src_ep, s_zb_af_address_t *dst_addr,
    uint8_t arm_notification, uint8_t disable_default_rsp, uint8_t seq_num)
{
    uint8_t buf[1];
    buf[0] = arm_notification;

    return zb_zcl_send_cmd(
                src_ep, dst_addr, ZCL_CLUSTER_ID_SS_IAS_ACE,
                COMMAND_IASACE_ARM_RESPONSE, true,
                ZCL_FRAME_SERVER_CLIENT_DIR, disable_default_rsp, 0,
                seq_num, 1, buf);
}

/**
 * @brief Send IAS ACE Get Zone ID Map Response Command
 * 
 * @param src_ep - Source Endpoint
 * @param dst_addr - Destination Address
 * @param zone_id_map - pointer to an array of 16 uint16_t
 * @param disable_default_rsp - Disable Default Response
 * @param seq_num - Sequence Number
 * @return zb_status_t - Status
 */
zb_status_t zb_zcl_ss_send_ias_ace_get_zone_id_map_response_cmd(
    uint8_t src_ep, s_zb_af_address_t *dst_addr,
    uint16_t *zone_id_map, uint8_t disable_default_rsp, uint8_t seq_num)
{
    uint8_t *buf;
    uint8_t *pbuf;
    zb_status_t status;

    uint8_t len = (ZONE_ID_MAP_ARRAY_SIZE * sizeof(uint16_t));
    buf = ZB_MEM_MALLOC(len);

    if (buf)
    {
        pbuf = buf;
        for (uint8_t i = 0; i < ZONE_ID_MAP_ARRAY_SIZE; i++)
        {
            *pbuf++ = LO_UINT16(zone_id_map[i]);
            *pbuf++ = HI_UINT16(zone_id_map[i]);
        }

        status = zb_zcl_send_cmd(
                    src_ep, dst_addr, ZCL_CLUSTER_ID_SS_IAS_ACE,
                    COMMAND_IASACE_GET_ZONE_ID_MAP_RESPONSE, true,
                    ZCL_FRAME_SERVER_CLIENT_DIR, disable_default_rsp, 0,
                    seq_num, len, buf);
        ZB_MEM_FREE(buf);
        return status;
    }
    else
    {
        status = ZB_MEM_ERROR;
    }
    return status;
}

/**
 * @brief Send IAS ACE Get Zone Information Response Command
 * 
 * @param src_ep - Source Endpoint
 * @param dst_addr - Destination Address
 * @param rsp - Response structure
 * @param disable_default_rsp - Disable Default Response
 * @param seq_num - Sequence Number
 * @return zb_status_t - Status
 */
zb_status_t zb_zcl_ss_send_ias_ace_get_zone_information_response_cmd(
    uint8_t src_ep, s_zb_af_address_t *dst_addr,
    s_zb_zcl_ace_get_zone_info_rsp_t *rsp,
    uint8_t disable_default_rsp, uint8_t seq_num)
{
    uint8_t *buf;
    uint8_t *pbuf;
    zb_status_t status;

    // zone_id (1 byte) + zone_type (2 bytes) + ieee_addr (8 bytes) + zone_label (string)
    uint8_t len = 11 + (rsp->zone_label.str_len + 1);
    buf = ZB_MEM_MALLOC(len);

    if (buf)
    {
        pbuf = buf;
        *pbuf++ = rsp->zone_id;
        *pbuf++ = LO_UINT16(rsp->zone_type);
        *pbuf++ = HI_UINT16(rsp->zone_type);
        memcpy(pbuf, (uint8_t *)&rsp->ieee_addr, 8);
        pbuf += 8;
        *pbuf++ = rsp->zone_label.str_len;

        if (rsp->zone_label.str_len > 0)
        {
            memcpy(pbuf, rsp->zone_label.pstr, rsp->zone_label.str_len);
            pbuf += rsp->zone_label.str_len;
        }

        status = zb_zcl_send_cmd(
                    src_ep, dst_addr, ZCL_CLUSTER_ID_SS_IAS_ACE,
                    COMMAND_IASACE_GET_ZONE_INFORMATION_RESPONSE, true,
                    ZCL_FRAME_SERVER_CLIENT_DIR, disable_default_rsp, 0,
                    seq_num, len, buf);
        ZB_MEM_FREE(buf);
        return status;
    }
    else
    {
        status = ZB_MEM_ERROR;
    }
    return status;
}

/**
 * @brief Send IAS ACE Zone Status Changed Command
 * 
 * @param src_ep - Source Endpoint
 * @param dst_addr - Destination Address
 * @param cmd - Zone Status Changed Command
 * @param disable_default_rsp - Disable Default Response
 * @param seq_num - Sequence Number
 * @return zb_status_t - Status
 */
zb_status_t zb_zcl_ss_send_ias_ace_zone_status_changed_cmd(
    uint8_t src_ep, s_zb_af_address_t *dst_addr,
    s_zb_zcl_ace_zone_status_changed_t *cmd,
    uint8_t disable_default_rsp, uint8_t seq_num)
{
    uint8_t *buf;
    uint8_t *pbuf;
    uint8_t len = sizeof(s_zb_zcl_ace_zone_status_changed_t)
                    - sizeof(s_utf8_string_t) + (cmd->zone_label.str_len + 1);
    zb_status_t status;

    buf = ZB_MEM_MALLOC(len);
    if (buf)
    {
        pbuf = buf;
        *pbuf++ = cmd->zone_id;
        *pbuf++ = LO_UINT16(cmd->zone_status);
        *pbuf++ = HI_UINT16(cmd->zone_status);
        *pbuf++ = cmd->audible_notification;
        *pbuf++ = cmd->zone_label.str_len;
        if (cmd->zone_label.str_len > 0)
        {
            memcpy(pbuf, cmd->zone_label.pstr, cmd->zone_label.str_len);
            pbuf += cmd->zone_label.str_len;
        }

        status = zb_zcl_send_cmd(
                    src_ep, dst_addr, ZCL_CLUSTER_ID_SS_IAS_ACE,
                    COMMAND_IASACE_ZONE_STATUS_CHANGED, true,
                    ZCL_FRAME_SERVER_CLIENT_DIR, disable_default_rsp, 0,
                    seq_num, len, buf);
        ZB_MEM_FREE(buf);
        return status;
    }
    else
    {
        status = ZB_MEM_ERROR;
    }
    return status;
}

/**
 * @brief Send IAS ACE Panel Status Changed Command
 * 
 * @param src_ep - Source Endpoint
 * @param dst_addr - Destination Address
 * @param cmd - Panel Status Changed Command
 * @param disable_default_rsp - Disable Default Response
 * @param seq_num - Sequence Number
 * @return zb_status_t - Status
 */
zb_status_t zb_zcl_ss_send_ias_ace_panel_status_changed_cmd(
    uint8_t src_ep, s_zb_af_address_t *dst_addr,
    s_zb_zcl_ace_panel_status_changed_t *cmd,
    uint8_t disable_default_rsp, uint8_t seq_num)
{
    uint8_t buf[PAYLOAD_LEN_PANEL_STATUS_CHANGED];
    buf[0] = cmd->panel_status;
    buf[1] = cmd->seconds_remaining;
    buf[2] = cmd->audible_notification;
    buf[3] = cmd->alarm_status;

    return zb_zcl_send_cmd(
                src_ep, dst_addr, ZCL_CLUSTER_ID_SS_IAS_ACE,
                COMMAND_IASACE_PANEL_STATUS_CHANGED, true,
                ZCL_FRAME_SERVER_CLIENT_DIR, disable_default_rsp, 0,
                seq_num, PAYLOAD_LEN_PANEL_STATUS_CHANGED, buf);
}

/**
 * @brief Send IAS ACE Get Panel Status Response Command
 * 
 * @param src_ep - Source Endpoint
 * @param dst_addr - Destination Address
 * @param rsp - Response structure
 * @param disable_default_rsp - Disable Default Response
 * @param seq_num - Sequence Number
 * @return zb_status_t - Status
 */
zb_status_t zb_zcl_ss_send_ias_ace_get_panel_status_response_cmd(
    uint8_t src_ep, s_zb_af_address_t *dst_addr,
    s_zb_zcl_ace_panel_status_rsp_t *rsp,
    uint8_t disable_default_rsp, uint8_t seq_num)
{
    uint8_t buf[PAYLOAD_LEN_GET_PANEL_STATUS_RESPONSE];
    buf[0] = rsp->panel_status;
    buf[1] = rsp->seconds_remaining;
    buf[2] = rsp->audible_notification;
    buf[3] = rsp->alarm_status;
    return zb_zcl_send_cmd(
                src_ep, dst_addr, ZCL_CLUSTER_ID_SS_IAS_ACE,
                COMMAND_IASACE_GET_PANEL_STATUS_RESPONSE, true,
                ZCL_FRAME_SERVER_CLIENT_DIR, disable_default_rsp, 0,
                seq_num, PAYLOAD_LEN_GET_PANEL_STATUS_RESPONSE, buf);
}

/**
 * @brief Send IAS ACE Set Bypassed Zone List Command
 * 
 * @param src_ep - Source Endpoint
 * @param dst_addr - Destination Address
 * @param cmd - Set Bypassed Zone List Command
 * @param disable_default_rsp - Disable Default Response
 * @param seq_num - Sequence Number
 * @return zb_status_t - Status
 */
zb_status_t zb_zcl_ss_send_ias_ace_set_bypassed_zone_list_cmd(
    uint8_t src_ep, s_zb_af_address_t *dst_addr,
    s_zb_zcl_ace_set_bypassed_zone_list_t *cmd,
    uint8_t disable_default_rsp, uint8_t seq_num)
{
    uint8_t *buf;
    uint8_t *pbuf;
    uint8_t len = 1 + cmd->number_of_zones;
    zb_status_t status;

    buf = ZB_MEM_MALLOC(len);
    if (buf)
    {
        pbuf = buf;
        *pbuf++ = cmd->number_of_zones;
        memcpy(pbuf, cmd->zone_id, cmd->number_of_zones);

        status = zb_zcl_send_cmd(
                    src_ep, dst_addr, ZCL_CLUSTER_ID_SS_IAS_ACE,
                    COMMAND_IASACE_SET_BYPASSED_ZONE_LIST, true,
                    ZCL_FRAME_SERVER_CLIENT_DIR, disable_default_rsp, 0,
                    seq_num, len, buf);
        ZB_MEM_FREE(buf);
        return status;
    }
    else
    {
        status = ZB_MEM_ERROR;
    }
    return status;
}

/**
 * @brief Send IAS ACE Bypass Response Command
 * 
 * @param src_ep - Source Endpoint
 * @param dst_addr - Destination Address
 * @param rsp - Response structure
 * @param disable_default_rsp - Disable Default Response
 * @param seq_num - Sequence Number
 * @return zb_status_t - Status
 */
zb_status_t zb_zcl_ss_send_ias_ace_bypass_response_cmd(
    uint8_t src_ep, s_zb_af_address_t *dst_addr,
    s_zb_zcl_ace_bypass_rsp_t *rsp,
    uint8_t disable_default_rsp, uint8_t seq_num)
{
    uint8_t *buf;
    uint8_t *pbuf;
    uint8_t len = 1 + rsp->number_of_zones;
    zb_status_t status;

    buf = ZB_MEM_MALLOC(len);
    if (buf)
    {
        pbuf = buf;
        *pbuf++ = rsp->number_of_zones;
        memcpy(pbuf, rsp->bypass_result, rsp->number_of_zones);

        status = zb_zcl_send_cmd(
                    src_ep, dst_addr, ZCL_CLUSTER_ID_SS_IAS_ACE,
                    COMMAND_IASACE_BYPASS_RESPONSE, true,
                    ZCL_FRAME_SERVER_CLIENT_DIR, disable_default_rsp, 0,
                    seq_num, len, buf);
        ZB_MEM_FREE(buf);
        return status;
    }
    else
    {
        status = ZB_MEM_ERROR;
    }
    return status;
}

/**
 * @brief Send IAS ACE Get Zone Status Response Command
 * 
 * @param src_ep - Source Endpoint
 * @param dst_addr - Destination Address
 * @param rsp - Response structure
 * @param disable_default_rsp - Disable Default Response
 * @param seq_num - Sequence Number
 * @return zb_status_t - Status
 */
zb_status_t zb_zcl_ss_send_ias_ace_get_zone_status_response_cmd(
    uint8_t src_ep, s_zb_af_address_t *dst_addr,
    s_zb_zcl_ace_get_zone_status_rsp_t *rsp,
    uint8_t disable_default_rsp, uint8_t seq_num)
{
    uint8_t *buf;
    uint8_t *pbuf;
    zb_status_t status;
    // zone_status_complete (1 byte) + number_of_zones (1 byte) + zone_info (2 bytes per zone)
    uint8_t len = 2 + (rsp->number_of_zones * sizeof(s_zb_zcl_ace_zone_status_t));

    buf = ZB_MEM_MALLOC(len);
    if (buf)
    {
        pbuf = buf;
        *pbuf++ = rsp->zone_status_complete;
        *pbuf++ = rsp->number_of_zones;
        memcpy(pbuf, rsp->zone_info, len - 2);

        status = zb_zcl_send_cmd(
                    src_ep, dst_addr, ZCL_CLUSTER_ID_SS_IAS_ACE,
                    COMMAND_IASACE_GET_ZONE_STATUS_RESPONSE, true,
                    ZCL_FRAME_SERVER_CLIENT_DIR, disable_default_rsp, 0,
                    seq_num, len, buf);
        ZB_MEM_FREE(buf);
    }
    else
    {
        status = ZB_MEM_ERROR;
    }
    return status;
}

static zb_status_t
zcl_ss_handle_incoming(s_zb_zcl_incoming_msg_t *msg)
{
    zb_status_t status = ZB_SUCCESS;

    if (ZCL_CLUSTER_CMD(msg->hdr.fc.type))
    {
        // Is this a manufacturer specific command?
        if (msg->hdr.fc.manu_specific == 0)
        {
            status = zcl_ss_handle_in_specific_commands(msg);
        }
        else
        {
            // We don't support any manufacturer specific commands
            status = ZB_FAILURE;
        }
    }
    else
    {
        // Handle all the normal commands (Read, Write, etc.)
        status = ZB_FAILURE;
    }
    return status;
}

static zb_status_t
zcl_ss_handle_in_specific_commands(s_zb_zcl_incoming_msg_t *msg)
{
    zb_status_t status;
    s_zb_zcl_ss_app_callbacks_t *cb;

    cb = zcl_ss_find_app_callbacks(msg->msg->dst_endpoint);
    if (cb == NULL)
    {
        return ZB_FAILURE;
    }

    switch (msg->msg->cluster_id)
    {
        case ZCL_CLUSTER_ID_SS_IAS_ZONE:
            if (ZCL_CLIENT_CMD(msg->hdr.fc.direction))
            {
                status = zcl_ss_process_in_zone_status_cmds_client(msg, cb);
            }
            else
            {
                status = ZB_FAILURE;
            }
            break;
        case ZCL_CLUSTER_ID_SS_IAS_ACE:
            if (ZCL_CLIENT_CMD(msg->hdr.fc.direction))
            {
                status = zcl_ss_process_in_ace_cmds_client(msg, cb);
            }
            else
            {
                status = zcl_ss_process_in_ace_cmds_server(msg, cb);
            }
            break;
        default:
            status = ZB_FAILURE;
            break;
    }
    return status;
}

static s_zb_zcl_ss_app_callbacks_t*
zcl_ss_find_app_callbacks(uint8_t endpoint)
{
    s_zb_zcl_ss_cb_rec_t *pcb;

    pcb = g_zcl_ss_cb_list;
    while (pcb)
    {
        if (pcb->endpoint == endpoint)
        {
            return pcb->cb;
        }
        pcb = pcb->next;
    }
    return NULL;
}

static zb_status_t
zcl_ss_process_in_zone_status_cmds_client(
    s_zb_zcl_incoming_msg_t *msg, s_zb_zcl_ss_app_callbacks_t *cb)
{
    zb_status_t status;

    switch (msg->hdr.command_id)
    {
        case COMMAND_IAS_ZONE_ZONE_STATUS_CHANGE_NOTIFICATION:
            status = zcl_ss_process_in_zone_status_change_notification(msg, cb);
            break;
        case COMMAND_IAS_ZONE_ZONE_ENROLL_REQUEST:
            status = zcl_ss_process_in_zone_status_enroll_request(msg, cb);
            break;
        default:
            status = ZB_FAILURE;
            break;
    }
    return status;
}

static zb_status_t
zcl_ss_process_in_ace_cmds_server(
    s_zb_zcl_incoming_msg_t *msg, s_zb_zcl_ss_app_callbacks_t *cb)
{
    zb_status_t status;

    switch (msg->hdr.command_id)
    {
        case COMMAND_IASACE_ARM:
            status = zcl_ss_process_in_cmd_ace_arm(msg, cb);
            break;
        case COMMAND_IASACE_BYPASS:
            status = zcl_ss_process_in_cmd_ace_bypass(msg, cb);
            break;
        case COMMAND_IASACE_EMERGENCY:
            status = zcl_ss_process_in_cmd_ace_emergency(msg, cb);
            break;
        case COMMAND_IASACE_FIRE:
            status = zcl_ss_process_in_cmd_ace_fire(msg, cb);
            break;
        case COMMAND_IASACE_PANIC:
            status = zcl_ss_process_in_cmd_ace_panic(msg, cb);
            break;
        case COMMAND_IASACE_GET_ZONE_ID_MAP:
            status = zcl_ss_process_in_cmd_ace_get_zone_id_map(msg, cb);
            break;
        case COMMAND_IASACE_GET_ZONE_INFORMATION:
            status = zcl_ss_process_in_cmd_ace_get_zone_information(msg, cb);
            break;
        case COMMAND_IASACE_GET_PANEL_STATUS:
            status = zcl_ss_process_in_cmd_ace_get_panel_status(msg, cb);
            break;
        case COMMAND_IASACE_GET_BYPASSED_ZONE_LIST:
            status = zcl_ss_process_in_cmd_ace_get_bypassed_zone_list(msg, cb);
            break;
        case COMMAND_IASACE_GET_ZONE_STATUS:
            status = zcl_ss_process_in_cmd_ace_get_zone_status(msg, cb);
            break;
        default:
            status = ZB_FAILURE;
            break;
    }
    return status;
}

static zb_status_t
zcl_ss_process_in_ace_cmds_client(
    s_zb_zcl_incoming_msg_t *msg, s_zb_zcl_ss_app_callbacks_t *cb)
{
    zb_status_t status;

    switch (msg->hdr.command_id)
    {
        case COMMAND_IASACE_ARM_RESPONSE:
            status = zcl_ss_process_in_cmd_ace_arm_response(msg, cb);
            break;
        case COMMAND_IASACE_GET_ZONE_ID_MAP_RESPONSE:
            status = zcl_ss_process_in_cmd_ace_get_zone_id_map_response(msg, cb);
            break;
        case COMMAND_IASACE_GET_ZONE_INFORMATION_RESPONSE:
            status = zcl_ss_process_in_cmd_ace_get_zone_information_response(msg, cb);
            break;
        case COMMAND_IASACE_ZONE_STATUS_CHANGED:
            status = zcl_ss_process_in_cmd_ace_zone_status_changed(msg, cb);
            break;
        case COMMAND_IASACE_PANEL_STATUS_CHANGED:
            status = zcl_ss_process_in_cmd_ace_panel_status_changed(msg, cb);
            break;
        case COMMAND_IASACE_GET_PANEL_STATUS_RESPONSE:
            status = zcl_ss_process_in_cmd_ace_get_panel_status_response(msg, cb);
            break;
        case COMMAND_IASACE_SET_BYPASSED_ZONE_LIST:
            status = zcl_ss_process_in_cmd_ace_set_bypassed_zone_list(msg, cb);
            break;
        case COMMAND_IASACE_BYPASS_RESPONSE:
            status = zcl_ss_process_in_cmd_ace_bypass_response(msg, cb);
            break;
        case COMMAND_IASACE_GET_ZONE_STATUS_RESPONSE:
            status = zcl_ss_process_in_cmd_ace_get_zone_status_response(msg, cb);
            break;
        default:
            status = ZB_FAILURE;
            break;
    }
    return status;
}

// ZCL IAS Zone Helper Functions
static zb_status_t
zcl_ss_add_zone(uint8_t endpoint, s_zb_zcl_ias_ace_zone_table_t *zone)
{
    s_zb_zcl_ss_zone_item_t *p_new_item;
    s_zb_zcl_ss_zone_item_t *p_loop;

    p_new_item = (s_zb_zcl_ss_zone_item_t *)ZB_MEM_MALLOC(sizeof(s_zb_zcl_ss_zone_item_t));
    if (p_new_item == NULL)
    {
        return ZB_MEM_ERROR;
    }
    p_new_item->next = NULL;
    p_new_item->endpoint = endpoint;
    memcpy((void *)&p_new_item->zone, (void *)zone, sizeof(s_zb_zcl_ias_ace_zone_table_t));

    if (g_zcl_ss_zone_list == NULL)
    {
        g_zcl_ss_zone_list = p_new_item;
    }
    else
    {
        p_loop = g_zcl_ss_zone_list;
        while (p_loop->next != NULL)
        {
        p_loop = p_loop->next;
        }
        p_loop->next = p_new_item;
    }
    return ZB_SUCCESS;
}

uint8_t
zcl_ss_count_all_zones(void)
{
    s_zb_zcl_ss_zone_item_t *p_loop;
    uint8_t count = 0;
    p_loop = g_zcl_ss_zone_list;
    while (p_loop != NULL)
    {
        count++;
        p_loop = p_loop->next;
    }
    return count;
}

static uint8_t
zcl_ss_zone_id_available(uint8_t zone_id)
{
    s_zb_zcl_ss_zone_item_t *p_loop;

    if (zone_id < ZCL_SS_MAX_ZONE_ID)
    {
        p_loop = g_zcl_ss_zone_list;
        while (p_loop != NULL)
        {
            if (p_loop->zone.zone_id == zone_id)
            {
                return false;
            }
            p_loop = p_loop->next;
        }
        return true;
    }
    return false;
}

static uint8_t
zcl_ss_get_next_free_zone_id(void)
{
    static uint8_t next_available_zone_id = 0;

    if (zcl_ss_zone_id_available(next_available_zone_id) == false)
    {
        uint8_t zone_id = next_available_zone_id;

        do
        {
            if (++zone_id > ZCL_SS_MAX_ZONE_ID)
            {
            zone_id = 0;
            }
        } while ((zone_id != next_available_zone_id) && (zcl_ss_zone_id_available(zone_id) == false));
        if (zone_id != next_available_zone_id)
        {
            next_available_zone_id = zone_id;
        }
        else
        {
            return ZCL_SS_MAX_ZONE_ID + 1;
        }
    }
    return next_available_zone_id;
}

s_zb_zcl_ias_ace_zone_table_t *
zb_zcl_ss_find_zone(uint8_t endpoint, uint8_t zone_id)
{
    s_zb_zcl_ss_zone_item_t *p_loop;
    p_loop = g_zcl_ss_zone_list;
    while (p_loop != NULL)
    {
        if (p_loop->endpoint == endpoint && p_loop->zone.zone_id == zone_id)
        {
            return &p_loop->zone;
        }
        p_loop = p_loop->next;
    }
    return NULL;
}

uint8_t
zb_zcl_ss_remove_zone(uint8_t endpoint, uint8_t zone_id)
{
    s_zb_zcl_ss_zone_item_t *p_loop;
    s_zb_zcl_ss_zone_item_t *p_prev;
    p_loop = g_zcl_ss_zone_list;
    p_prev = NULL;

    while (p_loop != NULL)
    {
        if (p_loop->endpoint == endpoint && p_loop->zone.zone_id == zone_id)
        {
            if (p_prev == NULL)
            {
                g_zcl_ss_zone_list = p_loop->next;
            }
            else
            {
                p_prev->next = p_loop->next;
            }
            ZB_MEM_FREE(p_loop);
            return true;
        }
        p_prev = p_loop;
        p_loop = p_loop->next;
    }
    return false;
}

void
zb_zcl_ss_update_zone_address(uint8_t endpoint, uint8_t zone_id, uint64_t ieee_addr)
{
    s_zb_zcl_ias_ace_zone_table_t *p_zone;
    p_zone = zb_zcl_ss_find_zone(endpoint, zone_id);
    if (p_zone != NULL)
    {
        p_zone->zone_address = ieee_addr;
    }
}

static zb_status_t
zcl_ss_process_in_zone_status_change_notification(
    s_zb_zcl_incoming_msg_t *msg, s_zb_zcl_ss_app_callbacks_t *cb)
{
    if (cb->pfn_zone_change_notification)
    {
        s_zb_zcl_ss_zone_change_notification_t noti;

        noti.zone_status = BUILD_UINT16(msg->data[0], msg->data[1]);
        noti.extended_status = msg->data[2];
        noti.zone_id = msg->data[3];
        noti.delay = BUILD_UINT16(msg->data[4], msg->data[5]);

        return cb->pfn_zone_change_notification(&noti, &msg->msg->src_addr);
    }
    return ZB_FAILURE;
}

static zb_status_t
zcl_ss_process_in_zone_status_enroll_request(
    s_zb_zcl_incoming_msg_t *msg, s_zb_zcl_ss_app_callbacks_t *cb)
{
    s_zb_zcl_ias_ace_zone_table_t zone;
    zb_status_t status = ZB_FAILURE;
    uint16_t zone_type;
    uint16_t manu_code;
    uint8_t response_code;
    uint8_t zone_id;

    zone_id = zcl_ss_get_next_free_zone_id();
    zone_type = BUILD_UINT16(msg->data[0], msg->data[1]);
    manu_code = BUILD_UINT16(msg->data[2], msg->data[3]);

    if (ZCL_SS_ZONE_TYPE_SUPPORT(zone_type))
    {
        if ((zcl_ss_count_all_zones() < ZCL_SS_MAX_ZONE_ID - 1) && (zone_id <= ZCL_SS_MAX_ZONE_ID))
        {
            zone.zone_id = zone_id;
            zone.zone_type = zone_type;
            zone.zone_address = g_zcl_ss_unknown_ieee_address;
            if (zcl_ss_add_zone(msg->msg->src_endpoint, &zone) == ZB_SUCCESS)
            {
                response_code = ZB_SUCCESS;
            }
            else
            {
                // CIE does not permit new zones to enroll at this time
                response_code = SS_IAS_ZONE_STATUS_ENROLL_RESPONSE_CODE_NO_ENROLL_PERMIT;
            }
        }
        else
        {
            // CIE has reached its limit of number of enrolled zones
            response_code = SS_IAS_ZONE_STATUS_ENROLL_RESPONSE_CODE_TOO_MANY_ZONES;
        }
    }
    else
    {
        // CIE does not support this zone type
        response_code = SS_IAS_ZONE_STATUS_ENROLL_RESPONSE_CODE_NOT_SUPPORTED;
    }
    if (cb->pfn_zone_enroll_request)
    {
        s_zb_zcl_ss_zone_enroll_request_t req;

        req.src_addr = &msg->msg->src_addr;
        req.zone_id = zone_id;
        req.zone_type = zone_type;
        req.manufacturer_code = manu_code;
        status = cb->pfn_zone_enroll_request(&req, msg->msg->src_endpoint);
    }
    if (status == ZB_SUCCESS)
    {
        // Send a response back
        status = zb_zcl_ss_ias_send_zone_status_enroll_response_cmd(msg->msg->src_endpoint,
            &msg->msg->src_addr, response_code, zone_id, true, msg->hdr.trans_seq_num);
        return ZCL_STATUS_CMD_HAS_RSP;
    }
    else
    {
        return status;
    }
}

// ZCL ACE Cluster helper functions
static uint8_t
zcl_ss_parse_utf8_string(uint8_t *buf, s_utf8_string_t *str, uint8_t max_len)
{
    uint8_t original_len = 0;

    str->str_len = *buf++;
    if (str->str_len == 0xFF)
    {
        str->str_len = 0;
    }

    if (str->str_len != 0)
    {
        original_len = str->str_len;

        if (str->str_len > max_len)
        {
            str->str_len = max_len;
        }
        str->pstr = buf;
    }
    else
    {
        str->pstr = NULL;
    }

    return original_len + 1; // this is including the str_len field
}

/**
 * @brief Parse received IAS ACE Arm Command
 * 
 * @param cmd - Command structure
 * @param data - Data buffer
 * @return zb_status_t - Status
 */
zb_status_t
zcl_ss_parse_in_cmd_ace_arm(s_zb_zcl_ace_arm_t *cmd, uint8_t *data)
{
    uint8_t field_len;

    cmd->arm_mode = *data++;

    field_len = zcl_ss_parse_utf8_string(data, &cmd->arm_disarm_code, ARM_DISARM_CODE_LEN);
    data += field_len;

    cmd->zone_id = *data++;

    return ZB_SUCCESS;
}

/**
 * @brief Parse received IAS ACE Bypass Command
 * 
 * @param cmd - Command structure
 * @param data - Data buffer
 * @return zb_status_t - Status
 */
zb_status_t
zcl_ss_parse_in_cmd_ace_bypass(s_zb_zcl_ace_bypass_t *cmd, uint8_t *data)
{
    cmd->number_of_zones = *data++;
    cmd->bypass_buf = data;

    data += cmd->number_of_zones;

    zcl_ss_parse_utf8_string(data, &cmd->arm_disarm_code, ARM_DISARM_CODE_LEN);

    return ZB_SUCCESS;
}

static zb_status_t
zcl_ss_parse_in_cmd_ace_get_zone_information_response(
    s_zb_zcl_ace_get_zone_info_rsp_t *cmd, uint8_t *data)
{
    cmd->zone_id = *data++;

    cmd->zone_type = BUILD_UINT16(data[0], data[1]);
    data += 2;

    cmd->ieee_addr = *(uint64_t *)data;
    data += 8;

    zcl_ss_parse_utf8_string(data, &cmd->zone_label, ZONE_LABEL_LEN);

    return ZB_SUCCESS;
}

static zb_status_t
zcl_ss_parse_in_cmd_ace_zone_status_changed(
    s_zb_zcl_ace_zone_status_changed_t *cmd, uint8_t *data)
{
    cmd->zone_id = *data++;
    cmd->zone_status = BUILD_UINT16(data[0], data[1]);
    data += 2;
    cmd->audible_notification = *data++;
    zcl_ss_parse_utf8_string(data, &cmd->zone_label, ZONE_LABEL_LEN);
    return ZB_SUCCESS;
}

/**
 * @brief Process received IAS ACE Arm Command
 * 
 * @param msg - Incoming message
 * @param cb - Callback function
 * @return zb_status_t - Status
 */
static zb_status_t
zcl_ss_process_in_cmd_ace_arm(
    s_zb_zcl_incoming_msg_t *msg, s_zb_zcl_ss_app_callbacks_t *cb)
{
    zb_status_t status = ZB_FAILURE;

    if (cb->pfn_ace_arm)
    {
        s_zb_zcl_ace_arm_t cmd;
        uint8_t arm_notification = 0xFF;

        if (zcl_ss_parse_in_cmd_ace_arm(&cmd, msg->data) == ZB_SUCCESS)
        {
            arm_notification = cb->pfn_ace_arm(&cmd);

            if (arm_notification != 0xFF)
            {
                status = zb_zcl_ss_send_ias_ace_arm_response_cmd(
                            msg->msg->src_endpoint, &msg->msg->src_addr,
                            arm_notification, true, msg->hdr.trans_seq_num);
                return ZCL_STATUS_CMD_HAS_RSP;
            }
            else
            {
                status = ZCL_STATUS_INVALID_VALUE;
            }
        }
    }
    return status;
}

/**
 * @brief Process received IAS ACE Bypass Command
 * 
 * @param msg - Incoming message
 * @param cb - Callback function
 * @return zb_status_t - Status
 */
static zb_status_t
zcl_ss_process_in_cmd_ace_bypass(
    s_zb_zcl_incoming_msg_t *msg, s_zb_zcl_ss_app_callbacks_t *cb)
{
    if (cb->pfn_ace_bypass)
    {
        s_zb_zcl_ace_bypass_t cmd;

        if (zcl_ss_parse_in_cmd_ace_bypass(&cmd, msg->data) == ZB_SUCCESS)
        {
            return cb->pfn_ace_bypass(&cmd);
        }
    }
    return ZB_FAILURE;
}


/**
 * @brief Process received IAS ACE Emergency Command
 * 
 * @param msg - Incoming message
 * @param cb - Callback function
 * @return zb_status_t - Status
 */
static zb_status_t
zcl_ss_process_in_cmd_ace_emergency(
    s_zb_zcl_incoming_msg_t *msg, s_zb_zcl_ss_app_callbacks_t *cb)
{
    if (cb->pfn_ace_emergency)
    {
        return cb->pfn_ace_emergency();
    }
    return ZB_FAILURE;
}

/**
 * @brief Process received IAS ACE Fire Command
 * 
 * @param msg - Incoming message
 * @param cb - Callback function
 * @return zb_status_t - Status
 */
static zb_status_t
zcl_ss_process_in_cmd_ace_fire(
    s_zb_zcl_incoming_msg_t *msg, s_zb_zcl_ss_app_callbacks_t *cb)
{
    if (cb->pfn_ace_fire)
    {
        return cb->pfn_ace_fire();
    }
    return ZB_FAILURE;
}

/**
 * @brief Process received IAS ACE Panic Command
 * 
 * @param msg - Incoming message
 * @param cb - Callback function
 * @return zb_status_t - Status
 */
static zb_status_t
zcl_ss_process_in_cmd_ace_panic(
    s_zb_zcl_incoming_msg_t *msg, s_zb_zcl_ss_app_callbacks_t *cb)
{
    if (cb->pfn_ace_panic)
    {
        return cb->pfn_ace_panic();
    }
    return ZB_FAILURE;
}

/**
 * @brief Process received IAS ACE Get Zone ID Map Command
 * 
 * @param msg - Incoming message
 * @param cb - Callback function
 * @return zb_status_t - Status
 */
static zb_status_t
zcl_ss_process_in_cmd_ace_get_zone_id_map(
    s_zb_zcl_incoming_msg_t *msg, s_zb_zcl_ss_app_callbacks_t *cb)
{
    zb_status_t status = ZB_FAILURE;
    uint16_t zone_id_map[ZONE_ID_MAP_ARRAY_SIZE];
    uint16_t map_selection;
    uint8_t zone_id;

    for (uint8_t i = 0; i < ZONE_ID_MAP_ARRAY_SIZE; i++)
    {
        map_selection = 0;

        for (uint8_t j = 0; j < ZONE_ID_MAP_ARRAY_SIZE; j++)
        {
            zone_id = ZONE_ID_MAP_ARRAY_SIZE * i + j;
            if (zb_zcl_ss_find_zone(msg->msg->dst_endpoint, zone_id) != NULL)
            {
                map_selection |= (1 << j);
            }
        }
        zone_id_map[i] = map_selection;
    }

    if (cb->pfn_ace_get_zone_id_map)
    {
        status = cb->pfn_ace_get_zone_id_map();
    }
    if (status == ZB_SUCCESS)
    {
        status = zb_zcl_ss_send_ias_ace_get_zone_id_map_response_cmd(
                    msg->msg->dst_endpoint, &msg->msg->src_addr, zone_id_map,
                    true, msg->hdr.trans_seq_num);
        return ZCL_STATUS_CMD_HAS_RSP;
    }
    return status;
}

/**
 * @brief Process received IAS ACE Get Zone Information Command
 * 
 * @param msg - Incoming message
 * @param cb - Callback function
 * @return zb_status_t - Status
 */
static zb_status_t
zcl_ss_process_in_cmd_ace_get_zone_information(
    s_zb_zcl_incoming_msg_t *msg, s_zb_zcl_ss_app_callbacks_t *cb)
{
    zb_status_t status = ZB_FAILURE;

    if (cb->pfn_ace_get_zone_information)
    {
        // the callback function shall take care of sending
        // Get Zone Information Response command
        status = cb->pfn_ace_get_zone_information(msg);
    }
    if (status == ZB_SUCCESS)
    {
        return ZCL_STATUS_CMD_HAS_RSP;
    }
    return status;;
}

/**
 * @brief Process received IAS ACE Get Panel Status Command
 * 
 * @param msg - Incoming message
 * @param cb - Callback function
 * @return zb_status_t - Status
 */
static zb_status_t
zcl_ss_process_in_cmd_ace_get_panel_status(
    s_zb_zcl_incoming_msg_t *msg, s_zb_zcl_ss_app_callbacks_t *cb)
{
    zb_status_t status = ZB_FAILURE;

    if (cb->pfn_ace_get_panel_status)
    {
        // the callback function shall take care of sending
        // Get Panel Status Response command
        status = cb->pfn_ace_get_panel_status(msg);
    }
    if (status == ZB_SUCCESS)
    {
        return ZCL_STATUS_CMD_HAS_RSP;
    }
    return status;
}

/**
 * @brief Process received IAS ACE Get Bypassed Zone List Command
 * 
 * @param msg - Incoming message
 * @param cb - Callback function
 * @return zb_status_t - Status
 */
static zb_status_t
zcl_ss_process_in_cmd_ace_get_bypassed_zone_list(
    s_zb_zcl_incoming_msg_t *msg, s_zb_zcl_ss_app_callbacks_t *cb)
{
    zb_status_t status = ZB_FAILURE;

    if (cb->pfn_ace_get_bypassed_zone_list)
    {
        // the callback function shall take care of sending
        // Get Bypassed Zone List Response command
        status = cb->pfn_ace_get_bypassed_zone_list(msg);
    }
    if (status == ZB_SUCCESS)
    {
        return ZCL_STATUS_CMD_HAS_RSP;
    }
    return status;
}

/**
 * @brief Process received IAS ACE Get Zone Status Command
 * 
 * @param msg - Incoming message
 * @param cb - Callback function
 * @return zb_status_t - Status
 */
static zb_status_t
zcl_ss_process_in_cmd_ace_get_zone_status(
    s_zb_zcl_incoming_msg_t *msg, s_zb_zcl_ss_app_callbacks_t *cb)
{
    zb_status_t status = ZB_FAILURE;

    if (cb->pfn_ace_get_zone_status)
    {
        // the callback function shall take care of sending
        // Get Zone Status Response command
        status = cb->pfn_ace_get_zone_status(msg);
    }
    if (status == ZB_SUCCESS)
    {
        return ZCL_STATUS_CMD_HAS_RSP;
    }
    return status;
}

/**
 * @brief Process received IAS ACE Arm Response Command
 * 
 * @param msg - Incoming message
 * @param cb - Callback function
 * @return zb_status_t - Status
 */
static zb_status_t
zcl_ss_process_in_cmd_ace_arm_response(
    s_zb_zcl_incoming_msg_t *msg, s_zb_zcl_ss_app_callbacks_t *cb)
{
    if (cb->pfn_ace_arm_response)
    {
        return cb->pfn_ace_arm_response(msg->data[0]);
    }
    return ZB_FAILURE;
}

/**
 * @brief Process received IAS ACE Get Zone ID Map Response Command
 * 
 * @param msg - Incoming message
 * @param cb - Callback function
 * @return zb_status_t - Status
 */
static zb_status_t
zcl_ss_process_in_cmd_ace_get_zone_id_map_response(
    s_zb_zcl_incoming_msg_t *msg, s_zb_zcl_ss_app_callbacks_t *cb)
{
    uint16_t *buf;
    uint16_t *pbuf;
    uint8_t *pdata;
    uint8_t len = 32; // 16 fields of 2 bytes each

    buf = ZB_MEM_MALLOC(len);
    if (buf == NULL)
    {
        return ZB_MEM_ERROR;
    }
    pbuf = buf;
    pdata = msg->data;

    for (uint8_t i = 0; i < ZONE_ID_MAP_ARRAY_SIZE; i++)
    {
        *pbuf++ = BUILD_UINT16(pdata[0], pdata[1]);
        pdata += 2;
    }

    if (cb->pfn_ace_get_zone_id_map_response)
    {
        zb_status_t status = cb->pfn_ace_get_zone_id_map_response(buf);
        ZB_MEM_FREE(buf);
        return status;
    }
    ZB_MEM_FREE(buf);
    return ZB_FAILURE;
}

/**
 * @brief Process received IAS ACE Get Zone Information Response Command
 * 
 * @param msg - Incoming message
 * @param cb - Callback function
 * @return zb_status_t - Status
 */
static zb_status_t
zcl_ss_process_in_cmd_ace_get_zone_information_response(s_zb_zcl_incoming_msg_t *msg, s_zb_zcl_ss_app_callbacks_t *cb)
{
    if (cb->pfn_ace_get_zone_information_response)
    {
        s_zb_zcl_ace_get_zone_info_rsp_t cmd;

        if (zcl_ss_parse_in_cmd_ace_get_zone_information_response(&cmd, msg->data) == ZB_SUCCESS)
        {
            return cb->pfn_ace_get_zone_information_response(&cmd);
        }
    }
    return ZB_FAILURE;
}

/**
 * @brief Process received IAS ACE Zone Status Changed Command
 * 
 * @param msg - Incoming message
 * @param cb - Callback function
 * @return zb_status_t - Status
 */
static zb_status_t
zcl_ss_process_in_cmd_ace_zone_status_changed(s_zb_zcl_incoming_msg_t *msg, s_zb_zcl_ss_app_callbacks_t *cb)
{
    if (cb->pfn_ace_zone_status_changed)
    {
        s_zb_zcl_ace_zone_status_changed_t cmd;

        if (zcl_ss_parse_in_cmd_ace_zone_status_changed(&cmd, msg->data) == ZB_SUCCESS)
        {
            return cb->pfn_ace_zone_status_changed(&cmd);
        }
    }
    return ZB_FAILURE;
}

/**
 * @brief Process received IAS ACE Panel Status Changed Command
 * 
 * @param msg - Incoming message
 * @param cb - Callback function
 * @return zb_status_t - Status
 */
static zb_status_t
zcl_ss_process_in_cmd_ace_panel_status_changed(s_zb_zcl_incoming_msg_t *msg, s_zb_zcl_ss_app_callbacks_t *cb)
{
    if (cb->pfn_ace_panel_status_changed)
    {
        s_zb_zcl_ace_panel_status_changed_t cmd;

        cmd.panel_status = msg->data[0];
        cmd.seconds_remaining = msg->data[1];
        cmd.audible_notification = msg->data[2];
        cmd.alarm_status = msg->data[3];

        return cb->pfn_ace_panel_status_changed(&cmd);
    }
    return ZB_FAILURE;
}

/**
 * @brief Process received IAS ACE Get Panel Status Response Command
 * 
 * @param msg - Incoming message
 * @param cb - Callback function
 * @return zb_status_t - Status
 */
static zb_status_t
zcl_ss_process_in_cmd_ace_get_panel_status_response(s_zb_zcl_incoming_msg_t *msg, s_zb_zcl_ss_app_callbacks_t *cb)
{
    if (cb->pfn_ace_get_panel_status_response)
    {
        s_zb_zcl_ace_panel_status_rsp_t cmd;

        cmd.panel_status = msg->data[0];
        cmd.seconds_remaining = msg->data[1];
        cmd.audible_notification = msg->data[2];
        cmd.alarm_status = msg->data[3];

        return cb->pfn_ace_get_panel_status_response(&cmd);
    }
    return ZB_FAILURE;
}

/**
 * @brief Process received IAS ACE Set Bypassed Zone List Command
 * 
 * @param msg - Incoming message
 * @param cb - Callback function
 * @return zb_status_t - Status
 */
static zb_status_t
zcl_ss_process_in_cmd_ace_set_bypassed_zone_list(s_zb_zcl_incoming_msg_t *msg, s_zb_zcl_ss_app_callbacks_t *cb)
{
    if (cb->pfn_ace_set_bypassed_zone_list)
    {
        s_zb_zcl_ace_set_bypassed_zone_list_t cmd;

        cmd.number_of_zones = msg->data[0];
        cmd.zone_id = &msg->data[1];

        return cb->pfn_ace_set_bypassed_zone_list(&cmd);
    }
    return ZB_FAILURE;
}

/**
 * @brief Process received IAS ACE Bypass Response Command
 * 
 * @param msg - Incoming message
 * @param cb - Callback function
 * @return zb_status_t - Status
 */
static zb_status_t
zcl_ss_process_in_cmd_ace_bypass_response(s_zb_zcl_incoming_msg_t *msg, s_zb_zcl_ss_app_callbacks_t *cb)
{
    if (cb->pfn_ace_bypass_response)
    {
        s_zb_zcl_ace_bypass_rsp_t cmd;

        cmd.number_of_zones = msg->data[0];
        cmd.bypass_result = &msg->data[1];

        return cb->pfn_ace_bypass_response(&cmd);
    }
    return ZB_FAILURE;
}

/**
 * @brief Process received IAS ACE Get Zone Status Response Command
 * 
 * @param msg - Incoming message
 * @param cb - Callback function
 * @return zb_status_t - Status
 */
static zb_status_t
zcl_ss_process_in_cmd_ace_get_zone_status_response(s_zb_zcl_incoming_msg_t *msg, s_zb_zcl_ss_app_callbacks_t *cb)
{
    if (cb->pfn_ace_get_zone_status_response)
    {
        s_zb_zcl_ace_get_zone_status_rsp_t cmd;

        cmd.zone_status_complete = msg->data[0];
        cmd.number_of_zones = msg->data[1];
        cmd.zone_info = (s_zb_zcl_ace_zone_status_t *)&msg->data[2];

        return cb->pfn_ace_get_zone_status_response(&cmd);
    }
    return ZB_FAILURE;
}