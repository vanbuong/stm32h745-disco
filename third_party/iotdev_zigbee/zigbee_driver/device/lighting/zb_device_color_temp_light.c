#include "device/lighting/zb_device_color_temp_light.h"

#include "common/zb_common.h"
#include "af/zb_af.h"
#include "core/zb_core.h"
#include "device/zb_device_manager.h"
#include "zcl/zb_zcl.h"
#include "zcl/zb_zcl_lighting.h"

#define TAG "ZB_DEV"

static const s_zb_attr_read_t s_color_temp_light_read_attrs[] = {
    { ZCL_CLUSTER_ID_LIGHTING_COLOR_CONTROL, 1, { ATTRID_COLOR_CONTROL_COLOR_TEMPERATURE_MIREDS } },
};

static void
color_temp_light_init(s_zb_device_t *device, s_zb_function_t *func)
{
    ZB_LOGI(TAG, "0x%llx - %s()", device->ieee_addr, __func__);
}

static void
color_temp_light_destroy(s_zb_device_t *device, s_zb_function_t *func)
{
    ZB_LOGI(TAG, "0x%llx - %s()", device->ieee_addr, __func__);
}

static const s_zb_attr_read_t*
color_temp_light_get_read_attrs(s_zb_device_t *device, s_zb_function_t *func)
{
    return s_color_temp_light_read_attrs;
}

static void
color_temp_light_notify_state(s_zb_device_t *device, s_zb_function_t *func, s_zb_device_color_temp_light_ctx_t *ctx)
{
    ZB_LOGD(TAG, "0x%llx - %s()", device->ieee_addr, __func__);
    s_zb_event_t event;
    event.type = ZB_EVENT_LIGHT_COLOR_TEMP_STATE;
    event.color_temp_light.color_temp = ctx->color_temp;
    zb_device_manager_notify_event(device, func, &event);
}

static void
color_temp_light_on_event(s_zb_device_t *device, s_zb_function_t *func, s_zb_zcl_incoming_msg_t *msg)
{
    s_zb_device_color_temp_light_ctx_t *ctx = (s_zb_device_color_temp_light_ctx_t *)func->ctx;
    bool changed = false;

    if (msg->msg->cluster_id == ZCL_CLUSTER_ID_LIGHTING_COLOR_CONTROL)
    {
        if (msg->hdr.command_id == ZCL_CMD_REPORT)
        {
            s_zb_zcl_report_attr_cmd_t *report_attr_cmd = (s_zb_zcl_report_attr_cmd_t *)msg->attr_cmd;
            for (uint8_t i = 0; i < report_attr_cmd->num_attr; i++)
            {
                s_zb_zcl_report_attr_info_t *report_attr_info = &report_attr_cmd->attr_list[i];
                if (report_attr_info->attr_id == ATTRID_COLOR_CONTROL_COLOR_TEMPERATURE_MIREDS)
                {
                    uint16_t color_temp = BUILD_UINT16(report_attr_info->attr_data[0], report_attr_info->attr_data[1]);
                    if (ctx->color_temp != color_temp)
                    {
                        ZB_LOGI(TAG, "0x%llx - %s() - COLOR_TEMP: (%d) -> (%d)", device->ieee_addr, __func__, ctx->color_temp, color_temp);
                        changed = true;
                    }
                    ctx->color_temp = color_temp;
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
                if (read_attr_rsp_info->attr_id == ATTRID_COLOR_CONTROL_COLOR_TEMPERATURE_MIREDS && read_attr_rsp_info->status == ZCL_STATUS_SUCCESS)
                {
                    uint16_t color_temp = BUILD_UINT16(read_attr_rsp_info->data[0], read_attr_rsp_info->data[1]);
                    if (ctx->color_temp != color_temp)
                    {
                        ZB_LOGI(TAG, "0x%llx - %s() - COLOR_TEMP: (%d) -> (%d)", device->ieee_addr, __func__, ctx->color_temp, color_temp);
                        changed = true;
                    }
                    ctx->color_temp = color_temp;
                    break;
                }
            }
        }
    }
    else
    {
        ZB_LOGW(TAG, "0x%llx - %s() - Unsupported cluster ID: 0x%04x", device->ieee_addr, __func__, msg->msg->cluster_id);
    }
    if (changed)
    {
        color_temp_light_notify_state(device, func, ctx);
    }
}

static void
color_temp_light_on_attr_report(s_zb_device_t *device, s_zb_function_t *func, s_zb_zcl_incoming_msg_t *msg)
{
    ZB_LOGI(TAG, "0x%llx - %s()", device->ieee_addr, __func__);
    color_temp_light_on_event(device, func, msg);
}

static void
color_temp_light_on_read_rsp(s_zb_device_t *device, s_zb_function_t *func, s_zb_zcl_incoming_msg_t *msg)
{
    ZB_LOGI(TAG, "0x%llx - %s()", device->ieee_addr, __func__);
    color_temp_light_on_event(device, func, msg);
}

static zb_status_t
color_temp_light_on_command(s_zb_device_t *device, s_zb_function_t *func, const s_zb_cmd_t *cmd)
{
    ZB_LOGI(TAG, "0x%llx - %s()", device->ieee_addr, __func__);

    uint8_t dst_ep = ZB_SENSOR_EP(func->sensor_id);
    zb_status_t status = ZB_OK;

    switch (cmd->type)
    {
        case ZB_CMD_SET_COLOR_TEMP:
        {
            s_zb_af_address_t addr;
            addr.address_mode = AF_ADDRESS_16BIT;
            addr.endpoint = dst_ep;
            addr.short_addr = device->nwk_addr;
            uint8_t seq_num = zb_zcl_next_seq_num();
            uint16_t trans = cmd->color_temp.trans_ms / 100;
            status = zb_zcl_lighting_color_control_send_move_to_color_temperature_cmd(ZB_HUB_ENDPOINT, &addr, cmd->color_temp.color_temp, trans, false, seq_num);
            if (status == ZB_OK)
            {
                s_zb_device_color_temp_light_ctx_t *ctx = (s_zb_device_color_temp_light_ctx_t *)func->ctx;
                if (ctx->color_temp != cmd->color_temp.color_temp)
                {
                    ctx->color_temp = cmd->color_temp.color_temp;
                    color_temp_light_notify_state(device, func, ctx);
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
            read_attr_cmd->attr_id[0] = ATTRID_COLOR_CONTROL_COLOR_TEMPERATURE_MIREDS;
            status = zb_zcl_send_read(ZB_HUB_ENDPOINT, &addr, ZCL_CLUSTER_ID_LIGHTING_COLOR_CONTROL, read_attr_cmd, ZCL_FRAME_CLIENT_SERVER_DIR, true, seq_num);
            break;
        }
        default:
        {
            ZB_LOGW(TAG, "0x%llx - %s() - Unsupported command type: %d", device->ieee_addr, __func__, cmd->type);
            status = ZB_FAIL;
        }
    }
    return status;
}

static void
color_temp_light_emit_state(s_zb_device_t *device, s_zb_function_t *func)
{
    color_temp_light_notify_state(device, func, (s_zb_device_color_temp_light_ctx_t *)func->ctx);
}

s_zb_function_ops_t zb_device_color_temp_light_ops = {
    .emit_state = color_temp_light_emit_state,
    .ctx_size = sizeof(s_zb_device_color_temp_light_ctx_t),
    .init = color_temp_light_init,
    .destroy = color_temp_light_destroy,
    .on_attr_report = color_temp_light_on_attr_report,
    .on_read_rsp = color_temp_light_on_read_rsp,
    .get_read_attrs = color_temp_light_get_read_attrs,
    .on_command = color_temp_light_on_command,
};