#include "device/generic/zb_device_basic_info.h"

#include "common/zb_common.h"
#include "af/zb_af.h"
#include "core/zb_core.h"
#include "device/zb_device_manager.h"
#include "zcl/zb_zcl.h"
#include "zcl/zb_zcl_general.h"

#define TAG "ZB_DEV"

static const s_zb_attr_read_t s_basic_info_read_attrs[] = {
    { ZCL_CLUSTER_ID_GENERAL_BASIC, 7,
        { ATTRID_BASIC_ZCL_VERSION,
          ATTRID_BASIC_APPLICATION_VERSION,
          ATTRID_BASIC_HW_VERSION,
          ATTRID_BASIC_MANUFACTURER_NAME,
          ATTRID_BASIC_MODEL_IDENTIFIER,
          ATTRID_BASIC_POWER_SOURCE,
          ATTRID_BASIC_PHYSICAL_ENVIRONMENT } },
};

static void
basic_info_init(s_zb_device_t *device, s_zb_function_t *func)
{
    ZB_LOGI(TAG, "0x%llx - %s()", device->ieee_addr, __func__);
    s_zb_device_basic_info_ctx_t *ctx = (s_zb_device_basic_info_ctx_t *)func->ctx;
    memset(ctx, 0, sizeof(s_zb_device_basic_info_ctx_t));
}

static void
basic_info_destroy(s_zb_device_t *device, s_zb_function_t *func)
{
    ZB_LOGI(TAG, "0x%llx - %s()", device->ieee_addr, __func__);
}

static const s_zb_attr_read_t*
basic_info_get_read_attrs(s_zb_device_t *device, s_zb_function_t *func)
{
    return s_basic_info_read_attrs;
}

static void
basic_info_notify_state(s_zb_device_t *device, s_zb_function_t *func, s_zb_device_basic_info_ctx_t *ctx)
{
    ZB_LOGD(TAG, "0x%llx - %s()", device->ieee_addr, __func__);

    /* Mirror the (static) Basic metadata onto the device record so JOINED /
     * DEVICE_LIST — which read from the device struct — carry it too, including
     * after a reboot where these non-persisted fields start empty. */
    device->app_version = ctx->app_version;
    device->hw_version = ctx->hw_version;
    device->physical_environment = ctx->physical_environment;
    strncpy(device->date_code, ctx->date_code, sizeof(device->date_code));

    s_zb_event_t event = {0};
    event.type = ZB_EVENT_DEVICE_UPDATED;
    strncpy(event.device_info.manufacturer, ctx->manufacturer, sizeof(event.device_info.manufacturer));
    strncpy(event.device_info.model, ctx->model, sizeof(event.device_info.model));
    strncpy(event.device_info.date_code, ctx->date_code, sizeof(event.device_info.date_code));
    event.device_info.parent_ieee = device->parent_ieee;
    event.device_info.parent_nwk_addr = device->parent_nwk_addr;
    event.device_info.battery_powered = device->battery_powered;
    event.device_info.sleepy_enabled = device->sleepy_enabled;
    event.device_info.ota_supported = device->ota_supported;
    event.device_info.device_type = device->device_type;
    event.device_info.app_version = ctx->app_version;
    event.device_info.hw_version = ctx->hw_version;
    event.device_info.physical_environment = ctx->physical_environment;
    /* Omit ZB_FUNC_BASIC_INFO from the published function list — it is device
     * metadata, already carried at the top level of device_info, not a real
     * sensor/actuator. Keep JOINED/LIST/UPDATED consistent. */
    uint8_t out = 0;
    for (int i = 0; i < device->function_count && out < ZB_MAX_FUNCTIONS; i++) {
        if (device->functions[i].type == ZB_FUNC_BASIC_INFO) {
            continue;
        }
        event.device_info.functions[out].sensor_id = device->functions[i].sensor_id;
        event.device_info.functions[out].type = device->functions[i].type;
        strncpy(event.device_info.functions[out].name, device->functions[i].name, sizeof(event.device_info.functions[out].name));
        out++;
    }
    event.device_info.function_count = out;
    zb_device_manager_notify_event(device, func, &event);
}

/* Copy a Zigbee octet string (data[0]=len, data[1..]=chars) into a fixed
 * char buffer only if it differs; returns true when it changed. */
static bool
basic_info_set_string(char *dst, size_t dst_size, const uint8_t *data)
{
    char tmp[32];
    if (dst_size > sizeof(tmp))
    {
        dst_size = sizeof(tmp);
    }
    memset(tmp, 0, dst_size);
    uint8_t data_len = data[0];
    if (data_len >= dst_size)
    {
        data_len = (uint8_t)(dst_size - 1);
    }
    memcpy(tmp, data + 1, data_len);
    if (memcmp(dst, tmp, dst_size) == 0)
    {
        return false;
    }
    memcpy(dst, tmp, dst_size);
    return true;
}

/* Fold one attribute into the context. Returns true only if it changed the
 * stored value, so the caller can suppress no-op DEVICE_UPDATED events. */
static bool
basic_info_update_ctx(s_zb_device_t *device, s_zb_device_basic_info_ctx_t *ctx, uint16_t attr_id, uint8_t *data)
{
    bool changed = false;
    switch (attr_id)
    {
        case ATTRID_BASIC_ZCL_VERSION:
            changed = (ctx->zcl_version != data[0]);
            ctx->zcl_version = data[0];
            break;
        case ATTRID_BASIC_APPLICATION_VERSION:
            changed = (ctx->app_version != data[0]);
            ctx->app_version = data[0];
            break;
        case ATTRID_BASIC_HW_VERSION:
            changed = (ctx->hw_version != data[0]);
            ctx->hw_version = data[0];
            break;
        case ATTRID_BASIC_MANUFACTURER_NAME:
            changed = basic_info_set_string(ctx->manufacturer, sizeof(ctx->manufacturer), data);
            break;
        case ATTRID_BASIC_MODEL_IDENTIFIER:
            changed = basic_info_set_string(ctx->model, sizeof(ctx->model), data);
            break;
        case ATTRID_BASIC_DATE_CODE:
            changed = basic_info_set_string(ctx->date_code, sizeof(ctx->date_code), data);
            break;
        case ATTRID_BASIC_POWER_SOURCE:
            changed = (ctx->power_source != data[0]);
            ctx->power_source = data[0];
            break;
        case ATTRID_BASIC_PHYSICAL_ENVIRONMENT:
            changed = (ctx->physical_environment != data[0]);
            ctx->physical_environment = data[0];
            break;
        default:
            ZB_LOGW(TAG, "0x%llx - %s() - Unsupported attribute ID: 0x%04x", device->ieee_addr, __func__, attr_id);
            break;
    }
    return changed;
}

static void
basic_info_on_event(s_zb_device_t *device, s_zb_function_t *func, s_zb_zcl_incoming_msg_t *msg)
{
    ZB_LOGI(TAG, "0x%llx - %s()", device->ieee_addr, __func__);
    s_zb_device_basic_info_ctx_t *ctx = (s_zb_device_basic_info_ctx_t *)func->ctx;
    bool changed = false;

    if (msg->msg->cluster_id == ZCL_CLUSTER_ID_GENERAL_BASIC)
    {
        if (msg->hdr.command_id == ZCL_CMD_REPORT)
        {
            s_zb_zcl_report_attr_cmd_t *report_attr_cmd = (s_zb_zcl_report_attr_cmd_t *)msg->attr_cmd;
            for (uint8_t i = 0; i < report_attr_cmd->num_attr; i++)
            {
                s_zb_zcl_report_attr_info_t *report_attr_info = &report_attr_cmd->attr_list[i];
                changed |= basic_info_update_ctx(device, ctx, report_attr_info->attr_id, report_attr_info->attr_data);
            }
        }
        else if (msg->hdr.command_id == ZCL_CMD_READ_RSP)
        {
            s_zb_zcl_read_attr_rsp_cmd_t *attr_cmd = (s_zb_zcl_read_attr_rsp_cmd_t *)msg->attr_cmd;
            for (uint8_t i = 0; i < attr_cmd->num_attr; i++)
            {
                s_zb_zcl_read_attr_rsp_info_t *attr_info = &attr_cmd->attr_list[i];
                if (attr_info->status != ZCL_STATUS_SUCCESS)
                {
                    ZB_LOGW(TAG, "0x%llx - %s() - Attribute ID: 0x%04x - Status: 0x%02x", device->ieee_addr, __func__, attr_info->attr_id, attr_info->status);
                    continue;
                }
                changed |= basic_info_update_ctx(device, ctx, attr_info->attr_id, attr_info->data);
            }
        }
        else
        {
            ZB_LOGW(TAG, "0x%llx - %s() - Unsupported command ID: 0x%04x", device->ieee_addr, __func__, msg->hdr.command_id);
            return;
        }
    }
    else
    {
        ZB_LOGW(TAG, "0x%llx - %s() - Unsupported cluster ID: 0x%04x", device->ieee_addr, __func__, msg->msg->cluster_id);
        return;
    }

    if (changed)
    {
        basic_info_notify_state(device, func, ctx);
    }
}

static void
basic_info_on_attr_report(s_zb_device_t *device, s_zb_function_t *func, s_zb_zcl_incoming_msg_t *msg)
{
    ZB_LOGI(TAG, "0x%llx - %s()", device->ieee_addr, __func__);
    basic_info_on_event(device, func, msg);
}

static void
basic_info_on_read_rsp(s_zb_device_t *device, s_zb_function_t *func, s_zb_zcl_incoming_msg_t *msg)
{
    ZB_LOGI(TAG, "0x%llx - %s()", device->ieee_addr, __func__);
    basic_info_on_event(device, func, msg);
}

static zb_status_t
basic_info_on_command(s_zb_device_t *device, s_zb_function_t *func, const s_zb_cmd_t *cmd)
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

            uint8_t buf[11] = {0};
            s_zb_zcl_read_attr_cmd_t *read_attr_cmd = (s_zb_zcl_read_attr_cmd_t *)&buf[0];
            read_attr_cmd->num_attr = 5;
            read_attr_cmd->attr_id[0] = ATTRID_BASIC_ZCL_VERSION;
            read_attr_cmd->attr_id[1] = ATTRID_BASIC_APPLICATION_VERSION;
            read_attr_cmd->attr_id[2] = ATTRID_BASIC_MANUFACTURER_NAME;
            read_attr_cmd->attr_id[3] = ATTRID_BASIC_MODEL_IDENTIFIER;
            read_attr_cmd->attr_id[4] = ATTRID_BASIC_POWER_SOURCE;
            
            status = zb_zcl_send_read(ZB_HUB_ENDPOINT, &addr, ZCL_CLUSTER_ID_GENERAL_BASIC, read_attr_cmd, ZCL_FRAME_CLIENT_SERVER_DIR, true, seq_num);
            ZB_LOGI(TAG, "0x%llx - %s() - Sent read (%d)", device->ieee_addr, __func__, status);
            break;
        }
        default:
            ZB_LOGW(TAG, "0x%llx - %s() - Unsupported command type: 0x%02x", device->ieee_addr, __func__, cmd->type);
            status = ZB_FAIL;
    }
    return status;
}

s_zb_function_ops_t zb_device_basic_info_ops = {
    .ctx_size = sizeof(s_zb_device_basic_info_ctx_t),
    .init = basic_info_init,
    .destroy = basic_info_destroy,
    .get_read_attrs = basic_info_get_read_attrs,
    .on_attr_report = basic_info_on_attr_report,
    .on_read_rsp = basic_info_on_read_rsp,
    .on_command = basic_info_on_command,
};
