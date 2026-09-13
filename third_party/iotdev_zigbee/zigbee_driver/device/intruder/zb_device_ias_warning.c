#include "device/intruder/zb_device_ias_warning.h"

#include "common/zb_common.h"
#include "af/zb_af.h"
#include "core/zb_core.h"
#include "device/zb_device_manager.h"
#include "zcl/zb_zcl.h"
#include "zcl/zb_zcl_general.h"
#include "zcl/zb_zcl_ss.h"

#define TAG "ZB_DEV"

static const s_zb_attr_read_t s_ias_warning_read_attrs[] = {
    { ZCL_CLUSTER_ID_SS_IAS_WD, 1, { ATTRID_IAS_WD_MAX_DURATION } },
};

static void
ias_warning_init(s_zb_device_t *device, s_zb_function_t *func)
{
    ZB_LOGI(TAG, "0x%llx - %s()", device->ieee_addr, __func__);
    s_zb_device_ias_warning_ctx_t *ctx = (s_zb_device_ias_warning_ctx_t *)func->ctx;
    ctx->max_duration = 0;
}

static void
ias_warning_destroy(s_zb_device_t *device, s_zb_function_t *func)
{
    ZB_LOGI(TAG, "0x%llx - %s()", device->ieee_addr, __func__);
}

static const s_zb_attr_read_t*
ias_warning_get_read_attrs(s_zb_device_t *device, s_zb_function_t *func)
{
    return s_ias_warning_read_attrs;
}

static void
ias_warning_notify_state(s_zb_device_t *device, s_zb_function_t *func, s_zb_device_ias_warning_ctx_t *ctx)
{
    ZB_LOGD(TAG, "0x%llx - %s()", device->ieee_addr, __func__);

    s_zb_event_t event;
    event.type = ZB_EVENT_DEVICE_IAS_WARNING;
    event.ias_warning.max_duration = ((s_zb_device_ias_warning_ctx_t *)func->ctx)->max_duration;
    zb_device_manager_notify_event(device, func, &event);
}

static void
ias_warning_on_read_rsp(s_zb_device_t *device, s_zb_function_t *func, s_zb_zcl_incoming_msg_t *msg)
{
    ZB_LOGI(TAG, "0x%llx - %s()", device->ieee_addr, __func__);
    bool changed = false;
    if (msg->msg->cluster_id == ZCL_CLUSTER_ID_SS_IAS_WD)
    {
        s_zb_zcl_read_attr_rsp_cmd_t *read_attr_rsp_cmd = (s_zb_zcl_read_attr_rsp_cmd_t *)msg->attr_cmd;
        for (uint8_t i = 0; i < read_attr_rsp_cmd->num_attr; i++)
        {
            s_zb_zcl_read_attr_rsp_info_t *read_attr_rsp_info = &read_attr_rsp_cmd->attr_list[i];
            if (read_attr_rsp_info->status == ZCL_STATUS_SUCCESS && read_attr_rsp_info->attr_id == ATTRID_IAS_WD_MAX_DURATION)
            {
                uint16_t max_duration = BUILD_UINT16(read_attr_rsp_info->data[0], read_attr_rsp_info->data[1]);
                changed = max_duration != ((s_zb_device_ias_warning_ctx_t *)func->ctx)->max_duration;
                ((s_zb_device_ias_warning_ctx_t *)func->ctx)->max_duration = max_duration;
            }
            else
            {
                ZB_LOGW(TAG, "0x%llx - %s() - Failed to read attribute ID: 0x%04x, status: 0x%02x",
                    device->ieee_addr, __func__, read_attr_rsp_info->attr_id, read_attr_rsp_info->status);
            }
        }
    }
    else
    {
        ZB_LOGW(TAG, "0x%llx - %s() - Unsupported cluster ID: 0x%04x", device->ieee_addr, __func__, msg->msg->cluster_id);
    }
    if (changed)
    {
        ias_warning_notify_state(device, func, (s_zb_device_ias_warning_ctx_t *)func->ctx);
    }
}

static void
ias_warning_on_attr_report(s_zb_device_t *device, s_zb_function_t *func, s_zb_zcl_incoming_msg_t *msg)
{
    ZB_LOGI(TAG, "0x%llx - %s()", device->ieee_addr, __func__);
}

static zb_status_t
ias_warning_on_command(s_zb_device_t *device, s_zb_function_t *func, const s_zb_cmd_t *cmd)
{
    ZB_LOGI(TAG, "0x%llx - %s()", device->ieee_addr, __func__);
    zb_status_t status = ZB_OK;
    switch (cmd->type)
    {
        case ZB_CMD_READ_STATE:
        {
            s_zb_af_address_t addr;
            addr.address_mode = AF_ADDRESS_16BIT;
            addr.endpoint = ZB_SENSOR_EP(func->sensor_id);
            addr.short_addr = device->nwk_addr;
            uint8_t seq_num = zb_zcl_next_seq_num();
            uint8_t buf[5] = {0};
            s_zb_zcl_read_attr_cmd_t *read_attr_cmd = (s_zb_zcl_read_attr_cmd_t *)&buf[0];
            read_attr_cmd->num_attr = 1;
            read_attr_cmd->attr_id[0] = ATTRID_IAS_WD_MAX_DURATION;
            status = zb_zcl_send_read(ZB_HUB_ENDPOINT, &addr, ZCL_CLUSTER_ID_SS_IAS_WD, read_attr_cmd, ZCL_FRAME_CLIENT_SERVER_DIR, false, seq_num);
            break;
        }
        case ZB_CMD_IAS_WARNING:
        {
            s_zb_af_address_t addr;
            addr.address_mode = AF_ADDRESS_16BIT;
            addr.endpoint = ZB_SENSOR_EP(func->sensor_id);
            addr.short_addr = device->nwk_addr;
            uint8_t seq_num = zb_zcl_next_seq_num();

            s_zb_zcl_wd_start_warning_t warning = {0};
            warning.warning_message.warning_bits.warn_mode        = cmd->ias_warning.mode;
            warning.warning_message.warning_bits.warn_strobe      = cmd->ias_warning.strobe;
            warning.warning_message.warning_bits.warn_siren_level = cmd->ias_warning.siren_level;
            warning.warning_duration  = cmd->ias_warning.duration;
            warning.strobe_duty_cycle = cmd->ias_warning.strobe_duty_cycle;
            warning.strobe_level      = cmd->ias_warning.strobe_level;

            status = zb_zcl_ss_send_ias_wd_start_warning_cmd(ZB_HUB_ENDPOINT, &addr, &warning, false, seq_num);
            break;
        }
        default:
            ZB_LOGW(TAG, "0x%llx - %s() - Unsupported command type: %d", device->ieee_addr, __func__, cmd->type);
            status = ZB_FAIL;
            break;
    }
    return status;
}

static void
ias_warning_emit_state(s_zb_device_t *device, s_zb_function_t *func)
{
    ias_warning_notify_state(device, func, (s_zb_device_ias_warning_ctx_t *)func->ctx);
}

s_zb_function_ops_t zb_device_ias_warning_ops = {
    .emit_state = ias_warning_emit_state,
    .ctx_size = sizeof(s_zb_device_ias_warning_ctx_t),
    .init = ias_warning_init,
    .destroy = ias_warning_destroy,
    .get_read_attrs = ias_warning_get_read_attrs,
    .on_read_rsp = ias_warning_on_read_rsp,
    .on_attr_report = ias_warning_on_attr_report,
    .on_command = ias_warning_on_command,
};