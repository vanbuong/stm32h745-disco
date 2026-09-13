#include "device/generic/zb_device_dimmer_switch.h"

#include "common/zb_common.h"
#include "af/zb_af.h"
#include "core/zb_core.h"
#include "device/zb_device_manager.h"
#include "zdo/zb_zdo.h"
#include "zcl/zb_zcl.h"
#include "zcl/zb_zcl_general.h"

#define TAG "ZB_DEV"

static void
dimmer_switch_init(s_zb_device_t *device, s_zb_function_t *func)
{
    ZB_LOGI(TAG, "0x%llx - %s()", device->ieee_addr, __func__);
    s_zb_device_dimmer_switch_ctx_t *ctx = (s_zb_device_dimmer_switch_ctx_t *)func->ctx;
    ctx->level = 0;
}

static void
dimmer_switch_destroy(s_zb_device_t *device, s_zb_function_t *func)
{
    ZB_LOGI(TAG, "0x%llx - %s()", device->ieee_addr, __func__);
}

static const s_zb_attr_read_t*
dimmer_switch_get_read_attrs(s_zb_device_t *device, s_zb_function_t *func)
{
    return NULL;
}

static void
dimmer_switch_notify_state(s_zb_device_t *device, s_zb_function_t *func, s_zb_device_dimmer_switch_ctx_t *ctx)
{
    ZB_LOGD(TAG, "0x%llx - %s()", device->ieee_addr, __func__);
    s_zb_event_t event = {0};
    event.type = ZB_EVENT_SWITCH_LEVEL_STATE;
    event.switch_level.level = ctx->level;
    zb_device_manager_notify_event(device, func, &event);
}

static void
dimmer_switch_on_read_rsp(s_zb_device_t *device, s_zb_function_t *func, s_zb_zcl_incoming_msg_t *msg)
{
    ZB_LOGI(TAG, "0x%llx - %s()", device->ieee_addr, __func__);
}

static void
dimmer_switch_on_attr_report(s_zb_device_t *device, s_zb_function_t *func, s_zb_zcl_incoming_msg_t *msg)
{
    ZB_LOGI(TAG, "0x%llx - %s()", device->ieee_addr, __func__);
    s_zb_device_dimmer_switch_ctx_t *ctx = (s_zb_device_dimmer_switch_ctx_t *)func->ctx;
    bool changed = false;

    if (msg->msg->cluster_id == ZCL_CLUSTER_ID_GENERAL_LEVEL_CONTROL)
    {
        s_zb_zcl_report_attr_cmd_t *report_attr_cmd = (s_zb_zcl_report_attr_cmd_t *)msg->attr_cmd;
        for (uint8_t i = 0; i < report_attr_cmd->num_attr; i++)
        {
            s_zb_zcl_report_attr_info_t *report_attr_info = &report_attr_cmd->attr_list[i];
            if (report_attr_info->attr_id == ATTRID_LEVEL_CURRENT_LEVEL)
            {
                if (ctx->level != report_attr_info->attr_data[0])
                {
                    changed = true;
                }
                ctx->level = report_attr_info->attr_data[0];
                break;
            }
        }
    }
    else
    {
        ZB_LOGW(TAG, "0x%llx - %s() - Unsupported cluster ID: 0x%04x", device->ieee_addr, __func__, msg->msg->cluster_id);
    }
    if (changed)
    {
        dimmer_switch_notify_state(device, func, ctx);
    }
}

static zb_status_t
dimmer_switch_on_command(s_zb_device_t *device, s_zb_function_t *func, const s_zb_cmd_t *cmd)
{
    ZB_LOGI(TAG, "0x%llx - %s()", device->ieee_addr, __func__);

    zb_status_t status = ZB_OK;

    switch (cmd->type)
    {
        case ZB_CMD_DEVICE_BIND:
        {
            s_zb_device_t *bind_device = zb_device_manager_find_by_ieee(cmd->device_bind.ieee_addr);
            if (bind_device == NULL)
            {
                ZB_LOGW(TAG, "0x%llx - %s() - Bind device not found", device->ieee_addr, __func__);
                status = ZB_FAIL;
                break;
            }
            uint16_t cluster_id = ZB_SENSOR_CLUSTER(cmd->device_bind.sensor_id);
            status = zb_core_bind_enqueue(device->nwk_addr, device->ieee_addr,
                    ZB_SENSOR_EP(func->sensor_id), cmd->device_bind.ieee_addr,
                    ZB_SENSOR_EP(cmd->device_bind.sensor_id), cluster_id, false);
            break;
        }
        case ZB_CMD_DEVICE_UNBIND:
        {
            s_zb_device_t *unbind_device = zb_device_manager_find_by_ieee(cmd->device_bind.ieee_addr);
            if (unbind_device == NULL)
            {
                ZB_LOGW(TAG, "0x%llx - %s() - Unbind device not found", device->ieee_addr, __func__);
                status = ZB_FAIL;
                break;
            }
            uint16_t cluster_id = ZB_SENSOR_CLUSTER(cmd->device_bind.sensor_id);
            status = zb_core_unbind_enqueue(device->nwk_addr, device->ieee_addr,
                    ZB_SENSOR_EP(func->sensor_id), cmd->device_bind.ieee_addr,
                    ZB_SENSOR_EP(cmd->device_bind.sensor_id), cluster_id);
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
dimmer_switch_emit_state(s_zb_device_t *device, s_zb_function_t *func)
{
    dimmer_switch_notify_state(device, func, (s_zb_device_dimmer_switch_ctx_t *)func->ctx);
}

s_zb_function_ops_t zb_device_dimmer_switch_ops = {
    .emit_state = dimmer_switch_emit_state,
    .ctx_size = sizeof(s_zb_device_dimmer_switch_ctx_t),
    .init = dimmer_switch_init,
    .destroy = dimmer_switch_destroy,
    .get_read_attrs = dimmer_switch_get_read_attrs,
    .on_attr_report = dimmer_switch_on_attr_report,
    .on_read_rsp = dimmer_switch_on_read_rsp,
    .on_command = dimmer_switch_on_command,
};