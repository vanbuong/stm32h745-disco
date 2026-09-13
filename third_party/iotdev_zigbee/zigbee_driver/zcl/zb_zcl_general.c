/*
 * zb_zcl_general.c
 *
 * Author: Vo Van Buong (BRT-SG)
 */

#include "aps/zb_aps_group.h"
#include "zcl/zb_zcl.h"
#include "zcl/zb_zcl_general.h"

/***************************************************************************************************
 * Macros
 **************************************************************************************************/

#define TAG "ZCL_GENERAL"

/***************************************************************************************************
 * Typedefs
 **************************************************************************************************/
typedef struct s_zb_zcl_general_cb_rec
{
    struct s_zb_zcl_general_cb_rec *next;
    uint8_t endpoint;
    s_zb_zcl_general_app_callbacks_t *cb;
} s_zb_zcl_general_cb_rec_t;


/***************************************************************************************************
 * Local Variables
 **************************************************************************************************/
static s_zb_zcl_general_cb_rec_t *g_zcl_general_cb_list = NULL;
static uint8_t g_zcl_general_plugin_registered = false;

/***************************************************************************************************
 * Local Functions
 **************************************************************************************************/
static zb_status_t zcl_general_handle_incoming(s_zb_zcl_incoming_msg_t *msg);
static zb_status_t zcl_general_handle_in_specific_commands(s_zb_zcl_incoming_msg_t *msg);
static s_zb_zcl_general_app_callbacks_t *zcl_general_find_app_callbacks(uint8_t endpoint);

// ZCL_BASIC
static zb_status_t zcl_general_process_in_basic(
    s_zb_zcl_incoming_msg_t *msg, s_zb_zcl_general_app_callbacks_t *cb);

// ZCL_IDENTIFY
static zb_status_t zcl_general_process_in_identify(
    s_zb_zcl_incoming_msg_t *msg, s_zb_zcl_general_app_callbacks_t *cb);

// ZCL_GROUPS
static zb_status_t zcl_general_process_in_groups_client(
    s_zb_zcl_incoming_msg_t *msg, s_zb_zcl_general_app_callbacks_t *cb);

// ZCL_SCENES
static zb_status_t zcl_general_process_in_scenes_client(
    s_zb_zcl_incoming_msg_t *msg, s_zb_zcl_general_app_callbacks_t *cb);

// ZCL_ALARMS
static zb_status_t zcl_general_process_in_alarms_client(
    s_zb_zcl_incoming_msg_t *msg, s_zb_zcl_general_app_callbacks_t *cb);


/**
 * @brief Register an applications command callbacks
 * 
 */
zb_status_t
zb_zcl_general_register_command_callbacks(
    uint8_t endpoint, s_zb_zcl_general_app_callbacks_t *callbacks)
{
    s_zb_zcl_general_cb_rec_t *p_new_item;
    s_zb_zcl_general_cb_rec_t *p_loop;

    if (!g_zcl_general_plugin_registered)
    {
        zb_zcl_register_plugin(ZCL_CLUSTER_ID_GENERAL_BASIC,
                                            ZCL_CLUSTER_ID_GENERAL_MULTISTATE_VALUE_BASIC,
                                            zcl_general_handle_incoming);
        g_zcl_general_plugin_registered = true;
    }

    p_new_item = (s_zb_zcl_general_cb_rec_t *)ZB_MEM_MALLOC(sizeof(s_zb_zcl_general_cb_rec_t));
    if (p_new_item == NULL)
    {
        return ZB_MEM_ERROR;
    }
    p_new_item->next = NULL;
    p_new_item->endpoint = endpoint;
    p_new_item->cb = callbacks;

    if (g_zcl_general_cb_list == NULL)
    {
        g_zcl_general_cb_list = p_new_item;
    }
    else
    {
        p_loop = g_zcl_general_cb_list;
        while (p_loop->next != NULL)
        {
            p_loop = p_loop->next;
        }
        p_loop->next = p_new_item;
    }
    return ZB_SUCCESS;
}

// ZCL_IDENTIFY Client commands
/**
 * @brief Call to send out an Identify command
 * 
 * @param src_ep - Sending application's endpoint
 * @param dst_addr - where to send the command to
 * @param identify_time - the time in seconds that the device will identify itself (in seconds)
 * @param disable_default_rsp - if true, the default response will not be sent
 * @param seq_num - the sequence number of the command
 * @return zb_status_t - ZB_SUCCESS if the command was sent successfully, otherwise an error code
 */
zb_status_t
zb_zcl_general_send_identify(
    uint16_t src_ep, s_zb_af_address_t *dst_addr,
    uint16_t identify_time, uint8_t disable_default_rsp, uint8_t seq_num)
{
    uint8_t buf[2] = {0};
    buf[0] = LO_UINT16(identify_time);
    buf[1] = HI_UINT16(identify_time);
    return zb_zcl_send_cmd(
                src_ep, dst_addr, ZCL_CLUSTER_ID_GENERAL_IDENTIFY, COMMAND_IDENTIFY_IDENTIFY,
                true, ZCL_FRAME_CLIENT_SERVER_DIR, disable_default_rsp, 0, seq_num, 2, buf);
}

/**
 * @brief Call to send out an Identify Trigger Effect command
 * 
 * @param src_ep - Sending application's endpoint
 * @param dst_addr - where to send the command to
 * @param effect_id - the effect to use
 * @param effect_variant - the variant of the effect to use
 * @param disable_default_rsp - if true, the default response will not be sent
 * @param seq_num - the sequence number of the command
 * @return zb_status_t - ZB_SUCCESS if the command was sent successfully, otherwise an error code
 */
zb_status_t
zb_zcl_general_send_identify_trigger_effect(
    uint16_t src_ep, s_zb_af_address_t *dst_addr,
    uint8_t effect_id, uint8_t effect_variant,
    uint8_t disable_default_rsp, uint8_t seq_num)
{
    uint8_t buf[2] = {0};
    buf[0] = effect_id;
    buf[1] = effect_variant;
    return zb_zcl_send_cmd(
                src_ep, dst_addr, ZCL_CLUSTER_ID_GENERAL_IDENTIFY, COMMAND_IDENTIFY_TRIGGER_EFFECT,
                true, ZCL_FRAME_CLIENT_SERVER_DIR, disable_default_rsp, 0, seq_num, 2, buf);
}

/**
 * @brief Call to send out an Identify Query Response command
 * 
 * @param src_ep - Sending application's endpoint
 * @param dst_addr - where to send the command to
 * @param timeout - how long the device will continue to identify itself (in seconds)
 * @param disable_default_rsp - if true, the default response will not be sent
 * @param seq_num - the sequence number of the command
 * @return zb_status_t - ZB_SUCCESS if the command was sent successfully, otherwise an error code
 */
zb_status_t
zb_zcl_general_send_identify_query_response(
    uint16_t src_ep, s_zb_af_address_t *dst_addr,
    uint16_t timeout, uint8_t disable_default_rsp, uint8_t seq_num)
{
    uint8_t buf[2] = {0};
    buf[0] = LO_UINT16(timeout);
    buf[1] = HI_UINT16(timeout);
    return zb_zcl_send_cmd(
                src_ep, dst_addr, ZCL_CLUSTER_ID_GENERAL_IDENTIFY, COMMAND_IDENTIFY_IDENTIFY_QUERY_RESPONSE,
                true, ZCL_FRAME_CLIENT_SERVER_DIR, disable_default_rsp, 0, seq_num, 2, buf);
}

// ZCL_GROUPS Client commands
/**
 * @brief Send a Group Request to a device.
 * 
 * @param src_ep - Sending application's endpoint
 * @param dst_addr - where to send the command to
 * @param cmd - the command to send, one of: COMMAND_GROUPS_VIEW_GROUP, COMMAND_GROUPS_REMOVE_GROUP
 * @param group_id - the group ID to send the command to
 * @param disable_default_rsp - if true, the default response will not be sent
 * @param seq_num - the sequence number of the command
 * @return zb_status_t - ZB_SUCCESS if the command was sent successfully, otherwise an error code
 */
zb_status_t
zb_zcl_general_send_groups_request(
    uint16_t src_ep, s_zb_af_address_t *dst_addr,
    uint8_t cmd, uint16_t group_id, uint8_t disable_default_rsp, uint8_t seq_num)
{
    uint8_t buf[2] = {0};
    buf[0] = LO_UINT16(group_id);
    buf[1] = HI_UINT16(group_id);
    return zb_zcl_send_cmd(
                src_ep, dst_addr, ZCL_CLUSTER_ID_GENERAL_GROUPS, cmd,
                true, ZCL_FRAME_CLIENT_SERVER_DIR, disable_default_rsp, 0, seq_num, 2, buf);
}

/**
 * @brief Send a Add Group Request to a device.
 * 
 * @param src_ep - Sending application's endpoint
 * @param dst_addr - where to send the command to
 * @param group_id - the group ID to add
 * @param group_name - the name of the group
 * @param disable_default_rsp - if true, the default response will not be sent
 * @param seq_num - the sequence number of the command
 * @return zb_status_t - ZB_SUCCESS if the command was sent successfully, otherwise an error code
 */
zb_status_t
zb_zcl_general_send_add_group_request(
    uint16_t src_ep, s_zb_af_address_t *dst_addr,
    uint8_t cmd, uint16_t group_id, uint8_t *group_name,
    uint8_t disable_default_rsp, uint8_t seq_num)
{
    uint8_t *buf;
    uint8_t *pbuf;
    uint8_t len;
    zb_status_t status;

    len = 2 + group_name[0] + 1;
    buf = (uint8_t *)ZB_MEM_MALLOC(len);
    if (buf == NULL)
    {
        return ZB_MEM_ERROR;
    }
    pbuf = buf;
    *pbuf++ = LO_UINT16(group_id);
    *pbuf++ = HI_UINT16(group_id);
    memcpy(pbuf, group_name, group_name[0] + 1);

    status = zb_zcl_send_cmd(
                src_ep, dst_addr, ZCL_CLUSTER_ID_GENERAL_GROUPS, cmd,
                true, ZCL_FRAME_CLIENT_SERVER_DIR, disable_default_rsp, 0, seq_num, len, buf);
    ZB_MEM_FREE(buf);
    return status;
}

zb_status_t
zb_zcl_general_send_group_get_membership_request(
    uint16_t src_ep, s_zb_af_address_t *dst_addr,
    uint8_t cmd, uint8_t rsp_cmd, uint8_t direction, uint8_t capacity,
    uint8_t group_cnt, uint16_t *group_list, uint8_t disable_default_rsp, uint8_t seq_num)
{
    uint8_t *buf;
    uint8_t *pbuf;
    uint8_t len;
    zb_status_t status;

    len = rsp_cmd ? 1 : 0; // Capacity
    len += 1 + sizeof(uint16_t) * group_cnt; // Group count and list

    buf = (uint8_t *)ZB_MEM_MALLOC(len);
    if (buf == NULL)
    {
        return ZB_MEM_ERROR;
    }
    pbuf = buf;
    if (rsp_cmd)
    {
        *pbuf++ = capacity;
    }
    *pbuf++ = group_cnt;
    for (uint8_t i = 0; i < group_cnt; i++)
    {
        *pbuf++ = LO_UINT16(group_list[i]);
        *pbuf++ = HI_UINT16(group_list[i]);
    }
    status = zb_zcl_send_cmd(
                src_ep, dst_addr, ZCL_CLUSTER_ID_GENERAL_GROUPS, cmd,
                true, direction, disable_default_rsp, 0, seq_num, len, buf);
    ZB_MEM_FREE(buf);
    return status;
}

// ZCL_SCENES Client commands
/**
 * @brief Send an (Enhanced) Add Scene Request message
 * 
 * @param src_ep - Sending application's endpoint
 * @param dst_addr - where to send the command to
 * @param cmd - the command to send, one of: COMMAND_SCENES_ADD_SCENE, COMMAND_SCENES_ENHANCED_ADD_SCENE
 * @param scene - the scene to add
 * @param disable_default_rsp - if true, the default response will not be sent
 * @param seq_num - the sequence number of the command
 * @return zb_status_t - ZB_SUCCESS if the command was sent successfully, otherwise an error code
 */
zb_status_t
zb_zcl_general_send_add_scene_request(
    uint16_t src_ep, s_zb_af_address_t *dst_addr,
    uint8_t cmd, s_zb_zcl_general_scene_t *scene,
    uint8_t disable_default_rsp, uint8_t seq_num)
{
    uint8_t *buf;
    uint8_t *pbuf;
    uint8_t len;
    zb_status_t status;

    len = 2 + 1 + 2; // Group ID + Scene ID + Transition time
    len += scene->name[0] + 1; // Scene name length and name

    // Add something for the extension field length
    len += scene->ext_len;

    buf = (uint8_t *)ZB_MEM_MALLOC(len);
    if (buf == NULL)
    {
        return ZB_MEM_ERROR;
    }
    pbuf = buf;
    *pbuf++ = LO_UINT16(scene->group_id);
    *pbuf++ = HI_UINT16(scene->group_id);
    *pbuf++ = scene->scene_id;
    *pbuf++ = LO_UINT16(scene->trans_time);
    *pbuf++ = HI_UINT16(scene->trans_time);
    memcpy(pbuf, scene->name, scene->name[0] + 1);
    pbuf += scene->name[0] + 1; // move pass name

    // Add the extension field
    if (scene->ext_len > 0)
    {
        memcpy(pbuf, scene->ext_field, scene->ext_len);
    }
    status = zb_zcl_send_cmd(
                src_ep, dst_addr, ZCL_CLUSTER_ID_GENERAL_SCENES, cmd,
                true, ZCL_FRAME_CLIENT_SERVER_DIR, disable_default_rsp, 0, seq_num, len, buf);
    ZB_MEM_FREE(buf);
    return status;
}

/**
 * @brief Send a Scene command (request) - not Scene Add
 * 
 * @param src_ep - Sending application's endpoint
 * @param dst_addr - where to send the command to
 * @param cmd - the command to send, one of:
 * COMMAND_SCENES_VIEW_SCENE,
 * COMMAND_SCENES_REMOVE_SCENE,
 * COMMAND_SCENES_REMOVE_ALL_SCENES,
 * COMMAND_SCENES_STORE_SCENE,
 * COMMAND_SCENES_RECALL_SCENE,
 * COMMAND_SCENES_GET_SCENE_MEMBERSHIP
 * COMMAND_SCENES_ENHANCED_VIEW_SCENE,
 * @param group_id - the group ID
 * @param scene_id - the scene ID (not applicable to COMMAND_SCENES_REMOVE_ALL_SCENES
 * and COMMAND_SCENES_GET_SCENE_MEMBERSHIP)
 * @param disable_default_rsp - if true, the default response will not be sent
 * @param seq_num - the sequence number of the command
 * @return zb_status_t - ZB_SUCCESS if the command was sent successfully, otherwise an error code
 */
zb_status_t
zb_zcl_general_send_scene_request(
    uint16_t src_ep, s_zb_af_address_t *dst_addr,
    uint8_t cmd, uint16_t group_id, uint8_t scene_id,
    uint8_t disable_default_rsp, uint8_t seq_num)
{
    uint8_t buf[3];
    uint8_t len = 2;

    buf[0] = LO_UINT16(group_id);
    buf[1] = HI_UINT16(group_id);
    if (cmd != COMMAND_SCENES_REMOVE_ALL_SCENES && cmd != COMMAND_SCENES_GET_SCENE_MEMBERSHIP)
    {
        buf[2] = scene_id;
        len++;
    }
    return zb_zcl_send_cmd(
                src_ep, dst_addr, ZCL_CLUSTER_ID_GENERAL_SCENES, cmd,
                true, ZCL_FRAME_CLIENT_SERVER_DIR, disable_default_rsp, 0, seq_num, len, buf);
}

// ZCL_LIGHT_LINK_ENHANCE
/**
 * @brief Send a Scene Copy Request message
 * 
 * @param src_ep - Sending application's endpoint
 * @param dst_addr - where to send the command to
 * @param mode - the mode to use
 * @param group_id_from - the group ID to copy from
 * @param scene_id_from - the scene ID to copy from
 * @param group_id_to - the group ID to copy to
 * @param scene_id_to - the scene ID to copy to
 * @param disable_default_rsp - if true, the default response will not be sent
 * @param seq_num - the sequence number of the command
 * @return zb_status_t - ZB_SUCCESS if the command was sent successfully, otherwise an error code
 */
zb_status_t
zb_zcl_general_send_scene_copy(
    uint16_t src_ep, s_zb_af_address_t *dst_addr,
    uint8_t mode, uint16_t group_id_from, uint8_t scene_id_from,
    uint16_t group_id_to, uint8_t scene_id_to,
    uint8_t disable_default_rsp, uint8_t seq_num)
{
    uint8_t buf[7];

    buf[0] = mode;
    buf[1] = LO_UINT16(group_id_from);
    buf[2] = HI_UINT16(group_id_from);
    buf[3] = scene_id_from;
    buf[4] = LO_UINT16(group_id_to);
    buf[5] = HI_UINT16(group_id_to);
    buf[6] = scene_id_to;
    return zb_zcl_send_cmd(
                src_ep, dst_addr, ZCL_CLUSTER_ID_GENERAL_SCENES, COMMAND_SCENES_COPY_SCENE,
                true, ZCL_FRAME_CLIENT_SERVER_DIR, disable_default_rsp, 0, seq_num, 7, buf);
}

// ZCL_ON_OFF
// ZCL_LIGHT_LINK_ENHANCE
/**
 * @brief Send an Off With Effect Command
 * 
 * @param src_ep - Sending application's endpoint
 * @param dst_addr - where to send the command to
 * @param effect_id - the effect to use
 * @param effect_variant - the variant of the effect to use
 * @param disable_default_rsp - if true, the default response will not be sent
 * @param seq_num - the sequence number of the command
 * @return zb_status_t - ZB_SUCCESS if the command was sent successfully, otherwise an error code
 */
zb_status_t
zb_zcl_general_send_on_off_cmd_off_with_effect(
    uint16_t src_ep, s_zb_af_address_t *dst_addr,
    uint8_t effect_id, uint8_t effect_variant,
    uint8_t disable_default_rsp, uint8_t seq_num)
{
    uint8_t buf[2];
    buf[0] = effect_id;
    buf[1] = effect_variant;
    return zb_zcl_send_cmd(
                src_ep, dst_addr, ZCL_CLUSTER_ID_GENERAL_ON_OFF, COMMAND_ON_OFF_OFF_WITH_EFFECT,
                true, ZCL_FRAME_CLIENT_SERVER_DIR, disable_default_rsp, 0, seq_num, 2, buf);
}

/**
 * @brief Send an On With Timed Off Command
 * 
 * @param src_ep - Sending application's endpoint
 * @param dst_addr - where to send the command to
 * @param on_off_ctrl - how the lamp is to be operated
 * @param on_time - the length of time (in 1/10ths second) that the lamp is to remain on, before automatically turning off
 * @param off_wait_time - the length of time (in 1/10ths second) that the lamp shall remain off, and guarded to prevent an on command turning the light back on
 * @param disable_default_rsp - if true, the default response will not be sent
 * @param seq_num - the sequence number of the command
 * @return zb_status_t - ZB_SUCCESS if the command was sent successfully, otherwise an error code
 */
zb_status_t
zb_zcl_general_send_on_off_cmd_on_with_timed_off(
    uint16_t src_ep, s_zb_af_address_t *dst_addr,
    s_zb_zcl_on_off_ctrl_t on_off_ctrl, uint16_t on_time,
    uint16_t off_wait_time, uint8_t disable_default_rsp, uint8_t seq_num)
{
    uint8_t buf[5];
    buf[0] = on_off_ctrl.byte;
    buf[1] = LO_UINT16(on_time);
    buf[2] = HI_UINT16(on_time);
    buf[3] = LO_UINT16(off_wait_time);
    buf[4] = HI_UINT16(off_wait_time);
    return zb_zcl_send_cmd(
                src_ep, dst_addr, ZCL_CLUSTER_ID_GENERAL_ON_OFF, COMMAND_ON_OFF_ON_WITH_TIMED_OFF,
                true, ZCL_FRAME_CLIENT_SERVER_DIR, disable_default_rsp, 0, seq_num, 5, buf);
}

// ZCL_LEVEL_CTRL
/**
 * @brief Send a Level Control Move to Level Request
 * 
 * @param src_ep - Sending application's endpoint
 * @param dst_addr - where to send the command to
 * @param cmd - the command to send, one of: COMMAND_LEVEL_MOVE_TO_LEVEL, COMMAND_LEVEL_MOVE_TO_LEVEL_WITH_ON_OFF
 * @param level - the level to move to
 * @param trans_time - the time to take to move to the level (in seconds)
 * @param disable_default_rsp - if true, the default response will not be sent
 * @param seq_num - the sequence number of the command
 * @return zb_status_t - ZB_SUCCESS if the command was sent successfully, otherwise an error code
 */
zb_status_t
zb_zcl_general_send_level_control_move_to_level_request(
    uint16_t src_ep, s_zb_af_address_t *dst_addr,
    uint8_t cmd, uint8_t level, uint16_t trans_time,
    uint8_t disable_default_rsp, uint8_t seq_num)
{
    uint8_t buf[3];
    buf[0] = level;
    buf[1] = LO_UINT16(trans_time);
    buf[2] = HI_UINT16(trans_time);
    return zb_zcl_send_cmd(
                src_ep, dst_addr, ZCL_CLUSTER_ID_GENERAL_LEVEL_CONTROL, cmd,
                true, ZCL_FRAME_CLIENT_SERVER_DIR, disable_default_rsp, 0, seq_num, 3, buf);
}

/**
 * @brief Send a Level Control Move Request
 * 
 * @param src_ep - Sending application's endpoint
 * @param dst_addr - where to send the command to
 * @param cmd - the command to send, one of: COMMAND_LEVEL_MOVE, COMMAND_LEVEL_MOVE_WITH_ON_OFF
 * @param move_mode - the mode to use, one of: LEVEL_MOVE_UP, LEVEL_MOVE_DOWN
 * @param rate - the rate to use, number of steps to take per second
 * @param disable_default_rsp - if true, the default response will not be sent
 * @param seq_num - the sequence number of the command
 * @return zb_status_t - ZB_SUCCESS if the command was sent successfully, otherwise an error code
 */
zb_status_t
zb_zcl_general_send_level_control_move_request(
    uint16_t src_ep, s_zb_af_address_t *dst_addr,
    uint8_t cmd, uint8_t move_mode, uint8_t rate,
    uint8_t disable_default_rsp, uint8_t seq_num)
{
    uint8_t buf[2];
    buf[0] = move_mode;
    buf[1] = rate;
    return zb_zcl_send_cmd(
        src_ep, dst_addr, ZCL_CLUSTER_ID_GENERAL_LEVEL_CONTROL, cmd,
        true, ZCL_FRAME_CLIENT_SERVER_DIR, disable_default_rsp, 0, seq_num, 2, buf);
}

/**
 * @brief Send a Level Control Step Request
 * 
 * @param src_ep - Sending application's endpoint
 * @param dst_addr - where to send the command to
 * @param cmd - the command to send, one of: COMMAND_LEVEL_STEP, COMMAND_LEVEL_STEP_WITH_ON_OFF
 * @param step_mode - the mode to use, one of: LEVEL_STEP_UP, LEVEL_STEP_DOWN
 * @param step_size - the size to use, number of steps to take
 * @param trans_time - the time to take to perform the step (in seconds)
 * @param disable_default_rsp - if true, the default response will not be sent
 * @param seq_num - the sequence number of the command
 * @return zb_status_t - ZB_SUCCESS if the command was sent successfully, otherwise an error code
 */
zb_status_t
zb_zcl_general_send_level_control_step_request(
    uint16_t src_ep, s_zb_af_address_t *dst_addr,
    uint8_t cmd, uint8_t step_mode, uint8_t step_size, uint16_t trans_time,
    uint8_t disable_default_rsp, uint8_t seq_num)
{
    uint8_t buf[4];
    buf[0] = step_mode;
    buf[1] = step_size;
    buf[2] = LO_UINT16(trans_time);
    buf[3] = HI_UINT16(trans_time);
    return zb_zcl_send_cmd(
                src_ep, dst_addr, ZCL_CLUSTER_ID_GENERAL_LEVEL_CONTROL, cmd,
                true, ZCL_FRAME_CLIENT_SERVER_DIR, disable_default_rsp, 0, seq_num, 4, buf);
}

/**
 * @brief Send a Level Control Stop Request
 * 
 * @param src_ep - Sending application's endpoint
 * @param dst_addr - where to send the command to
 * @param cmd - the command to send, one of: COMMAND_LEVEL_STOP, COMMAND_LEVEL_STOP_WITH_ON_OFF
 * @param disable_default_rsp - if true, the default response will not be sent
 * @param seq_num - the sequence number of the command
 * @return zb_status_t - ZB_SUCCESS if the command was sent successfully, otherwise an error code
 */
zb_status_t
zb_zcl_general_send_level_control_stop_request(
    uint16_t src_ep, s_zb_af_address_t *dst_addr,
    uint8_t cmd, uint8_t disable_default_rsp, uint8_t seq_num)
{
    return zb_zcl_send_cmd(
                src_ep, dst_addr, ZCL_CLUSTER_ID_GENERAL_LEVEL_CONTROL, cmd,
                true, ZCL_FRAME_CLIENT_SERVER_DIR, disable_default_rsp, 0, seq_num, 0, NULL);
}

// ZCL_ALARMS
/**
 * @brief Send an Alarm Command
 * 
 * @param src_ep - Sending application's endpoint
 * @param dst_addr - where to send the command to
 * @param alarm_code - the alarm code to send
 * @param cluster_id - the cluster ID to send the alarm to
 * @param disable_default_rsp - if true, the default response will not be sent
 * @param seq_num - the sequence number of the command
 * @return zb_status_t - ZB_SUCCESS if the command was sent successfully, otherwise an error code
 */
zb_status_t
zb_zcl_general_send_alarms(
    uint16_t src_ep, s_zb_af_address_t *dst_addr,
    uint8_t alarm_code, uint16_t cluster_id,
    uint8_t disable_default_rsp, uint8_t seq_num)
{
    uint8_t buf[3];
    buf[0] = alarm_code;
    buf[1] = LO_UINT16(cluster_id);
    buf[2] = HI_UINT16(cluster_id);
    return zb_zcl_send_cmd(
                src_ep, dst_addr, ZCL_CLUSTER_ID_GENERAL_ALARMS, COMMAND_ALARMS_ALARM,
                true, ZCL_FRAME_CLIENT_SERVER_DIR, disable_default_rsp, 0, seq_num, 2, buf);
}

/**
 * @brief Send an Alarm Reset Command
 * 
 * @param src_ep - Sending application's endpoint
 * @param dst_addr - where to send the command to
 * @param alarm_code - the alarm code to reset
 * @param cluster_id - the cluster ID to reset the alarm for
 * @param disable_default_rsp - if true, the default response will not be sent
 * @param seq_num - the sequence number of the command
 * @return zb_status_t - ZB_SUCCESS if the command was sent successfully, otherwise an error code
 */
zb_status_t
zb_zcl_general_send_alarms_reset(
    uint16_t src_ep, s_zb_af_address_t *dst_addr,
    uint8_t alarm_code, uint16_t cluster_id,
    uint8_t disable_default_rsp, uint8_t seq_num)
{
    uint8_t buf[3];
    buf[0] = alarm_code;
    buf[1] = LO_UINT16(cluster_id);
    buf[2] = HI_UINT16(cluster_id);
    return zb_zcl_send_cmd(
                src_ep, dst_addr, ZCL_CLUSTER_ID_GENERAL_ALARMS, COMMAND_ALARMS_RESET_ALARM,
                true, ZCL_FRAME_CLIENT_SERVER_DIR, disable_default_rsp, 0, seq_num, 3, buf);
}

/**
 * @brief Send an Alarm Get Response Command
 * 
 * @param src_ep - Sending application's endpoint
 * @param dst_addr - where to send the command to
 * @param status - the status of the command
 * @param alarm_code - the alarm code to get the response for
 * @param cluster_id - the cluster ID to get the response for
 * @param timestamp - the timestamp of the alarm
 * @param disable_default_rsp - if true, the default response will not be sent
 * @param seq_num - the sequence number of the command
 * @return zb_status_t - ZB_SUCCESS if the command was sent successfully, otherwise an error code
 */
zb_status_t
zb_zcl_general_send_alarms_get_response(
    uint16_t src_ep, s_zb_af_address_t *dst_addr,
    uint8_t status, uint8_t alarm_code, uint16_t cluster_id,
    uint32_t timestamp, uint8_t disable_default_rsp, uint8_t seq_num)
{
    uint8_t buf[8];
    uint8_t len = 1;
    uint8_t *pbuf = buf;
    *pbuf++ = status;
    if (status == ZCL_STATUS_SUCCESS)
    {
        len += 1 + 2 + 4; // alarm_code + cluster_id + timestamp
        *pbuf++ = alarm_code;
        *pbuf++ = LO_UINT16(cluster_id);
        *pbuf++ = HI_UINT16(cluster_id);
        BUILD_UINT32_TO_BUFFER(pbuf, timestamp);
    }
    return zb_zcl_send_cmd(
        src_ep, dst_addr, ZCL_CLUSTER_ID_GENERAL_ALARMS, COMMAND_ALARMS_GET_ALARM_RESPONSE,
        true, ZCL_FRAME_CLIENT_SERVER_DIR, disable_default_rsp, 0, seq_num, len, buf);
}

/**
 * @brief Find the application callbacks for a given endpoint
 * 
 * @param endpoint - the endpoint to find the callbacks for
 * @return s_zb_zcl_general_app_callbacks_t* - the application callbacks for the given endpoint
 *         NULL if no callbacks were found
 */
static s_zb_zcl_general_app_callbacks_t*
zcl_general_find_app_callbacks(uint8_t endpoint)
{
    s_zb_zcl_general_cb_rec_t *p_cb;
    p_cb = g_zcl_general_cb_list;
    while (p_cb != NULL)
    {
        if (p_cb->endpoint == endpoint)
        {
            return p_cb->cb;
        }
        p_cb = p_cb->next;
    }
    return NULL;
}

/**
 * @brief Callback from ZCL to process incoming Commands specific to this cluster library
 *        or Profile commands for attributes that aren't in the attribute list
 * 
 * @param msg - the incoming message
 * @return zb_status_t - ZB_SUCCESS if the command was processed successfully, otherwise an error code
 */
static zb_status_t
zcl_general_handle_incoming(s_zb_zcl_incoming_msg_t *msg)
{
    zb_status_t status = ZB_SUCCESS;

    if (ZCL_CLUSTER_CMD(msg->hdr.fc.type))
    {
        // Is this a manufacturer specific command?
        if (msg->hdr.fc.manu_specific == 0)
        {
            status = zcl_general_handle_in_specific_commands(msg);
        }
        else
        {
            // We don't support any manufacturer specific commands
            status = ZB_FAILURE;
        }
    }
    else
    {
        // Handle all the normal commands (Read, Write, etc.) -- should never get here
        status = ZB_FAILURE;
    }
    return status;
}

static zb_status_t
zcl_general_handle_in_specific_commands(s_zb_zcl_incoming_msg_t *msg)
{
    zb_status_t status;
    s_zb_zcl_general_app_callbacks_t *pcb;

    pcb = zcl_general_find_app_callbacks(msg->msg->src_endpoint);

    if (pcb == NULL)
    {
        return ZB_FAILURE;
    }

    switch (msg->msg->cluster_id)
    {
        case ZCL_CLUSTER_ID_GENERAL_IDENTIFY:
            status = zcl_general_process_in_identify(msg, pcb);
            break;
        case ZCL_CLUSTER_ID_GENERAL_GROUPS:
            if (ZCL_CLIENT_CMD(msg->hdr.fc.direction))
            {
                status = zcl_general_process_in_groups_client(msg, pcb);
            }
            else
            {
                status = ZB_FAILURE;
            }
            break;
        case ZCL_CLUSTER_ID_GENERAL_SCENES:
            if (ZCL_CLIENT_CMD(msg->hdr.fc.direction))
            {
                status = zcl_general_process_in_scenes_client(msg, pcb);
            }
            else
            {
                status = ZB_FAILURE;
            }
            break;
        case ZCL_CLUSTER_ID_GENERAL_ALARMS:
            if (ZCL_CLIENT_CMD(msg->hdr.fc.direction))
            {
                status = zcl_general_process_in_alarms_client(msg, pcb);
            }
            else
            {
                status = ZB_FAILURE;
            }
            break;
        default:
            status = ZB_FAILURE;
            break;
    }
    return status;
}

/**
 * @brief Process an incoming Identify command
 * 
 * @param msg - the incoming message
 * @param cb - the application callbacks
 * @return zb_status_t - ZB_SUCCESS if the command was processed successfully, otherwise an error code
 */
static zb_status_t
zcl_general_process_in_identify(
    s_zb_zcl_incoming_msg_t *msg, s_zb_zcl_general_app_callbacks_t *cb)
{
    if (ZCL_CLIENT_CMD(msg->hdr.fc.direction))
    {
        if (msg->hdr.command_id > COMMAND_IDENTIFY_IDENTIFY_QUERY_RESPONSE)
        {
            return ZB_FAILURE;
        }
        if (cb->pfn_identify_query_rsp != NULL)
        {
            s_zb_zcl_identify_query_rsp_t rsp;
            rsp.src_addr = &msg->msg->src_addr;
            rsp.timeout = BUILD_UINT16(msg->data[0], msg->data[1]);
            cb->pfn_identify_query_rsp(&rsp);
        }
    }
    return ZB_SUCCESS;
}

// ZCL_GROUPS Client commands
/**
 * @brief Process an incoming Groups command
 * 
 * @param msg - the incoming message
 * @param cb - the application callbacks
 * @return zb_status_t - ZB_SUCCESS if the command was processed successfully, otherwise an error code
 */
static zb_status_t
zcl_general_process_in_groups_client(
    s_zb_zcl_incoming_msg_t *msg, s_zb_zcl_general_app_callbacks_t *cb)
{
    s_zb_aps_group_t group;
    uint8_t *pdata = msg->data;
    uint8_t group_cnt;
    uint8_t name_len;
    s_zb_zcl_groups_rsp_t rsp;
    uint16_t *group_list = NULL;
    zb_status_t status = ZB_SUCCESS;

    memset(&group, 0, sizeof(s_zb_aps_group_t));
    memset(&rsp, 0, sizeof(s_zb_zcl_groups_rsp_t));

    switch (msg->hdr.command_id)
    {
        case COMMAND_GROUPS_ADD_GROUP_RESPONSE:
        case COMMAND_GROUPS_VIEW_GROUP_RESPONSE:
        case COMMAND_GROUPS_REMOVE_GROUP_RESPONSE:
            rsp.cmd_id = *pdata++;
            group.id = BUILD_UINT16(pdata[0], pdata[1]);
            if (rsp.status == ZCL_STATUS_SUCCESS && msg->hdr.command_id == COMMAND_GROUPS_VIEW_GROUP_RESPONSE)
            {
                pdata += 2;
                name_len = *pdata++;
                if (name_len > (APS_GROUP_NAME_LEN - 1))
                {
                    name_len = APS_GROUP_NAME_LEN - 1;
                }
                group.name[0] = name_len;
                memcpy(&group.name[1], pdata, name_len);
                rsp.group_name = group.name;
            }

            if (cb->pfn_groups_rsp != NULL)
            {
                rsp.src_addr = &msg->msg->src_addr;
                rsp.cmd_id = msg->hdr.command_id;
                rsp.group_count = 1;
                rsp.group_list = &group.id;
                rsp.capacity = 0;

                cb->pfn_groups_rsp(&rsp);
            }
            break;
        case COMMAND_GROUPS_GET_GROUP_MEMBERSHIP_RESPONSE:
            rsp.capacity = *pdata++;
            group_cnt = *pdata++;

            if (group_cnt > 0)
            {
                group_list = ZB_MEM_MALLOC(group_cnt * sizeof(uint16_t));
                if (group_list)
                {
                    rsp.group_count = group_cnt;
                    for (uint8_t i = 0; i < group_cnt; i++)
                    {
                        group_list[i] = BUILD_UINT16(pdata[0], pdata[1]);
                        pdata += 2;
                    }
                }
            }
            if (cb->pfn_groups_rsp != NULL)
            {
                rsp.src_addr = &msg->msg->src_addr;
                rsp.cmd_id = msg->hdr.command_id;
                rsp.group_list = group_list;
                cb->pfn_groups_rsp(&rsp);
            }

            if (group_list)
            {
                ZB_MEM_FREE(group_list);
            }
            break;
        default:
            status = ZB_FAILURE;
            break;
    }
    return status;
}

/**
 * @brief Process an incoming Scenes command
 * 
 * @param msg - the incoming message
 * @param cb - the application callbacks
 * @return zb_status_t - ZB_SUCCESS if the command was processed successfully, otherwise an error code
 */
static zb_status_t
zcl_general_process_in_scenes_client(
    s_zb_zcl_incoming_msg_t *msg, s_zb_zcl_general_app_callbacks_t *cb)
{
    s_zb_zcl_general_scene_t scene;
    uint8_t *pdata = msg->data;
    uint8_t name_len;
    s_zb_zcl_scene_rsp_t rsp;
    uint8_t *scene_list = NULL;
    zb_status_t status = ZB_SUCCESS;

    memset(&scene, 0, sizeof(s_zb_zcl_general_scene_t));
    memset(&rsp, 0, sizeof(s_zb_zcl_scene_rsp_t));

    rsp.status = *pdata++;

    if (msg->hdr.command_id == COMMAND_SCENES_GET_SCENE_MEMBERSHIP_RESPONSE)
    {
        rsp.capacity = *pdata++;
    }

    scene.group_id = BUILD_UINT16(pdata[0], pdata[1]);
    pdata += 2;

    switch (msg->hdr.command_id)
    {
        case COMMAND_SCENES_VIEW_SCENE_RESPONSE:
            scene.scene_id = *pdata++;
            scene.trans_time = BUILD_UINT16(pdata[0], pdata[1]);
            pdata += 2;
            name_len = *pdata++;
            if (name_len > (ZCL_GENERAL_SCENE_NAME_LEN - 1))
            {
                name_len = ZCL_GENERAL_SCENE_NAME_LEN - 1;
            }
            scene.name[0] = name_len;
            memcpy(&scene.name[1], pdata, name_len);
            pdata += name_len;
            // Fall through to callback - break is left off intentionally
        case COMMAND_SCENES_ADD_SCENE_RESPONSE:
        case COMMAND_SCENES_REMOVE_SCENE_RESPONSE:
        case COMMAND_SCENES_REMOVE_ALL_SCENES_RESPONSE:
        case COMMAND_SCENES_STORE_SCENE_RESPONSE:
        case COMMAND_SCENES_ENHANCED_ADD_SCENE_RESPONSE:
        case COMMAND_SCENES_ENHANCED_VIEW_SCENE_RESPONSE:
        case COMMAND_SCENES_COPY_SCENE_RESPONSE:
            if (cb->pfn_scene_rsp != NULL)
            {
                if (msg->hdr.command_id == COMMAND_SCENES_REMOVE_ALL_SCENES_RESPONSE)
                {
                    scene.scene_id = *pdata++;
                }
                rsp.src_addr = &msg->msg->src_addr;
                rsp.cmd_id = msg->hdr.command_id;
                rsp.scene = &scene;
                cb->pfn_scene_rsp(&rsp);
            }
            break;
        case COMMAND_SCENES_GET_SCENE_MEMBERSHIP_RESPONSE:
            if (rsp.status == ZCL_STATUS_SUCCESS)
            {
                uint8_t scene_cnt = *pdata++;
                if (scene_cnt > 0)
                {
                    scene_list = ZB_MEM_MALLOC(scene_cnt);
                    if (scene_list)
                    {
                        rsp.scene_count = scene_cnt;
                        for (uint8_t i = 0; i < scene_cnt; i++)
                        {
                            scene_list[i] = *pdata++;
                        }
                    }
                }
            }
            if (cb->pfn_scene_rsp != NULL)
            {
                rsp.src_addr = &msg->msg->src_addr;
                rsp.cmd_id = msg->hdr.command_id;
                rsp.scene_list = scene_list;
                rsp.scene = &scene;
                cb->pfn_scene_rsp(&rsp);
            }

            if (scene_list)
            {
                ZB_MEM_FREE(scene_list);
            }
            break;
        default:
            status = ZB_FAILURE;
            break;
    }
    return status;
}

static zb_status_t
zcl_general_process_in_alarms_client(
    s_zb_zcl_incoming_msg_t *msg, s_zb_zcl_general_app_callbacks_t *cb)
{
    uint8_t *pdata = msg->data;
    s_zb_zcl_alarms_t alarm;
    zb_status_t status = ZB_SUCCESS;

    memset(&alarm, 0, sizeof(s_zb_zcl_alarms_t));

    switch (msg->hdr.command_id)
    {
        case COMMAND_ALARMS_ALARM:
            if (cb->pfn_alarms != NULL)
            {
                alarm.src_addr = &msg->msg->src_addr;
                alarm.cmd_id = msg->hdr.command_id;
                alarm.alarm_code = pdata[0];
                alarm.cluster_id = BUILD_UINT16(pdata[1], pdata[2]);
                cb->pfn_alarms(msg->hdr.fc.direction, &alarm);
            }
            else
            {
                status = ZB_FAILURE;
            }
            break;
        case COMMAND_ALARMS_GET_ALARM_RESPONSE:
            if (cb->pfn_alarms != NULL)
            {
                alarm.src_addr = &msg->msg->src_addr;
                alarm.cmd_id = msg->hdr.command_id;
                alarm.alarm_code = *pdata++;
                alarm.cluster_id = BUILD_UINT16(pdata[0], pdata[1]);

                cb->pfn_alarms(msg->hdr.fc.direction, &alarm);
            }
            else
            {
                status = ZB_FAILURE;
            }
            break;
        default:
            status = ZCL_STATUS_UNSUP_CLUSTER_COMMAND;
            break;
    }
    return status;
}