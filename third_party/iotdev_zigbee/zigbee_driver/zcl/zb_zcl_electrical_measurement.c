#include "common/zb_common.h"
#include "zcl/zb_zcl.h"
#include "zcl/zb_zcl_general.h"
#include "zcl/zb_zcl_electrical_measurement.h"

#define TAG "ZCL_EM"

/*********************************************************************
 * TYPEDEFS
 */

typedef struct s_zb_zcl_electrical_measurement_cb_rec
{
    struct s_zb_zcl_electrical_measurement_cb_rec *next;
    uint8_t endpoint;
    s_zb_zcl_electrical_measurement_callbacks_t *cb;
} s_zb_zcl_electrical_measurement_cb_rec_t;

/*********************************************************************
 * Local Variables
 */
static s_zb_zcl_electrical_measurement_cb_rec_t *g_zcl_electrical_measurement_cb_list = NULL;
static uint8_t g_zcl_electrical_measurement_plugin_registered = false;

/*********************************************************************
 * Local Functions
 */
static zb_status_t zcl_electrical_measurement_handle_incoming(s_zb_zcl_incoming_msg_t *msg);
static zb_status_t zcl_electrical_measurement_handle_in_specific_commands(s_zb_zcl_incoming_msg_t *msg);
static s_zb_zcl_electrical_measurement_callbacks_t *zcl_electrical_measurement_find_callbacks(uint8_t endpoint);
static zb_status_t zcl_electrical_measurement_process_in_cmds(s_zb_zcl_incoming_msg_t *msg, s_zb_zcl_electrical_measurement_callbacks_t *cb);

static zb_status_t zcl_electrical_measurement_process_in_get_profile_info_rsp(s_zb_zcl_incoming_msg_t *msg, s_zb_zcl_electrical_measurement_callbacks_t *cb);
static zb_status_t zcl_electrical_measurement_process_in_get_measurement_profile_rsp(s_zb_zcl_incoming_msg_t *msg, s_zb_zcl_electrical_measurement_callbacks_t *cb);

/**
 * @fn      zb_zcl_electrical_measurement_register_callbacks
 * 
 * @brief   Register an applications command callbacks
 * 
 * @param   endpoint - application's endpoint
 * @param   callbacks - pointer to callback record.
 * 
 * @return  zb_status_t - ZB_SUCCESS if the callback was registered successfully, otherwise an error code
 */
zb_status_t
zb_zcl_electrical_measurement_register_callbacks(uint8_t endpoint, s_zb_zcl_electrical_measurement_callbacks_t *callbacks)
{
    s_zb_zcl_electrical_measurement_cb_rec_t *p_new_item;
    s_zb_zcl_electrical_measurement_cb_rec_t *p_loop;

    if (!g_zcl_electrical_measurement_plugin_registered)
    {
        zb_zcl_register_plugin(ZCL_CLUSTER_ID_MS_ELECTRICAL_MEASUREMENT,
                                            ZCL_CLUSTER_ID_MS_ELECTRICAL_MEASUREMENT,
                                            zcl_electrical_measurement_handle_incoming);
        g_zcl_electrical_measurement_plugin_registered = true;
    }

    p_new_item = (s_zb_zcl_electrical_measurement_cb_rec_t *)ZB_MEM_MALLOC(sizeof(s_zb_zcl_electrical_measurement_cb_rec_t));
    if (p_new_item == NULL)
    {
        return ZB_MEM_ERROR;
    }
    p_new_item->next = NULL;
    p_new_item->endpoint = endpoint;
    p_new_item->cb = callbacks;

    if (g_zcl_electrical_measurement_cb_list == NULL)
    {
        g_zcl_electrical_measurement_cb_list = p_new_item;
    }
    else
    {
        p_loop = g_zcl_electrical_measurement_cb_list;
        while (p_loop->next != NULL)
        {
            p_loop = p_loop->next;
        }
        p_loop->next = p_new_item;
    }
    return ZB_SUCCESS;
}

/**
 * @fn      zb_zcl_electrical_measurement_send_get_profile_info
 * 
 * @brief   Call to send out Electrical Measurement Get Profile Info command from ZED  to ZR/ZC.
 *          The response will indicate the parameters of the device's profile
 * 
 * @param   src_ep - Sending application's endpoint
 * @param   dst_addr - where to send the command to
 * @param   disable_default_rsp - if true, the default response will not be sent
 * @param   seq_num - the sequence number of the command
 * 
 * @return  zb_status_t - ZB_SUCCESS if the command was sent successfully, otherwise an error code
 */
zb_status_t
zb_zcl_electrical_measurement_send_get_profile_info(
    uint8_t src_ep, s_zb_af_address_t *dst_addr,
    uint8_t disable_default_rsp, uint8_t seq_num)
{
    return zb_zcl_send_cmd(
        src_ep, dst_addr, ZCL_CLUSTER_ID_MS_ELECTRICAL_MEASUREMENT, COMMAND_ELECTRICAL_MEASUREMENT_GET_PROFILE_INFO,
        true, ZCL_FRAME_CLIENT_SERVER_DIR, disable_default_rsp, 0, seq_num, 0, NULL);
}

/**
 * @fn      zb_zcl_electrical_measurement_send_get_measurement_profile
 * 
 * @brief   Call to send out Electrical Measurement Get Measurement Profile.
 *          This will ask the server for the approprite parameters of the measurement profile.
 * 
 * @param   src_ep - Sending application's endpoint
 * @param   dst_addr - where to send the command to
 * @param   attr_id - The electrical measurement attribute being profiled
 * @param   start_time - Selects the interval block from available interval blocks
 * @param   number_of_intervals - Represents the number of intervals being requested
 * @param   disable_default_rsp - if true, the default response will not be sent
 * @param   seq_num - the sequence number of the command
 * 
 * @return  zb_status_t - ZB_SUCCESS if the command was sent successfully, otherwise an error code
 */
zb_status_t
zb_zcl_electrical_measurement_send_get_measurement_profile(
    uint8_t src_ep, s_zb_af_address_t *dst_addr, uint16_t attr_id, uint32_t start_time,
    uint8_t number_of_intervals, uint8_t disable_default_rsp, uint8_t seq_num)
{
    uint8_t buf[7];
    buf[0] = LO_UINT16(attr_id);
    buf[1] = HI_UINT16(attr_id);
    buf[2] = BREAK_UINT32(start_time, 0);
    buf[3] = BREAK_UINT32(start_time, 1);
    buf[4] = BREAK_UINT32(start_time, 2);
    buf[5] = BREAK_UINT32(start_time, 3);
    buf[6] = number_of_intervals;
    return zb_zcl_send_cmd(
        src_ep, dst_addr, ZCL_CLUSTER_ID_MS_ELECTRICAL_MEASUREMENT, COMMAND_ELECTRICAL_MEASUREMENT_GET_MEASUREMENT_PROFILE,
        true, ZCL_FRAME_CLIENT_SERVER_DIR, disable_default_rsp, 0, seq_num, 7, buf);
}

/**
 * @fn      zb_zcl_electrical_measurement_find_callbacks
 * 
 * @brief   Find the callbacks for an endpoint
 * 
 * @param   endpoint - the endpoint to find the callbacks for
 * 
 * @return  pointer to the callbacks
 */
static s_zb_zcl_electrical_measurement_callbacks_t *
zcl_electrical_measurement_find_callbacks(uint8_t endpoint)
{
    s_zb_zcl_electrical_measurement_cb_rec_t *p_loop = g_zcl_electrical_measurement_cb_list;
    while (p_loop != NULL)
    {
        if (p_loop->endpoint == endpoint)
        {
            return p_loop->cb;
        }
        p_loop = p_loop->next;
    }
    return NULL;
}

/**
 * @fn      zcl_electrical_measurement_handle_incoming
 * 
 * @brief   Callback from ZCL to process incoming commands specific to this cluster library
 *          or Profile commands for attributes that aren't in the attribute list
 * 
 * @param   msg - pointer to the incoming message
 * 
 * @return  zb_status_t - ZB_SUCCESS if the command was processed successfully, otherwise an error code
 */
static zb_status_t
zcl_electrical_measurement_handle_incoming(s_zb_zcl_incoming_msg_t *msg)
{
    zb_status_t status = ZB_SUCCESS;

    if (ZCL_CLUSTER_CMD(msg->hdr.fc.type))
    {
        // Is this a manufacturer specific command?
        if (msg->hdr.fc.manu_specific == 0)
        {
            status = zcl_electrical_measurement_handle_in_specific_commands(msg);
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

/**
 * @fn      zcl_electrical_measurement_handle_in_specific_commands
 * 
 * @brief   Handle incoming commands specific to this cluster library
 * 
 * @param   msg - pointer to the incoming message
 * 
 * @return  zb_status_t - ZB_SUCCESS if the command was processed successfully, otherwise an error code
 */
static zb_status_t
zcl_electrical_measurement_handle_in_specific_commands(s_zb_zcl_incoming_msg_t *msg)
{
    zb_status_t status;
    s_zb_zcl_electrical_measurement_callbacks_t *cb;

    cb = zcl_electrical_measurement_find_callbacks(msg->msg->src_endpoint);

    if (cb == NULL)
    {
        return ZB_FAILURE;
    }

    status = zcl_electrical_measurement_process_in_cmds(msg, cb);

    return status;
}

/**
 * @fn      zcl_electrical_measurement_process_in_cmds
 * 
 * @brief   Process incoming commands for this cluster
 * 
 * @param   msg - pointer to the incoming message
 * @param   cb - pointer to the callbacks for the endpoint
 * 
 * @return  zb_status_t - ZB_SUCCESS if the command was processed successfully, otherwise an error code
 */
static zb_status_t
zcl_electrical_measurement_process_in_cmds(s_zb_zcl_incoming_msg_t *msg, s_zb_zcl_electrical_measurement_callbacks_t *cb)
{
    zb_status_t status = ZB_SUCCESS;

    // Server to client commands
    if (ZCL_CLIENT_CMD(msg->hdr.fc.direction))
    {
        switch (msg->hdr.command_id)
        {
            case COMMAND_ELECTRICAL_MEASUREMENT_GET_PROFILE_INFO_RSP:
                status = zcl_electrical_measurement_process_in_get_profile_info_rsp(msg, cb);
                break;
            case COMMAND_ELECTRICAL_MEASUREMENT_GET_MEASUREMENT_PROFILE_RSP:
                status = zcl_electrical_measurement_process_in_get_measurement_profile_rsp(msg, cb);
                break;
            default:
                status = ZB_FAILURE;
                break;
        }
    }
    else
    {
        // We don't expect any commands from client to server
        status = ZB_FAILURE;
    }

    return status;
}

/**
 * @fn      zcl_electrical_measurement_process_in_get_profile_info_rsp
 * 
 * @brief   Process incoming Get Profile Info Response command
 * 
 * @param   msg - pointer to the incoming message
 * @param   cb - pointer to the callbacks for the endpoint
 * 
 * @return  zb_status_t - ZB_SUCCESS if the command was processed successfully, otherwise an error code
 */
static zb_status_t
zcl_electrical_measurement_process_in_get_profile_info_rsp(s_zb_zcl_incoming_msg_t *msg, s_zb_zcl_electrical_measurement_callbacks_t *cb)
{
    uint8_t offset;
    uint16_t calculated_array_size;
    s_zb_zcl_electrical_measurement_get_profile_info_rsp_t cmd;
    zb_status_t status;

    if (cb->pfn_get_profile_info_rsp != NULL)
    {
        // Calculate size of variable array
        calculated_array_size = msg->data_len - 3; // variable array - 3 bytes of fixed variables

        cmd.attrs_list = (uint16_t *)ZB_MEM_MALLOC(calculated_array_size);

        if (cmd.attrs_list == NULL)
        {
            return ZB_MEM_ERROR;
        }
        cmd.profile_count = msg->data[0];
        cmd.profile_interval_period = msg->data[1];
        cmd.max_number_of_intervals = msg->data[2];
        cmd.number_of_attrs = calculated_array_size / 2; // each attribute ID is 2 bytes
        offset = 3;

        for (int i = 0; i < cmd.number_of_attrs; i++)
        {
            cmd.attrs_list[i] = BUILD_UINT16(msg->data[offset], msg->data[offset + 1]);
            offset += 2;
        }

        status = cb->pfn_get_profile_info_rsp(&cmd);
        ZB_MEM_FREE(cmd.attrs_list);
        return status;
    }

    return ZB_FAILURE;
}

/**
 * @fn      zcl_electrical_measurement_process_in_get_measurement_profile_rsp
 * 
 * @brief   Process incoming Get Measurement Profile Response command
 * 
 * @param   msg - pointer to the incoming message
 * @param   cb - pointer to the callbacks for the endpoint
 * 
 * @return  zb_status_t - ZB_SUCCESS if the command was processed successfully, otherwise an error code
 */
static zb_status_t
zcl_electrical_measurement_process_in_get_measurement_profile_rsp(s_zb_zcl_incoming_msg_t *msg, s_zb_zcl_electrical_measurement_callbacks_t *cb)
{
    uint8_t offset;
    uint16_t calculated_array_size;
    s_zb_zcl_electrical_measurement_get_measurement_profile_rsp_t cmd;
    zb_status_t status;

    if (cb->pfn_get_measurement_profile_rsp != NULL)
    {
        // Calculate size of intervals by subtracting size of start_time, status,
        // profile_interval_period, number_of_intervals_delivered, and attr_id from message length
        calculated_array_size = msg->data_len - 9;

        cmd.interval_list = ZB_MEM_MALLOC(calculated_array_size);

        if (cmd.interval_list == NULL)
        {
            return ZB_MEM_ERROR;
        }
        cmd.start_time = BUILD_UINT32(msg->data[0], msg->data[1], msg->data[2], msg->data[3]);
        cmd.status = msg->data[4];
        cmd.profile_interval_period = msg->data[5];
        cmd.number_of_intervals_delivered = msg->data[6];
        cmd.attr_id = BUILD_UINT16(msg->data[7], msg->data[8]);
        offset = 9;

        for (int i = 0; i < cmd.number_of_intervals_delivered; i++)
        {
            cmd.interval_list[i] = msg->data[offset++];
        }

        status = cb->pfn_get_measurement_profile_rsp(&cmd);
        ZB_MEM_FREE(cmd.interval_list);
        return status;
    }

    return ZB_FAILURE;
}