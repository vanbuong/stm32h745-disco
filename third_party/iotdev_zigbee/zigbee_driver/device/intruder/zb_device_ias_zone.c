#include "device/intruder/zb_device_ias_zone.h"

#include "common/zb_common.h"
#include "af/zb_af.h"
#include "core/zb_core.h"
#include "device/zb_device_manager.h"
#include "zcl/zb_zcl.h"
#include "zcl/zb_zcl_general.h"
#include "zcl/zb_zcl_ss.h"

#define TAG "ZB_DEV"

static const s_zb_attr_read_t s_ias_zone_read_attrs[] = {
    { ZCL_CLUSTER_ID_SS_IAS_ZONE, 2, { ATTRID_IAS_ZONE_ZONE_TYPE, ATTRID_IAS_ZONE_ZONE_STATUS } },
};

static void
ias_zone_init(s_zb_device_t *device, s_zb_function_t *func)
{
    ZB_LOGI(TAG, "0x%llx - %s()", device->ieee_addr, __func__);
    s_zb_device_ias_zone_ctx_t *ctx = (s_zb_device_ias_zone_ctx_t *)func->ctx;
    ctx->zone_type = 0;
    ctx->zone_status = 0;
}

static void
ias_zone_destroy(s_zb_device_t *device, s_zb_function_t *func)
{
    ZB_LOGI(TAG, "0x%llx - %s()", device->ieee_addr, __func__);
}

static const s_zb_attr_read_t*
ias_zone_get_read_attrs(s_zb_device_t *device, s_zb_function_t *func)
{
    return s_ias_zone_read_attrs;
}

static void
ias_zone_notify_state(s_zb_device_t *device, s_zb_function_t *func, s_zb_device_ias_zone_ctx_t *ctx)
{
    ZB_LOGD(TAG, "0x%llx - %s()", device->ieee_addr, __func__);
    s_zb_event_t event;
    event.type = ZB_EVENT_SENSOR_IAS_ZONE;
    event.ias.zone_type = ctx->zone_type;
    event.ias.zone_status = ctx->zone_status;
    zb_device_manager_notify_event(device, func, &event);
}

static bool
ias_zone_on_ctx_update(s_zb_device_t *device, s_zb_function_t *func, uint16_t attr_id, uint8_t *data)
{
    bool changed = false;
    s_zb_device_ias_zone_ctx_t *ctx = (s_zb_device_ias_zone_ctx_t *)func->ctx;
    switch (attr_id)
    {
        case ATTRID_IAS_ZONE_ZONE_TYPE:
        {
            uint16_t zone_type = BUILD_UINT16(data[0], data[1]);
            ZB_LOGI(TAG, "0x%llx - %s() - ZONE_TYPE: (%d) -> (%d)", device->ieee_addr, __func__, ctx->zone_type, zone_type);
            ctx->zone_type = zone_type;
            break;
        }
        case ATTRID_IAS_ZONE_ZONE_STATUS:
        {
            uint16_t zone_status = BUILD_UINT16(data[0], data[1]);
            ZB_LOGI(TAG, "0x%llx - %s() - ZONE_STATUS: (%d) -> (%d)", device->ieee_addr, __func__, ctx->zone_status, zone_status);
            if (ctx->zone_status != zone_status)
            {
                changed = true;
            }
            ctx->zone_status = zone_status;
            break;
        }
        default:
            ZB_LOGW(TAG, "0x%llx - %s() - Unsupported attribute ID: 0x%04x", device->ieee_addr, __func__, attr_id);
            break;
    }
    return changed;
}

static void
ias_zone_on_attr_report(s_zb_device_t *device, s_zb_function_t *func, s_zb_zcl_incoming_msg_t *msg)
{
    ZB_LOGI(TAG, "0x%llx - %s()", device->ieee_addr, __func__);
    bool changed = false;
    if (msg->msg->cluster_id == ZCL_CLUSTER_ID_SS_IAS_ZONE)
    {
        s_zb_zcl_report_attr_cmd_t *report_attr_cmd = (s_zb_zcl_report_attr_cmd_t *)msg->attr_cmd;
        for (uint8_t i = 0; i < report_attr_cmd->num_attr; i++)
        {
            s_zb_zcl_report_attr_info_t *report_attr_info = &report_attr_cmd->attr_list[i];
            changed |= ias_zone_on_ctx_update(device, func, report_attr_info->attr_id, report_attr_info->attr_data);
        }
    }
    else
    {
        ZB_LOGW(TAG, "0x%llx - %s() - Unsupported cluster ID: 0x%04x", device->ieee_addr, __func__, msg->msg->cluster_id);
    }
    if (changed)
    {
        ias_zone_notify_state(device, func, (s_zb_device_ias_zone_ctx_t *)func->ctx);
    }
}

static void
ias_zone_on_read_rsp(s_zb_device_t *device, s_zb_function_t *func, s_zb_zcl_incoming_msg_t *msg)
{
    ZB_LOGI(TAG, "0x%llx - %s()", device->ieee_addr, __func__);
    bool changed = false;
    if (msg->msg->cluster_id == ZCL_CLUSTER_ID_SS_IAS_ZONE)
    {
        s_zb_zcl_read_attr_rsp_cmd_t *read_attr_rsp_cmd = (s_zb_zcl_read_attr_rsp_cmd_t *)msg->attr_cmd;
        for (uint8_t i = 0; i < read_attr_rsp_cmd->num_attr; i++)
        {
            s_zb_zcl_read_attr_rsp_info_t *read_attr_rsp_info = &read_attr_rsp_cmd->attr_list[i];
            if (read_attr_rsp_info->status == ZCL_STATUS_SUCCESS)
            {
                changed |= ias_zone_on_ctx_update(device, func, read_attr_rsp_info->attr_id, read_attr_rsp_info->data);
            }
            else
            {
                ZB_LOGW(TAG, "0x%llx - %s() - Failed to read attribute ID: 0x%04x, status: 0x%02x",
                    __func__, device->ieee_addr, read_attr_rsp_info->attr_id, read_attr_rsp_info->status);
            }
        }
    }
    else
    {
        ZB_LOGW(TAG, "0x%llx - %s() - Unsupported cluster ID: 0x%04x", device->ieee_addr, __func__, msg->msg->cluster_id);
    }
    if (changed)
    {
        ias_zone_notify_state(device, func, (s_zb_device_ias_zone_ctx_t *)func->ctx);
    }
}

static zb_status_t
ias_zone_on_command(s_zb_device_t *device, s_zb_function_t *func, const s_zb_cmd_t *cmd)
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
            read_attr_cmd->num_attr = 2;
            read_attr_cmd->attr_id[0] = ATTRID_IAS_ZONE_ZONE_TYPE;
            read_attr_cmd->attr_id[1] = ATTRID_IAS_ZONE_ZONE_STATUS;
            status = zb_zcl_send_read(ZB_HUB_ENDPOINT, &addr, ZCL_CLUSTER_ID_SS_IAS_ZONE, read_attr_cmd, ZCL_FRAME_CLIENT_SERVER_DIR, true, seq_num);
            ZB_LOGI(TAG, "0x%llx - %s() - Sent read (%d)", device->ieee_addr, __func__, status);
            break;
        }
        default:
            ZB_LOGW(TAG, "0x%llx - %s() - Unsupported command type: %d", device->ieee_addr, __func__, cmd->type);
            status = ZB_FAILURE;
            break;
    }
    return status;
}

static void
ias_zone_emit_state(s_zb_device_t *device, s_zb_function_t *func)
{
    ias_zone_notify_state(device, func, (s_zb_device_ias_zone_ctx_t *)func->ctx);
}

s_zb_function_ops_t zb_device_ias_zone_ops = {
    .emit_state = ias_zone_emit_state,
    .ctx_size = sizeof(s_zb_device_ias_zone_ctx_t),
    .init = ias_zone_init,
    .destroy = ias_zone_destroy,
    .get_read_attrs = ias_zone_get_read_attrs,
    .on_attr_report = ias_zone_on_attr_report,
    .on_read_rsp = ias_zone_on_read_rsp,
    .on_command = ias_zone_on_command,
};