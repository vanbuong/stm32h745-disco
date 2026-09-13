#include "zcl/zb_zcl.h"
#include "zcl/zb_zcl_general.h"
#include "zcl/zb_zcl_closures.h"

/*********************************************************************
 * MACROS
 */
#define TAG "ZCL_CLOSURES"

/*********************************************************************
 * TYPEDEFS
 */
typedef struct s_zb_zcl_closures_door_lock_cb_rec
{
    struct s_zb_zcl_closures_door_lock_cb_rec *next;
    uint8_t endpoint;
    s_zb_zcl_closures_door_lock_app_callbacks_t *cb;
} s_zb_zcl_closures_door_lock_cb_rec_t;

/*********************************************************************
 * LOCAL VARIABLES
 */
static s_zb_zcl_closures_door_lock_cb_rec_t *g_zcl_closures_door_lock_cb_list = NULL;
static uint8_t g_zcl_closures_door_lock_plugin_registered = false;

/*********************************************************************
 * LOCAL FUNCTIONS
 */
static zb_status_t zcl_closures_handle_incoming(s_zb_zcl_incoming_msg_t *msg);
static zb_status_t zcl_closures_handle_in_specific_commands(s_zb_zcl_incoming_msg_t *msg);

// ZCL_DOOR_LOCK
static s_zb_zcl_closures_door_lock_app_callbacks_t *zcl_closures_find_door_lock_callbacks(uint8_t endpoint);
static zb_status_t zcl_closures_process_in_door_lock_cmds(s_zb_zcl_incoming_msg_t *msg, s_zb_zcl_closures_door_lock_app_callbacks_t *cb);
static zb_status_t zcl_closures_process_in_door_lock_rsp(s_zb_zcl_incoming_msg_t *msg, s_zb_zcl_closures_door_lock_app_callbacks_t *cb);
static zb_status_t zcl_closures_process_in_door_lock_unlock_with_timeout_rsp(s_zb_zcl_incoming_msg_t *msg, s_zb_zcl_closures_door_lock_app_callbacks_t *cb);
static zb_status_t zcl_closures_process_in_door_lock_get_log_record_rsp(s_zb_zcl_incoming_msg_t *msg, s_zb_zcl_closures_door_lock_app_callbacks_t *cb);
static zb_status_t zcl_closures_process_in_door_lock_set_pin_code_rsp(s_zb_zcl_incoming_msg_t *msg, s_zb_zcl_closures_door_lock_app_callbacks_t *cb);
static zb_status_t zcl_closures_process_in_door_lock_get_pin_code_rsp(s_zb_zcl_incoming_msg_t *msg, s_zb_zcl_closures_door_lock_app_callbacks_t *cb);
static zb_status_t zcl_closures_process_in_door_lock_clear_pin_code_rsp(s_zb_zcl_incoming_msg_t *msg, s_zb_zcl_closures_door_lock_app_callbacks_t *cb);
static zb_status_t zcl_closures_process_in_door_lock_clear_all_pin_codes_rsp(s_zb_zcl_incoming_msg_t *msg, s_zb_zcl_closures_door_lock_app_callbacks_t *cb);
static zb_status_t zcl_closures_process_in_door_lock_set_user_status_rsp(s_zb_zcl_incoming_msg_t *msg, s_zb_zcl_closures_door_lock_app_callbacks_t *cb);
static zb_status_t zcl_closures_process_in_door_lock_get_user_status_rsp(s_zb_zcl_incoming_msg_t *msg, s_zb_zcl_closures_door_lock_app_callbacks_t *cb);
static zb_status_t zcl_closures_process_in_door_lock_set_week_day_schedule_rsp(s_zb_zcl_incoming_msg_t *msg, s_zb_zcl_closures_door_lock_app_callbacks_t *cb);
static zb_status_t zcl_closures_process_in_door_lock_get_week_day_schedule_rsp(s_zb_zcl_incoming_msg_t *msg, s_zb_zcl_closures_door_lock_app_callbacks_t *cb);
static zb_status_t zcl_closures_process_in_door_lock_clear_week_day_schedule_rsp(s_zb_zcl_incoming_msg_t *msg, s_zb_zcl_closures_door_lock_app_callbacks_t *cb);
static zb_status_t zcl_closures_process_in_door_lock_set_year_day_schedule_rsp(s_zb_zcl_incoming_msg_t *msg, s_zb_zcl_closures_door_lock_app_callbacks_t *cb);
static zb_status_t zcl_closures_process_in_door_lock_get_year_day_schedule_rsp(s_zb_zcl_incoming_msg_t *msg, s_zb_zcl_closures_door_lock_app_callbacks_t *cb);
static zb_status_t zcl_closures_process_in_door_lock_clear_year_day_schedule_rsp(s_zb_zcl_incoming_msg_t *msg, s_zb_zcl_closures_door_lock_app_callbacks_t *cb);
static zb_status_t zcl_closures_process_in_door_lock_set_holiday_schedule_rsp(s_zb_zcl_incoming_msg_t *msg, s_zb_zcl_closures_door_lock_app_callbacks_t *cb);
static zb_status_t zcl_closures_process_in_door_lock_get_holiday_schedule_rsp(s_zb_zcl_incoming_msg_t *msg, s_zb_zcl_closures_door_lock_app_callbacks_t *cb);
static zb_status_t zcl_closures_process_in_door_lock_clear_holiday_schedule_rsp(s_zb_zcl_incoming_msg_t *msg, s_zb_zcl_closures_door_lock_app_callbacks_t *cb);
static zb_status_t zcl_closures_process_in_door_lock_set_user_type_rsp(s_zb_zcl_incoming_msg_t *msg, s_zb_zcl_closures_door_lock_app_callbacks_t *cb);
static zb_status_t zcl_closures_process_in_door_lock_get_user_type_rsp(s_zb_zcl_incoming_msg_t *msg, s_zb_zcl_closures_door_lock_app_callbacks_t *cb);
static zb_status_t zcl_closures_process_in_door_lock_set_rfid_code_rsp(s_zb_zcl_incoming_msg_t *msg, s_zb_zcl_closures_door_lock_app_callbacks_t *cb);
static zb_status_t zcl_closures_process_in_door_lock_get_rfid_code_rsp(s_zb_zcl_incoming_msg_t *msg, s_zb_zcl_closures_door_lock_app_callbacks_t *cb);
static zb_status_t zcl_closures_process_in_door_lock_clear_rfid_code_rsp(s_zb_zcl_incoming_msg_t *msg, s_zb_zcl_closures_door_lock_app_callbacks_t *cb);
static zb_status_t zcl_closures_process_in_door_lock_clear_all_rfid_codes_rsp(s_zb_zcl_incoming_msg_t *msg, s_zb_zcl_closures_door_lock_app_callbacks_t *cb);
static zb_status_t zcl_closures_process_in_door_lock_operation_event_notification(s_zb_zcl_incoming_msg_t *msg, s_zb_zcl_closures_door_lock_app_callbacks_t *cb);
static zb_status_t zcl_closures_process_in_door_lock_programming_event_notification(s_zb_zcl_incoming_msg_t *msg, s_zb_zcl_closures_door_lock_app_callbacks_t *cb);

/**
 * @brief Register Callbacks for Door Lock Cluster commands
 * 
 * @param[in] endpoint Endpoint
 * @param[in] callbacks Callbacks
 * @return zb_status_t Status of the registration
 */
zb_status_t
zb_zcl_closures_register_door_lock_cmd_callbacks(
    uint8_t endpoint, s_zb_zcl_closures_door_lock_app_callbacks_t *callbacks)
{
    s_zb_zcl_closures_door_lock_cb_rec_t *p_new_item;
    s_zb_zcl_closures_door_lock_cb_rec_t *p_loop;

    if (!g_zcl_closures_door_lock_plugin_registered)
    {
        zb_zcl_register_plugin(
            ZCL_CLUSTER_ID_CLOSURES_DOOR_LOCK,
            ZCL_CLUSTER_ID_CLOSURES_DOOR_LOCK,
            zcl_closures_handle_incoming);
        g_zcl_closures_door_lock_plugin_registered = true;
    }

    p_new_item = (s_zb_zcl_closures_door_lock_cb_rec_t *)ZB_MEM_MALLOC(sizeof(s_zb_zcl_closures_door_lock_cb_rec_t));
    if (p_new_item == NULL)
    {
        return ZB_MEM_ERROR;
    }
    p_new_item->next = NULL;
    p_new_item->endpoint = endpoint;
    p_new_item->cb = callbacks;

    if (g_zcl_closures_door_lock_cb_list == NULL)
    {
        g_zcl_closures_door_lock_cb_list = p_new_item;
    }
    else
    {
        p_loop = g_zcl_closures_door_lock_cb_list;
        while (p_loop->next != NULL)
        {
            p_loop = p_loop->next;
        }
        p_loop->next = p_new_item;
    }
    return ZB_SUCCESS;
}

/**
 * @brief Find the callbacks for the Door Lock Cluster commands
 * 
 * @param[in] endpoint Endpoint
 * @return s_zb_zcl_closures_door_lock_app_callbacks_t * Callbacks
 */
static s_zb_zcl_closures_door_lock_app_callbacks_t*
zcl_closures_find_door_lock_callbacks(uint8_t endpoint)
{
    s_zb_zcl_closures_door_lock_cb_rec_t *p_cb;
    p_cb = g_zcl_closures_door_lock_cb_list;
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
 * @brief Handle incoming messages for the Closure Cluster
 * 
 * @param[in] msg Incoming message
 * @return zb_status_t Status of the handling
 */
static zb_status_t
zcl_closures_handle_incoming(s_zb_zcl_incoming_msg_t *msg)
{
    zb_status_t status = ZB_SUCCESS;

    if (ZCL_CLUSTER_CMD(msg->hdr.fc.type))
    {
        if (msg->hdr.fc.manu_specific == 0)
        {
            status = zcl_closures_handle_in_specific_commands(msg);
        }
        else
        {
            // We don't support any manufacturer specific commands for now
            // Later we can add support for manufacturer specific commands by using registered callbacks
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
 * @brief Handle incoming specific commands for the Closure Cluster
 * 
 * @param[in] msg Incoming message
 * @return zb_status_t Status of the handling
 */
static zb_status_t
zcl_closures_handle_in_specific_commands(s_zb_zcl_incoming_msg_t *msg)
{
    zb_status_t status = ZB_FAILURE;
    s_zb_zcl_closures_door_lock_app_callbacks_t *cb;

    cb = zcl_closures_find_door_lock_callbacks(msg->msg->dst_endpoint);
    if (cb == NULL)
    {
        return ZB_FAILURE;
    }

    if (msg->msg->cluster_id == ZCL_CLUSTER_ID_CLOSURES_DOOR_LOCK)
    {
        status = zcl_closures_process_in_door_lock_cmds(msg, cb);
    }
    else
    {
        status = ZB_FAILURE;
    }
    return status;
}

/**
 * @brief Process incoming commands for the Door Lock Cluster
 * 
 * @param[in] msg Incoming message
 * @param[in] cb Callbacks
 * @return zb_status_t Status of the processing
 */
static zb_status_t
zcl_closures_process_in_door_lock_cmds(
    s_zb_zcl_incoming_msg_t *msg, s_zb_zcl_closures_door_lock_app_callbacks_t *cb)
{
    zb_status_t status;

    // Only process client commands
    if (ZCL_CLIENT_CMD(msg->hdr.fc.direction))
    {
        switch (msg->hdr.command_id)
        {
            case COMMAND_DOOR_LOCK_LOCK_DOOR_RESPONSE:
            case COMMAND_DOOR_LOCK_UNLOCK_DOOR_RESPONSE:
            case COMMAND_DOOR_LOCK_TOGGLE_RESPONSE:
                status = zcl_closures_process_in_door_lock_rsp(msg, cb);
                break;
            case COMMAND_DOOR_LOCK_UNLOCK_WITH_TIMEOUT_RESPONSE:
                status = zcl_closures_process_in_door_lock_unlock_with_timeout_rsp(msg, cb);
                break;
            case COMMAND_DOOR_LOCK_GET_LOG_RECORD_RESPONSE:
                status = zcl_closures_process_in_door_lock_get_log_record_rsp(msg, cb);
                break;
            case COMMAND_DOOR_LOCK_SET_PIN_CODE_RESPONSE:
                status = zcl_closures_process_in_door_lock_set_pin_code_rsp(msg, cb);
                break;
            case COMMAND_DOOR_LOCK_GET_PIN_CODE_RESPONSE:
                status = zcl_closures_process_in_door_lock_get_pin_code_rsp(msg, cb);
                break;
            case COMMAND_DOOR_LOCK_CLEAR_PIN_CODE_RESPONSE:
                status = zcl_closures_process_in_door_lock_clear_pin_code_rsp(msg, cb);
                break;
            case COMMAND_DOOR_LOCK_CLEAR_ALL_PIN_CODES_RESPONSE:
                status = zcl_closures_process_in_door_lock_clear_all_pin_codes_rsp(msg, cb);
                break;
            case COMMAND_DOOR_LOCK_SET_USER_STATUS_RESPONSE:
                status = zcl_closures_process_in_door_lock_set_user_status_rsp(msg, cb);
                break;
            case COMMAND_DOOR_LOCK_GET_USER_STATUS_RESPONSE:
                status = zcl_closures_process_in_door_lock_get_user_status_rsp(msg, cb);
                break;
            case COMMAND_DOOR_LOCK_SET_WEEKDAY_SCHEDULE_RESPONSE:
                status = zcl_closures_process_in_door_lock_set_week_day_schedule_rsp(msg, cb);
                break;
            case COMMAND_DOOR_LOCK_GET_WEEKDAY_SCHEDULE_RESPONSE:
                status = zcl_closures_process_in_door_lock_get_week_day_schedule_rsp(msg, cb);
                break;
            case COMMAND_DOOR_LOCK_CLEAR_WEEKDAY_SCHEDULE_RESPONSE:
                status = zcl_closures_process_in_door_lock_clear_week_day_schedule_rsp(msg, cb);
                break;
            case COMMAND_DOOR_LOCK_SET_YEAR_DAY_SCHEDULE_RESPONSE:
                status = zcl_closures_process_in_door_lock_set_year_day_schedule_rsp(msg, cb);
                break;
            case COMMAND_DOOR_LOCK_GET_YEAR_DAY_SCHEDULE_RESPONSE:
                status = zcl_closures_process_in_door_lock_get_year_day_schedule_rsp(msg, cb);
                break;
            case COMMAND_DOOR_LOCK_CLEAR_YEAR_DAY_SCHEDULE_RESPONSE:
                status = zcl_closures_process_in_door_lock_clear_year_day_schedule_rsp(msg, cb);
                break;
            case COMMAND_DOOR_LOCK_SET_HOLIDAY_SCHEDULE_RESPONSE:
                status = zcl_closures_process_in_door_lock_set_holiday_schedule_rsp(msg, cb);
                break;
            case COMMAND_DOOR_LOCK_GET_HOLIDAY_SCHEDULE_RESPONSE:
                status = zcl_closures_process_in_door_lock_get_holiday_schedule_rsp(msg, cb);
                break;
            case COMMAND_DOOR_LOCK_CLEAR_HOLIDAY_SCHEDULE_RESPONSE:
                status = zcl_closures_process_in_door_lock_clear_holiday_schedule_rsp(msg, cb);
                break;
            case COMMAND_DOOR_LOCK_SET_USER_TYPE_RESPONSE:
                status = zcl_closures_process_in_door_lock_set_user_type_rsp(msg, cb);
                break;
            case COMMAND_DOOR_LOCK_GET_USER_TYPE_RESPONSE:
                status = zcl_closures_process_in_door_lock_get_user_type_rsp(msg, cb);
                break;
            case COMMAND_DOOR_LOCK_SET_RFID_CODE_RESPONSE:
                status = zcl_closures_process_in_door_lock_set_rfid_code_rsp(msg, cb);
                break;
            case COMMAND_DOOR_LOCK_GET_RFID_CODE_RESPONSE:
                status = zcl_closures_process_in_door_lock_get_rfid_code_rsp(msg, cb);
                break;
            case COMMAND_DOOR_LOCK_CLEAR_RFID_CODE_RESPONSE:
                status = zcl_closures_process_in_door_lock_clear_rfid_code_rsp(msg, cb);
                break;
            case COMMAND_DOOR_LOCK_CLEAR_ALL_RFID_CODES_RESPONSE:
                status = zcl_closures_process_in_door_lock_clear_all_rfid_codes_rsp(msg, cb);
                break;
            case COMMAND_DOOR_LOCK_OPERATING_EVENT_NOTIFICATION:
                status = zcl_closures_process_in_door_lock_operation_event_notification(msg, cb);
                break;
            case COMMAND_DOOR_LOCK_PROGRAMMING_EVENT_NOTIFICATION:
                status = zcl_closures_process_in_door_lock_programming_event_notification(msg, cb);
                break;
            default:
                status = ZB_FAILURE;
                break;
        }
    }
    else
    {
        status = ZB_FAILURE;
    }
    return status;
}

static zb_status_t
zcl_closures_process_in_door_lock_rsp(
    s_zb_zcl_incoming_msg_t *msg, s_zb_zcl_closures_door_lock_app_callbacks_t *cb)
{
    if (cb->pfn_door_lock_lock_door_rsp != NULL)
    {
        return cb->pfn_door_lock_lock_door_rsp(msg, msg->data[0]);
    }

    return ZB_FAILURE;
}

static zb_status_t
zcl_closures_process_in_door_lock_unlock_with_timeout_rsp(
    s_zb_zcl_incoming_msg_t *msg, s_zb_zcl_closures_door_lock_app_callbacks_t *cb)
{
    if (cb->pfn_door_lock_unlock_with_timeout_rsp != NULL)
    {
        return cb->pfn_door_lock_unlock_with_timeout_rsp(msg, msg->data[0]);
    }

    return ZB_FAILURE;
}

static zb_status_t
zcl_closures_process_in_door_lock_get_log_record_rsp(
    s_zb_zcl_incoming_msg_t *msg, s_zb_zcl_closures_door_lock_app_callbacks_t *cb)
{
    zb_status_t status;

    if (cb->pfn_door_lock_get_log_record_rsp != NULL)
    {
        uint8_t offset;
        uint8_t calculated_array_len;
        s_zb_zcl_door_lock_get_log_record_rsp_t cmd;

        // First octet of PIN/RFID code variable string identifies the length of the string
        calculated_array_len = msg->data[11] + 1; // Add first byte of string

        cmd.pin = (uint8_t *)ZB_MEM_MALLOC(calculated_array_len);
        if (cmd.pin == NULL)
        {
            return ZB_MEM_ERROR;
        }

        cmd.log_index = BUILD_UINT16(msg->data[0], msg->data[1]);
        cmd.timestamp = BUILD_UINT32(msg->data[2], msg->data[3], msg->data[4], msg->data[5]);
        cmd.event_type = msg->data[6];
        cmd.source = msg->data[7];
        cmd.event_id_alarm_code = msg->data[8];
        cmd.user_id = BUILD_UINT16(msg->data[9], msg->data[10]);
        offset = 11;
        for (uint8_t i = 0; i < calculated_array_len; i++)
        {
            cmd.pin[i] = msg->data[offset++];
        }

        status = cb->pfn_door_lock_get_log_record_rsp(msg, &cmd);
        ZB_MEM_FREE(cmd.pin);
        return status;
    }

    return ZB_FAILURE;
}

static zb_status_t
zcl_closures_process_in_door_lock_set_pin_code_rsp(
    s_zb_zcl_incoming_msg_t *msg, s_zb_zcl_closures_door_lock_app_callbacks_t *cb)
{
    if (cb->pfn_door_lock_set_pin_code_rsp != NULL)
    {
        return cb->pfn_door_lock_set_pin_code_rsp(msg, msg->data[0]);
    }

    return ZB_FAILURE;
}

static zb_status_t
zcl_closures_process_in_door_lock_get_pin_code_rsp(
    s_zb_zcl_incoming_msg_t *msg, s_zb_zcl_closures_door_lock_app_callbacks_t *cb)
{
    zb_status_t status;

    if (cb->pfn_door_lock_get_pin_code_rsp != NULL)
    {
        uint8_t offset;
        uint8_t calculated_array_len;
        s_zb_zcl_door_lock_get_pin_code_rsp_t cmd;

        // First octet of PIN/RFID code variable string identifies the length of the string
        calculated_array_len = msg->data[4] + 1; // Add first byte of string

        cmd.code = (uint8_t *)ZB_MEM_MALLOC(calculated_array_len);
        if (cmd.code == NULL)
        {
            return ZB_MEM_ERROR;
        }
        cmd.user_id = BUILD_UINT16(msg->data[0], msg->data[1]);
        cmd.user_status = msg->data[2];
        cmd.user_type = msg->data[3];
        offset = 4;
        for (uint8_t i = 0; i < calculated_array_len; i++)
        {
            cmd.code[i] = msg->data[offset++];
        }

        status = cb->pfn_door_lock_get_pin_code_rsp(msg, &cmd);
        ZB_MEM_FREE(cmd.code);
        return status;
    }

    return ZB_FAILURE;
}

static zb_status_t
zcl_closures_process_in_door_lock_clear_pin_code_rsp(
    s_zb_zcl_incoming_msg_t *msg, s_zb_zcl_closures_door_lock_app_callbacks_t *cb)
{
    if (cb->pfn_door_lock_clear_pin_code_rsp != NULL)
    {
        return cb->pfn_door_lock_clear_pin_code_rsp(msg, msg->data[0]);
    }

    return ZB_FAILURE;
}

static zb_status_t
zcl_closures_process_in_door_lock_clear_all_pin_codes_rsp(
    s_zb_zcl_incoming_msg_t *msg, s_zb_zcl_closures_door_lock_app_callbacks_t *cb)
{
    if (cb->pfn_door_lock_clear_all_pin_codes_rsp != NULL)
    {
        return cb->pfn_door_lock_clear_all_pin_codes_rsp(msg, msg->data[0]);
    }

    return ZB_FAILURE;
}

static zb_status_t
zcl_closures_process_in_door_lock_set_user_status_rsp(
    s_zb_zcl_incoming_msg_t *msg, s_zb_zcl_closures_door_lock_app_callbacks_t *cb)
{
    if (cb->pfn_door_lock_set_user_status_rsp != NULL)
    {
        return cb->pfn_door_lock_set_user_status_rsp(msg, msg->data[0]);
    }

    return ZB_FAILURE;
}

static zb_status_t
zcl_closures_process_in_door_lock_get_user_status_rsp(
    s_zb_zcl_incoming_msg_t *msg, s_zb_zcl_closures_door_lock_app_callbacks_t *cb)
{
    if (cb->pfn_door_lock_get_user_status_rsp != NULL)
    {
        s_zb_zcl_door_lock_get_user_status_rsp_t cmd;
        cmd.user_id = BUILD_UINT16(msg->data[0], msg->data[1]);
        cmd.user_status = msg->data[2];

        return cb->pfn_door_lock_get_user_status_rsp(msg, &cmd);
    }

    return ZB_FAILURE;
}

static zb_status_t
zcl_closures_process_in_door_lock_set_week_day_schedule_rsp(
    s_zb_zcl_incoming_msg_t *msg, s_zb_zcl_closures_door_lock_app_callbacks_t *cb)
{
    if (cb->pfn_door_lock_set_week_day_schedule_rsp != NULL)
    {
        return cb->pfn_door_lock_set_week_day_schedule_rsp(msg, msg->data[0]);
    }

    return ZB_FAILURE;
}

static zb_status_t
zcl_closures_process_in_door_lock_get_week_day_schedule_rsp(
    s_zb_zcl_incoming_msg_t *msg, s_zb_zcl_closures_door_lock_app_callbacks_t *cb)
{
    if (cb->pfn_door_lock_get_week_day_schedule_rsp != NULL)
    {
        s_zb_zcl_door_lock_get_week_day_schedule_rsp_t cmd;
        cmd.schedule_id = msg->data[0];
        cmd.user_id = BUILD_UINT16(msg->data[1], msg->data[2]);
        cmd.status = msg->data[3];

        if (cmd.status == ZB_SUCCESS)
        {
            cmd.days_mask = msg->data[4];
            cmd.start_hour = msg->data[5];
            cmd.start_minute = msg->data[6];
            cmd.end_hour = msg->data[7];
            cmd.end_minute = msg->data[8];
        }

        return cb->pfn_door_lock_get_week_day_schedule_rsp(msg, &cmd);
    }

    return ZB_FAILURE;
}

static zb_status_t
zcl_closures_process_in_door_lock_clear_week_day_schedule_rsp(
    s_zb_zcl_incoming_msg_t *msg, s_zb_zcl_closures_door_lock_app_callbacks_t *cb)
{
    if (cb->pfn_door_lock_clear_week_day_schedule_rsp != NULL)
    {
        return cb->pfn_door_lock_clear_week_day_schedule_rsp(msg, msg->data[0]);
    }

    return ZB_FAILURE;
}

static zb_status_t
zcl_closures_process_in_door_lock_set_year_day_schedule_rsp(
    s_zb_zcl_incoming_msg_t *msg, s_zb_zcl_closures_door_lock_app_callbacks_t *cb)
{
    if (cb->pfn_door_lock_set_year_day_schedule_rsp != NULL)
    {
        return cb->pfn_door_lock_set_year_day_schedule_rsp(msg, msg->data[0]);
    }

    return ZB_FAILURE;
}

static zb_status_t
zcl_closures_process_in_door_lock_get_year_day_schedule_rsp(
    s_zb_zcl_incoming_msg_t *msg, s_zb_zcl_closures_door_lock_app_callbacks_t *cb)
{
    if (cb->pfn_door_lock_get_year_day_schedule_rsp != NULL)
    {
        s_zb_zcl_door_lock_get_year_day_schedule_rsp_t cmd;
        cmd.schedule_id = msg->data[0];
        cmd.user_id = BUILD_UINT16(msg->data[1], msg->data[2]);
        cmd.status = msg->data[3];

        if (cmd.status == ZB_SUCCESS)
        {
            cmd.zigbee_local_start_time = BUILD_UINT32(msg->data[4], msg->data[5], msg->data[6], msg->data[7]);
            cmd.zigbee_local_end_time = BUILD_UINT32(msg->data[8], msg->data[9], msg->data[10], msg->data[11]);
        }

        return cb->pfn_door_lock_get_year_day_schedule_rsp(msg, &cmd);
    }

    return ZB_FAILURE;
}

static zb_status_t
zcl_closures_process_in_door_lock_clear_year_day_schedule_rsp(
    s_zb_zcl_incoming_msg_t *msg, s_zb_zcl_closures_door_lock_app_callbacks_t *cb)
{
    if (cb->pfn_door_lock_clear_year_day_schedule_rsp != NULL)
    {
        return cb->pfn_door_lock_clear_year_day_schedule_rsp(msg, msg->data[0]);
    }

    return ZB_FAILURE;
}

static zb_status_t
zcl_closures_process_in_door_lock_set_holiday_schedule_rsp(
    s_zb_zcl_incoming_msg_t *msg, s_zb_zcl_closures_door_lock_app_callbacks_t *cb)
{
    if (cb->pfn_door_lock_set_holiday_schedule_rsp != NULL)
    {
        return cb->pfn_door_lock_set_holiday_schedule_rsp(msg, msg->data[0]);
    }

    return ZB_FAILURE;
}

static zb_status_t
zcl_closures_process_in_door_lock_get_holiday_schedule_rsp(
    s_zb_zcl_incoming_msg_t *msg, s_zb_zcl_closures_door_lock_app_callbacks_t *cb)
{
    if (cb->pfn_door_lock_get_holiday_schedule_rsp != NULL)
    {
        s_zb_zcl_door_lock_get_holiday_schedule_rsp_t cmd;
        cmd.schedule_id = msg->data[0];
        cmd.status = msg->data[1];

        if (cmd.status == ZB_SUCCESS)
        {
            cmd.zigbee_local_start_time = BUILD_UINT32(msg->data[2], msg->data[3], msg->data[4], msg->data[5]);
            cmd.zigbee_local_end_time = BUILD_UINT32(msg->data[6], msg->data[7], msg->data[8], msg->data[9]);
            cmd.operating_mode_during_holiday = msg->data[10];
        }

        return cb->pfn_door_lock_get_holiday_schedule_rsp(msg, &cmd);
    }

    return ZB_FAILURE;
}

static zb_status_t
zcl_closures_process_in_door_lock_clear_holiday_schedule_rsp(
    s_zb_zcl_incoming_msg_t *msg, s_zb_zcl_closures_door_lock_app_callbacks_t *cb)
{
    if (cb->pfn_door_lock_clear_holiday_schedule_rsp != NULL)
    {
        return cb->pfn_door_lock_clear_holiday_schedule_rsp(msg, msg->data[0]);
    }

    return ZB_FAILURE;
}

static zb_status_t
zcl_closures_process_in_door_lock_set_user_type_rsp(
    s_zb_zcl_incoming_msg_t *msg, s_zb_zcl_closures_door_lock_app_callbacks_t *cb)
{
    if (cb->pfn_door_lock_set_user_type_rsp != NULL)
    {
        return cb->pfn_door_lock_set_user_type_rsp(msg, msg->data[0]);
    }

    return ZB_FAILURE;
}

static zb_status_t
zcl_closures_process_in_door_lock_get_user_type_rsp(
    s_zb_zcl_incoming_msg_t *msg, s_zb_zcl_closures_door_lock_app_callbacks_t *cb)
{
    if (cb->pfn_door_lock_get_user_type_rsp != NULL)
    {
        s_zb_zcl_door_lock_get_user_type_rsp_t cmd;
        cmd.user_id = BUILD_UINT16(msg->data[0], msg->data[1]);
        cmd.user_type = msg->data[2];

        return cb->pfn_door_lock_get_user_type_rsp(msg, &cmd);
    }

    return ZB_FAILURE;
}

static zb_status_t
zcl_closures_process_in_door_lock_set_rfid_code_rsp(
    s_zb_zcl_incoming_msg_t *msg, s_zb_zcl_closures_door_lock_app_callbacks_t *cb)
{
    if (cb->pfn_door_lock_set_rfid_code_rsp != NULL)
    {
        return cb->pfn_door_lock_set_rfid_code_rsp(msg, msg->data[0]);
    }

    return ZB_FAILURE;
}

static zb_status_t
zcl_closures_process_in_door_lock_get_rfid_code_rsp(
    s_zb_zcl_incoming_msg_t *msg, s_zb_zcl_closures_door_lock_app_callbacks_t *cb)
{
    zb_status_t status;

    if (cb->pfn_door_lock_get_rfid_code_rsp != NULL)
    {
        uint8_t offset;
        uint8_t calculated_array_len;
        s_zb_zcl_door_lock_get_rfid_code_rsp_t cmd;

        // First octet of RFID code variable string identifies the length of the string
        calculated_array_len = msg->data[4] + 1; // Add first byte of string

        cmd.rfid_code = (uint8_t *)ZB_MEM_MALLOC(calculated_array_len);
        if (cmd.rfid_code == NULL)
        {
            return ZB_MEM_ERROR;
        }
        cmd.user_id = BUILD_UINT16(msg->data[0], msg->data[1]);
        cmd.user_status = msg->data[2];
        cmd.user_type = msg->data[3];
        offset = 4;
        for (uint8_t i = 0; i < calculated_array_len; i++)
        {
            cmd.rfid_code[i] = msg->data[offset++];
        }

        status = cb->pfn_door_lock_get_rfid_code_rsp(msg, &cmd);
        ZB_MEM_FREE(cmd.rfid_code);
        return status;
    }

    return ZB_FAILURE;
}

static zb_status_t
zcl_closures_process_in_door_lock_clear_rfid_code_rsp(
    s_zb_zcl_incoming_msg_t *msg, s_zb_zcl_closures_door_lock_app_callbacks_t *cb)
{
    if (cb->pfn_door_lock_clear_rfid_code_rsp != NULL)
    {
        return cb->pfn_door_lock_clear_rfid_code_rsp(msg, msg->data[0]);
    }

    return ZB_FAILURE;
}

static zb_status_t
zcl_closures_process_in_door_lock_clear_all_rfid_codes_rsp(
    s_zb_zcl_incoming_msg_t *msg, s_zb_zcl_closures_door_lock_app_callbacks_t *cb)
{
    if (cb->pfn_door_lock_clear_all_rfid_codes_rsp != NULL)
    {
        return cb->pfn_door_lock_clear_all_rfid_codes_rsp(msg, msg->data[0]);
    }

    return ZB_FAILURE;
}

static zb_status_t
zcl_closures_process_in_door_lock_operation_event_notification(
    s_zb_zcl_incoming_msg_t *msg, s_zb_zcl_closures_door_lock_app_callbacks_t *cb)
{
    uint8_t offset;
    uint8_t calculated_array_len;
    s_zb_zcl_door_lock_operating_event_notification_t cmd;
    zb_status_t status;

    if (cb->pfn_door_lock_operating_event_notification != NULL)
    {
        calculated_array_len = msg->data[9] + 1;

        cmd.data = (uint8_t *)ZB_MEM_MALLOC(calculated_array_len);
        if (cmd.data == NULL)
        {
            return ZB_MEM_ERROR;
        }
        cmd.event_source = msg->data[0];
        cmd.event_code = msg->data[1];
        cmd.user_id = BUILD_UINT16(msg->data[2], msg->data[3]);
        cmd.pin = msg->data[4];
        cmd.zigbee_local_time = BUILD_UINT32(msg->data[5], msg->data[6], msg->data[7], msg->data[8]);
        offset = 9;
        for (uint8_t i = 0; i < calculated_array_len; i++)
        {
            cmd.data[i] = msg->data[offset++];
        }

        status = cb->pfn_door_lock_operating_event_notification(msg, &cmd);
        ZB_MEM_FREE(cmd.data);
        return status;
    }

    return ZB_FAILURE;
}

static zb_status_t
zcl_closures_process_in_door_lock_programming_event_notification(
    s_zb_zcl_incoming_msg_t *msg, s_zb_zcl_closures_door_lock_app_callbacks_t *cb)
{
    uint8_t offset;
    uint8_t calculated_array_len;
    s_zb_zcl_door_lock_programming_event_notification_t cmd;
    zb_status_t status;

    if (cb->pfn_door_lock_programming_event_notification != NULL)
    {
        calculated_array_len = msg->data[11] + 1;

        cmd.data = (uint8_t *)ZB_MEM_MALLOC(calculated_array_len);
        if (cmd.data == NULL)
        {
            return ZB_MEM_ERROR;
        }
        cmd.event_source = msg->data[0];
        cmd.event_code = msg->data[1];
        cmd.user_id = BUILD_UINT16(msg->data[2], msg->data[3]);
        cmd.pin = msg->data[4];
        cmd.user_type = msg->data[5];
        cmd.user_status = msg->data[6];
        cmd.zigbee_local_time = BUILD_UINT32(msg->data[7], msg->data[8], msg->data[9], msg->data[10]);
        offset = 11;
        for (uint8_t i = 0; i < calculated_array_len; i++)
        {
            cmd.data[i] = msg->data[offset++];
        }

        status = cb->pfn_door_lock_programming_event_notification(msg, &cmd);
        ZB_MEM_FREE(cmd.data);
        return status;
    }

    return ZB_FAILURE;
}

zb_status_t
zb_zcl_closures_send_door_lock_request(
    uint8_t src_ep, s_zb_af_address_t *dst_addr, uint8_t cmd,
    s_zb_zcl_door_lock_t *payload, uint8_t disable_default_rsp, uint8_t seq_num)
{
    zb_status_t status;
    uint8_t *pbuf;
    uint8_t calculated_buf_size;

    calculated_buf_size = payload->pin_rfid_code[0] + 1;

    pbuf = (uint8_t *)ZB_MEM_MALLOC(calculated_buf_size);
    if (pbuf == NULL)
    {
        return ZB_MEM_ERROR;
    }
    pbuf[0] = payload->pin_rfid_code[0];
    for (uint8_t i = 0; i < calculated_buf_size; i++)
    {
        pbuf[i + 1] = payload->pin_rfid_code[i];
    }

    status = zb_zcl_send_cmd(src_ep, dst_addr, ZCL_CLUSTER_ID_CLOSURES_DOOR_LOCK,
                cmd, true, ZCL_FRAME_CLIENT_SERVER_DIR,
                disable_default_rsp, 0, seq_num, calculated_buf_size, pbuf);
    ZB_MEM_FREE(pbuf);
    return status;
}

zb_status_t
zb_zcl_closures_send_door_lock_unlock_with_timeout_request(
    uint8_t src_ep, s_zb_af_address_t *dst_addr,
    s_zb_zcl_door_lock_unlock_timeout_t *payload,
    uint8_t disable_default_rsp, uint8_t seq_num)
{
    zb_status_t status;
    uint8_t *pbuf;
    uint8_t offset;
    uint8_t calculated_array_len;
    uint8_t calculated_buf_size;

    calculated_array_len = payload->pin_rfid_code[0] + 1;
    calculated_buf_size = calculated_array_len + PAYLOAD_LEN_UNLOCK_TIMEOUT;

    pbuf = (uint8_t *)ZB_MEM_MALLOC(calculated_buf_size);
    if (pbuf == NULL)
    {
        return ZB_MEM_ERROR;
    }
    pbuf[0] = LO_UINT16(payload->timeout);
    pbuf[1] = HI_UINT16(payload->timeout);
    offset = 2;
    for (uint8_t i = 0; i < calculated_array_len; i++)
    {
        pbuf[offset++] = payload->pin_rfid_code[i];
    }

    status = zb_zcl_send_cmd(src_ep, dst_addr, ZCL_CLUSTER_ID_CLOSURES_DOOR_LOCK,
                COMMAND_DOOR_LOCK_UNLOCK_WITH_TIMEOUT, true, ZCL_FRAME_CLIENT_SERVER_DIR,
                disable_default_rsp, 0, seq_num, calculated_buf_size, pbuf);
    ZB_MEM_FREE(pbuf);
    return status;
}

zb_status_t
zb_zcl_closures_send_door_lock_get_log_record_request(
    uint8_t src_ep, s_zb_af_address_t *dst_addr,
    uint16_t log_index, uint8_t disable_default_rsp, uint8_t seq_num)
{
    uint8_t payload[PAYLOAD_LEN_GET_LOG_RECORD];

    payload[0] = LO_UINT16(log_index);
    payload[1] = HI_UINT16(log_index);

    return zb_zcl_send_cmd(src_ep, dst_addr, ZCL_CLUSTER_ID_CLOSURES_DOOR_LOCK,
                COMMAND_DOOR_LOCK_GET_LOG_RECORD, true, ZCL_FRAME_CLIENT_SERVER_DIR,
                disable_default_rsp, 0, seq_num, PAYLOAD_LEN_GET_LOG_RECORD, payload);
}

zb_status_t
zb_zcl_closures_send_door_lock_set_pin_code_request(
    uint8_t src_ep, s_zb_af_address_t *dst_addr,
    s_zb_zcl_door_lock_set_pin_code_t *payload,
    uint8_t disable_default_rsp, uint8_t seq_num)
{
    zb_status_t status;
    uint8_t *pbuf;
    uint8_t offset;
    uint8_t calculated_array_len;
    uint8_t calculated_buf_size;

    calculated_array_len = payload->pin[0] + 1;
    calculated_buf_size = calculated_array_len + PAYLOAD_LEN_SET_PIN_CODE;

    pbuf = (uint8_t *)ZB_MEM_MALLOC(calculated_buf_size);
    if (pbuf == NULL)
    {
        return ZB_MEM_ERROR;
    }
    pbuf[0] = LO_UINT16(payload->user_id);
    pbuf[1] = HI_UINT16(payload->user_id);
    pbuf[2] = payload->user_status;
    pbuf[3] = payload->user_type;
    offset = 4;
    for (uint8_t i = 0; i < calculated_array_len; i++)
    {
        pbuf[offset++] = payload->pin[i];
    }

    status = zb_zcl_send_cmd(src_ep, dst_addr, ZCL_CLUSTER_ID_CLOSURES_DOOR_LOCK,
                COMMAND_DOOR_LOCK_SET_PIN_CODE, true, ZCL_FRAME_CLIENT_SERVER_DIR,
                disable_default_rsp, 0, seq_num, calculated_buf_size, pbuf);
    ZB_MEM_FREE(pbuf);
    return status;
}

zb_status_t
zb_zcl_closures_send_door_lock_user_id_request(
    uint8_t src_ep, s_zb_af_address_t *dst_addr, uint8_t cmd,
    uint16_t user_id, uint8_t disable_default_rsp, uint8_t seq_num)
{
    uint8_t payload[PAYLOAD_LEN_USER_ID];

    payload[0] = LO_UINT16(user_id);
    payload[1] = HI_UINT16(user_id);

    return zb_zcl_send_cmd(src_ep, dst_addr, ZCL_CLUSTER_ID_CLOSURES_DOOR_LOCK,
                cmd, true, ZCL_FRAME_CLIENT_SERVER_DIR,
                disable_default_rsp, 0, seq_num, PAYLOAD_LEN_USER_ID, payload);
}

zb_status_t
zb_zcl_closures_send_door_lock_clear_all_codes_request(
    uint8_t src_ep, s_zb_af_address_t *dst_addr,
    uint8_t cmd, uint8_t disable_default_rsp, uint8_t seq_num)
{
    return zb_zcl_send_cmd(src_ep, dst_addr, ZCL_CLUSTER_ID_CLOSURES_DOOR_LOCK,
                cmd, true, ZCL_FRAME_CLIENT_SERVER_DIR,
                disable_default_rsp, 0, seq_num, 0, NULL);
}

zb_status_t
zb_zcl_closures_send_door_lock_set_user_status_request(
    uint8_t src_ep, s_zb_af_address_t *dst_addr,
    uint16_t user_id, uint8_t user_status, uint8_t disable_default_rsp, uint8_t seq_num)
{
    uint8_t payload[PAYLOAD_LEN_SET_USER_STATUS];

    payload[0] = LO_UINT16(user_id);
    payload[1] = HI_UINT16(user_id);
    payload[2] = user_status;

    return zb_zcl_send_cmd(src_ep, dst_addr, ZCL_CLUSTER_ID_CLOSURES_DOOR_LOCK,
                COMMAND_DOOR_LOCK_SET_USER_STATUS, true, ZCL_FRAME_CLIENT_SERVER_DIR,
                disable_default_rsp, 0, seq_num, PAYLOAD_LEN_SET_USER_STATUS, payload);
}

zb_status_t
zb_zcl_closures_send_door_lock_set_weekday_schedule_request(
    uint8_t src_ep, s_zb_af_address_t *dst_addr,
    uint8_t schedule_id, uint16_t user_id, uint8_t days_mask,
    uint8_t start_hour, uint8_t start_minute, uint8_t end_hour,
    uint8_t end_minute, uint8_t disable_default_rsp, uint8_t seq_num)
{
    uint8_t payload[PAYLOAD_LEN_SET_WEEK_DAY_SCHEDULE];

    payload[0] = schedule_id;
    payload[1] = LO_UINT16(user_id);
    payload[2] = HI_UINT16(user_id);
    payload[3] = days_mask;
    payload[4] = start_hour;
    payload[5] = start_minute;
    payload[6] = end_hour;
    payload[7] = end_minute;

    return zb_zcl_send_cmd(src_ep, dst_addr, ZCL_CLUSTER_ID_CLOSURES_DOOR_LOCK,
                COMMAND_DOOR_LOCK_SET_WEEKDAY_SCHEDULE, true, ZCL_FRAME_CLIENT_SERVER_DIR,
                disable_default_rsp, 0, seq_num, PAYLOAD_LEN_SET_WEEK_DAY_SCHEDULE, payload);
}

zb_status_t
zb_zcl_closures_send_door_lock_schedule_request(
    uint8_t src_ep, s_zb_af_address_t *dst_addr, uint8_t cmd,
    uint8_t schedule_id, uint16_t user_id, uint8_t disable_default_rsp, uint8_t seq_num)
{
    uint8_t payload[PAYLOAD_LEN_SCHEDULE];

    payload[0] = schedule_id;
    payload[1] = LO_UINT16(user_id);
    payload[2] = HI_UINT16(user_id);

    return zb_zcl_send_cmd(src_ep, dst_addr, ZCL_CLUSTER_ID_CLOSURES_DOOR_LOCK,
                cmd, true, ZCL_FRAME_CLIENT_SERVER_DIR,
                disable_default_rsp, 0, seq_num, PAYLOAD_LEN_SCHEDULE, payload);
}

zb_status_t
zb_zcl_closures_send_door_lock_set_year_day_schedule_request(
    uint8_t src_ep, s_zb_af_address_t *dst_addr,
    uint8_t schedule_id, uint16_t user_id, uint32_t zigbee_local_start_time,
    uint32_t zigbee_local_end_time, uint8_t disable_default_rsp, uint8_t seq_num)
{
    uint8_t payload[PAYLOAD_LEN_SET_YEAR_DAY_SCHEDULE];

    payload[0] = schedule_id;
    payload[1] = LO_UINT16(user_id);
    payload[2] = HI_UINT16(user_id);
    payload[3] = BREAK_UINT32(zigbee_local_start_time, 0);
    payload[4] = BREAK_UINT32(zigbee_local_start_time, 1);
    payload[5] = BREAK_UINT32(zigbee_local_start_time, 2);
    payload[6] = BREAK_UINT32(zigbee_local_start_time, 3);
    payload[7] = BREAK_UINT32(zigbee_local_end_time, 0);
    payload[8] = BREAK_UINT32(zigbee_local_end_time, 1);
    payload[9] = BREAK_UINT32(zigbee_local_end_time, 2);
    payload[10] = BREAK_UINT32(zigbee_local_end_time, 3);

    return zb_zcl_send_cmd(src_ep, dst_addr, ZCL_CLUSTER_ID_CLOSURES_DOOR_LOCK,
                COMMAND_DOOR_LOCK_SET_YEAR_DAY_SCHEDULE, true, ZCL_FRAME_CLIENT_SERVER_DIR,
                disable_default_rsp, 0, seq_num, PAYLOAD_LEN_SET_YEAR_DAY_SCHEDULE, payload);
}

zb_status_t
zb_zcl_closures_send_door_lock_set_holiday_schedule_request(
    uint8_t src_ep, s_zb_af_address_t *dst_addr,
    uint8_t schedule_id, uint32_t zigbee_local_start_time,
    uint32_t zigbee_local_end_time, uint8_t operating_mode_during_holiday,
    uint8_t disable_default_rsp, uint8_t seq_num)
{
    uint8_t payload[PAYLOAD_LEN_SET_HOLIDAY_SCHEDULE];

    payload[0] = schedule_id;
    payload[1] = BREAK_UINT32(zigbee_local_start_time, 0);
    payload[2] = BREAK_UINT32(zigbee_local_start_time, 1);
    payload[3] = BREAK_UINT32(zigbee_local_start_time, 2);
    payload[4] = BREAK_UINT32(zigbee_local_start_time, 3);
    payload[5] = BREAK_UINT32(zigbee_local_end_time, 0);
    payload[6] = BREAK_UINT32(zigbee_local_end_time, 1);
    payload[7] = BREAK_UINT32(zigbee_local_end_time, 2);
    payload[8] = BREAK_UINT32(zigbee_local_end_time, 3);
    payload[9] = operating_mode_during_holiday;

    return zb_zcl_send_cmd(src_ep, dst_addr, ZCL_CLUSTER_ID_CLOSURES_DOOR_LOCK,
                COMMAND_DOOR_LOCK_SET_HOLIDAY_SCHEDULE, true, ZCL_FRAME_CLIENT_SERVER_DIR,
                disable_default_rsp, 0, seq_num, PAYLOAD_LEN_SET_HOLIDAY_SCHEDULE, payload);
}

zb_status_t
zb_zcl_closures_send_door_lock_holiday_schedule_request(
    uint8_t src_ep, s_zb_af_address_t *dst_addr, uint8_t cmd,
    uint8_t schedule_id, uint8_t disable_default_rsp, uint8_t seq_num)
{
    uint8_t payload[PAYLOAD_LEN_HOLIDAY_SCHEDULE];

    payload[0] = schedule_id;

    return zb_zcl_send_cmd(src_ep, dst_addr, ZCL_CLUSTER_ID_CLOSURES_DOOR_LOCK,
                cmd, true, ZCL_FRAME_CLIENT_SERVER_DIR,
                disable_default_rsp, 0, seq_num, PAYLOAD_LEN_HOLIDAY_SCHEDULE, payload);
}

zb_status_t
zb_zcl_closures_send_door_lock_set_user_type_request(
    uint8_t src_ep, s_zb_af_address_t *dst_addr,
    uint16_t user_id, uint8_t user_type, uint8_t disable_default_rsp, uint8_t seq_num)
{
    uint8_t payload[PAYLOAD_LEN_SET_USER_TYPE];

    payload[0] = LO_UINT16(user_id);
    payload[1] = HI_UINT16(user_id);
    payload[2] = user_type;

    return zb_zcl_send_cmd(src_ep, dst_addr, ZCL_CLUSTER_ID_CLOSURES_DOOR_LOCK,
                COMMAND_DOOR_LOCK_SET_USER_TYPE, true, ZCL_FRAME_CLIENT_SERVER_DIR,
                disable_default_rsp, 0, seq_num, PAYLOAD_LEN_SET_USER_TYPE, payload);
}

zb_status_t
zb_zcl_closures_send_door_lock_set_rfid_code_request(
    uint8_t src_ep, s_zb_af_address_t *dst_addr,
    s_zb_zcl_door_lock_set_rfid_code_t *payload,
    uint8_t disable_default_rsp, uint8_t seq_num)
{
    zb_status_t status;
    uint8_t *pbuf;
    uint8_t offset;
    uint8_t calculated_array_len;
    uint8_t calculated_buf_size;

    calculated_array_len = payload->rfid_code[0] + 1;
    calculated_buf_size = calculated_array_len + PAYLOAD_LEN_SET_RFID_CODE;

    pbuf = (uint8_t *)ZB_MEM_MALLOC(calculated_buf_size);
    if (pbuf == NULL)
    {
        return ZB_MEM_ERROR;
    }
    pbuf[0] = LO_UINT16(payload->user_id);
    pbuf[1] = HI_UINT16(payload->user_id);
    pbuf[2] = payload->user_status;
    pbuf[3] = payload->user_type;
    offset = 4;
    for (uint8_t i = 0; i < calculated_array_len; i++)
    {
        pbuf[offset++] = payload->rfid_code[i];
    }

    status = zb_zcl_send_cmd(src_ep, dst_addr, ZCL_CLUSTER_ID_CLOSURES_DOOR_LOCK,
                COMMAND_DOOR_LOCK_SET_RFID_CODE, true, ZCL_FRAME_CLIENT_SERVER_DIR,
                disable_default_rsp, 0, seq_num, calculated_buf_size, pbuf);
    ZB_MEM_FREE(pbuf);
    return status;
}

/**
 * @brief ZCL Window Covering Cluster - Client commands
 * 
 */

/**
 * @brief Send a Window Covering Simple request
 * 
 * @param[in] src_ep Source endpoint
 * @param[in] dst_addr Destination address
 * @param[in] cmd Command to send
 * @param[in] disable_default_rsp Whether to disable the default response
 * @param[in] seq_num Sequence number
 * @return zb_status_t Status of the command
 */
zb_status_t
zb_zcl_closures_window_covering_simple_request(
    uint8_t src_ep, s_zb_af_address_t *dst_addr, uint8_t cmd,
    uint8_t disable_default_rsp, uint8_t seq_num)
{
    return zb_zcl_send_cmd(src_ep, dst_addr, ZCL_CLUSTER_ID_CLOSURES_WINDOW_COVERING,
                cmd, true, ZCL_FRAME_CLIENT_SERVER_DIR,
                disable_default_rsp, 0, seq_num, 0, NULL);
}

/**
 * @brief Send a Window Covering Send Go To Value request
 * 
 * @param[in] src_ep Source endpoint
 * @param[in] dst_addr Destination address
 * @param[in] cmd Command for COMMAND_WINDOW_COVERING_GO_TO_LIFT_VALUE
 * @param[in] value Value to send
 * @param[in] disable_default_rsp Whether to disable the default response
 * @param[in] seq_num Sequence number
 * @return zb_status_t Status of the command
 */
zb_status_t
zb_zcl_closures_window_covering_send_goto_value_request(
    uint8_t src_ep, s_zb_af_address_t *dst_addr, uint8_t cmd,
    uint16_t value, uint8_t disable_default_rsp, uint8_t seq_num)
{
    uint8_t payload[ZCL_WC_GOTOVALUEREQ_PAYLOADLEN];

    payload[0] = LO_UINT16(value);
    payload[1] = HI_UINT16(value);

    return zb_zcl_send_cmd(src_ep, dst_addr, ZCL_CLUSTER_ID_CLOSURES_WINDOW_COVERING,
                cmd, true, ZCL_FRAME_CLIENT_SERVER_DIR,
                disable_default_rsp, 0, seq_num, ZCL_WC_GOTOVALUEREQ_PAYLOADLEN, payload);
}

zb_status_t
zb_zcl_closures_window_covering_send_goto_percentage_request(
    uint8_t src_ep, s_zb_af_address_t *dst_addr, uint8_t cmd,
    uint8_t percentage_value, uint8_t disable_default_rsp, uint8_t seq_num)
{
    uint8_t payload[ZCL_WC_GOTOPERCENTAGEREQ_PAYLOADLEN];

    payload[0] = percentage_value;

    return zb_zcl_send_cmd(src_ep, dst_addr, ZCL_CLUSTER_ID_CLOSURES_WINDOW_COVERING,
                cmd, true, ZCL_FRAME_CLIENT_SERVER_DIR,
                disable_default_rsp, 0, seq_num, ZCL_WC_GOTOPERCENTAGEREQ_PAYLOADLEN, payload);
}

