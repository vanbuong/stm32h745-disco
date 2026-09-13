#include "zcl/zb_zcl.h"
#include "zcl/zb_zcl_general.h"
#include "zcl/zb_zcl_poll_control.h"
#include "common/zb_common.h"

/*********************************************************************
 * TYPEDEFS
 */
typedef struct s_zb_zcl_poll_control_cb_rec
{
    struct s_zb_zcl_poll_control_cb_rec *next;
    uint8_t endpoint;
    s_zb_zcl_poll_control_app_callbacks_t *cb;
} s_zb_zcl_poll_control_cb_rec_t;

/*********************************************************************
 * LOCAL VARIABLES
 */
static s_zb_zcl_poll_control_cb_rec_t *g_zcl_poll_control_cb_list = NULL;
static uint8_t g_zcl_poll_control_plugin_registered = false;

/*********************************************************************
 * LOCAL FUNCTIONS
 */
static zb_status_t zcl_poll_control_handle_incoming(s_zb_zcl_incoming_msg_t *msg);
static zb_status_t zcl_poll_control_handle_in_specific_commands(s_zb_zcl_incoming_msg_t *msg);
static s_zb_zcl_poll_control_app_callbacks_t *zcl_poll_control_find_app_callbacks(uint8_t endpoint);
static zb_status_t zcl_poll_control_process_in_cmds(
    s_zb_zcl_incoming_msg_t *msg, s_zb_zcl_poll_control_app_callbacks_t *cb);

static zb_status_t zcl_poll_control_process_in_cmd_check_in(
    s_zb_zcl_incoming_msg_t *msg, s_zb_zcl_poll_control_app_callbacks_t *cb);

zb_status_t
zb_zcl_poll_control_register_cmd_callback(
    uint8_t endpoint, s_zb_zcl_poll_control_app_callbacks_t *callbacks)
{
    s_zb_zcl_poll_control_cb_rec_t *p_new_item;
    s_zb_zcl_poll_control_cb_rec_t *p_loop;

    if (!g_zcl_poll_control_plugin_registered)
    {
        zb_zcl_register_plugin(ZCL_CLUSTER_ID_GENERAL_POLL_CONTROL,
                                            ZCL_CLUSTER_ID_GENERAL_POLL_CONTROL,
                                            zcl_poll_control_handle_incoming);
        g_zcl_poll_control_plugin_registered = true;
    }

    p_new_item = (s_zb_zcl_poll_control_cb_rec_t *)ZB_MEM_MALLOC(sizeof(s_zb_zcl_poll_control_cb_rec_t));
    if (p_new_item == NULL)
    {
        return ZB_MEM_ERROR;
    }
    p_new_item->next = NULL;
    p_new_item->endpoint = endpoint;
    p_new_item->cb = callbacks;

    if (g_zcl_poll_control_cb_list == NULL)
    {
        g_zcl_poll_control_cb_list = p_new_item;
    }
    else
    {
        p_loop = g_zcl_poll_control_cb_list;
        while (p_loop->next != NULL)
        {
            p_loop = p_loop->next;
        }
        p_loop->next = p_new_item;
    }
    return ZB_SUCCESS;
}

zb_status_t
zb_zcl_poll_control_send_check_in_rsp(
    uint8_t src_ep, s_zb_af_address_t *dst_addr,
    uint8_t start_fast_polling, uint16_t fast_poll_timeout,
    uint8_t disable_default_rsp, uint8_t seq_num)
{
    uint8_t buf[3];

    buf[0] = start_fast_polling;
    buf[1] = LO_UINT16(fast_poll_timeout);
    buf[2] = HI_UINT16(fast_poll_timeout);
    return zb_zcl_send_cmd(
                src_ep, dst_addr, ZCL_CLUSTER_ID_GENERAL_POLL_CONTROL,
                COMMAND_POLL_CONTROL_CHECK_IN_RSP, true, ZCL_FRAME_CLIENT_SERVER_DIR,
                disable_default_rsp, 0, seq_num, 3, buf);
}

zb_status_t
zb_zcl_poll_control_send_fast_poll_stop(
    uint8_t src_ep, s_zb_af_address_t *dst_addr,
    uint8_t disable_default_rsp, uint8_t seq_num)
{
    return zb_zcl_send_cmd(
                src_ep, dst_addr, ZCL_CLUSTER_ID_GENERAL_POLL_CONTROL,
                COMMAND_POLL_CONTROL_FAST_POLL_STOP, true, ZCL_FRAME_CLIENT_SERVER_DIR,
                disable_default_rsp, 0, seq_num, 0, NULL);
}

zb_status_t
zb_zcl_poll_control_send_set_long_poll_interval(
    uint8_t src_ep, s_zb_af_address_t *dst_addr,
    uint32_t new_long_poll_interval,
    uint8_t disable_default_rsp, uint8_t seq_num)
{
    uint8_t buf[4];
    buf[0] = BREAK_UINT32(new_long_poll_interval, 0);
    buf[1] = BREAK_UINT32(new_long_poll_interval, 1);
    buf[2] = BREAK_UINT32(new_long_poll_interval, 2);
    buf[3] = BREAK_UINT32(new_long_poll_interval, 3);
    return zb_zcl_send_cmd(
                src_ep, dst_addr, ZCL_CLUSTER_ID_GENERAL_POLL_CONTROL,
                COMMAND_POLL_CONTROL_SET_LONG_POLL_INTERVAL, true, ZCL_FRAME_CLIENT_SERVER_DIR,
                disable_default_rsp, 0, seq_num, 4, buf);
}

zb_status_t
zb_zcl_poll_control_send_set_short_poll_interval(
    uint8_t src_ep, s_zb_af_address_t *dst_addr,
    uint16_t new_short_poll_interval,
    uint8_t disable_default_rsp, uint8_t seq_num)
{
    uint8_t buf[2];
    buf[0] = LO_UINT16(new_short_poll_interval);
    buf[1] = HI_UINT16(new_short_poll_interval);
    return zb_zcl_send_cmd(
                src_ep, dst_addr, ZCL_CLUSTER_ID_GENERAL_POLL_CONTROL,
                COMMAND_POLL_CONTROL_SET_SHORT_POLL_INTERVAL, true, ZCL_FRAME_CLIENT_SERVER_DIR,
                disable_default_rsp, 0, seq_num, 2, buf);
}
