#include "device/generic/zb_device_relay.h"

#include "common/zb_common.h"
#include "af/zb_af.h"
#include "core/zb_core.h"
#include "device/zb_device_manager.h"
#include "zcl/zb_zcl.h"
#include "zcl/zb_zcl_general.h"

#define TAG "ZB_DEV"

static const s_zb_attr_read_t s_relay_read_attrs[] = {
    { ZCL_CLUSTER_ID_GENERAL_ON_OFF, 1, { ATTRID_ON_OFF_ON_OFF } },
};

static void
relay_init(s_zb_device_t *device, s_zb_function_t *func)
{
    ZB_LOGI(TAG, "0x%llx - %s()", device->ieee_addr, __func__);
}

static void
relay_destroy(s_zb_device_t *device, s_zb_function_t *func)
{
    ZB_LOGI(TAG, "0x%llx - %s()", device->ieee_addr, __func__);
}

static const s_zb_attr_read_t*
relay_get_read_attrs(s_zb_device_t *device, s_zb_function_t *func)
{
    (void)device;
    (void)func;
    return s_relay_read_attrs;
}

static void
relay_notify_state(s_zb_device_t *device, s_zb_function_t *func, s_zb_device_relay_ctx_t *ctx)
{
    ZB_LOGD(TAG, "0x%llx - %s()", device->ieee_addr, __func__);

    s_zb_event_t event = {0};
    event.type = ZB_EVENT_LIGHT_ONOFF_STATE;
    event.onoff_light.on = (ctx->on_off != 0);
    zb_device_manager_notify_event(device, func, &event);
}

static void
relay_on_event(s_zb_device_t *device, s_zb_function_t *func, s_zb_zcl_incoming_msg_t *msg)
{
    s_zb_device_relay_ctx_t *ctx = (s_zb_device_relay_ctx_t *)func->ctx;
    bool changed = false;

    if (msg->msg->cluster_id == ZCL_CLUSTER_ID_GENERAL_ON_OFF)
    {
        if (msg->hdr.command_id == ZCL_CMD_REPORT)
        {
            s_zb_zcl_report_attr_cmd_t *report_attr_cmd = (s_zb_zcl_report_attr_cmd_t *)msg->attr_cmd;
            for (uint8_t i = 0; i < report_attr_cmd->num_attr; i++)
            {
                s_zb_zcl_report_attr_info_t *report_attr_info = &report_attr_cmd->attr_list[i];
                if (report_attr_info->attr_id == ATTRID_ON_OFF_ON_OFF)
                {
                    if (ctx->on_off != report_attr_info->attr_data[0])
                    {
                        changed = true;
                    }
                    ctx->on_off = report_attr_info->attr_data[0];
                    break;
                }
            }
        }
        else if (msg->hdr.command_id == ZCL_CMD_READ_RSP)
        {
            s_zb_zcl_read_attr_rsp_cmd_t *read_attr_rsp_cmd = (s_zb_zcl_read_attr_rsp_cmd_t *)msg->attr_cmd;
            for (uint8_t i = 0; i < read_attr_rsp_cmd->num_attr; i++)
            {
                s_zb_zcl_read_attr_rsp_info_t *read_attr_rsp_info = &read_attr_rsp_cmd->attr_list[i];
                if (read_attr_rsp_info->attr_id == ATTRID_ON_OFF_ON_OFF && read_attr_rsp_info->status == ZCL_STATUS_SUCCESS)
                {
                    if (ctx->on_off != read_attr_rsp_info->data[0])
                    {
                        changed = true;
                    }
                    ctx->on_off = read_attr_rsp_info->data[0];
                    break;
                }
            }
        }
    }

    if (changed)
    {
        relay_notify_state(device, func, ctx);
    }
}

static void
relay_on_attr_report(s_zb_device_t *device, s_zb_function_t *func, s_zb_zcl_incoming_msg_t *msg)
{
    relay_on_event(device, func, msg);
}

static void
relay_on_read_rsp(s_zb_device_t *device, s_zb_function_t *func, s_zb_zcl_incoming_msg_t *msg)
{
    relay_on_event(device, func, msg);
}

static zb_status_t
relay_on_command(s_zb_device_t *device, s_zb_function_t *func, const s_zb_cmd_t *cmd)
{
    uint8_t dst_ep = ZB_SENSOR_EP(func->sensor_id);
    zb_status_t status = ZB_OK;

    switch (cmd->type)
    {
        case ZB_CMD_SET_ONOFF:
        {
            s_zb_af_address_t addr;
            addr.address_mode = AF_ADDRESS_16BIT;
            addr.endpoint = dst_ep;
            addr.short_addr = device->nwk_addr;
            uint8_t seq_num = zb_zcl_next_seq_num();

            if (cmd->onoff.on)
            {
                status = zb_zcl_general_send_on_off_cmd_on(ZB_HUB_ENDPOINT, &addr, false, seq_num);
            }
            else
            {
                status = zb_zcl_general_send_on_off_cmd_off(ZB_HUB_ENDPOINT, &addr, false, seq_num);
            }
            if (status == ZB_OK)
            {
                s_zb_device_relay_ctx_t *ctx = (s_zb_device_relay_ctx_t *)func->ctx;
                if (ctx->on_off != (uint8_t)cmd->onoff.on)
                {
                    ctx->on_off = (uint8_t)cmd->onoff.on;
                    relay_notify_state(device, func, ctx);
                }
            }
            break;
        }
        case ZB_CMD_READ_STATE:
        {
            s_zb_af_address_t addr;
            addr.address_mode = AF_ADDRESS_16BIT;
            addr.endpoint = dst_ep;
            addr.short_addr = device->nwk_addr;
            uint8_t seq_num = zb_zcl_next_seq_num();

            uint8_t buf[3] = {0};
            s_zb_zcl_read_attr_cmd_t *read_attr_cmd = (s_zb_zcl_read_attr_cmd_t *)&buf[0];
            read_attr_cmd->num_attr = 1;
            read_attr_cmd->attr_id[0] = ATTRID_ON_OFF_ON_OFF;
            status = zb_zcl_send_read(ZB_HUB_ENDPOINT, &addr, ZCL_CLUSTER_ID_GENERAL_ON_OFF, read_attr_cmd, ZCL_FRAME_CLIENT_SERVER_DIR, true, seq_num);
            break;
        }
        default:
            status = ZB_FAIL;
            break;
    }
    return status;
}

static void
relay_emit_state(s_zb_device_t *device, s_zb_function_t *func)
{
    relay_notify_state(device, func, (s_zb_device_relay_ctx_t *)func->ctx);
}

s_zb_function_ops_t zb_device_relay_ops = {
    .emit_state = relay_emit_state,
    .ctx_size = sizeof(s_zb_device_relay_ctx_t),
    .init = relay_init,
    .destroy = relay_destroy,
    .get_read_attrs = relay_get_read_attrs,
    .on_attr_report = relay_on_attr_report,
    .on_read_rsp = relay_on_read_rsp,
    .on_command = relay_on_command,
};
