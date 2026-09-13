#include "device/measurement/zb_device_pm10_sensor.h"

#include "common/zb_common.h"
#include "common/zb_sensor_units.h"
#include "af/zb_af.h"
#include "core/zb_core.h"
#include "device/zb_device_manager.h"
#include "zcl/zb_zcl.h"
#include "zcl/zb_zcl_ms.h"

#define TAG "ZB_DEV"

static const s_zb_attr_read_t s_pm10_sensor_read_attrs[] = {
    { ZCL_CLUSTER_ID_MS_PM10_MEASUREMENT, 1, { ATTRID_CONCENTRATION_MEASUREMENT_MEASURED_VALUE } },
};

static void
pm10_sensor_init(s_zb_device_t *device, s_zb_function_t *func)
{
    ZB_LOGI(TAG, "0x%llx - %s()", device->ieee_addr, __func__);
    s_zb_device_pm10_sensor_ctx_t *ctx = (s_zb_device_pm10_sensor_ctx_t *)func->ctx;
    ctx->raw_value = 0;
    ctx->value = 0.0f;
}

static void
pm10_sensor_destroy(s_zb_device_t *device, s_zb_function_t *func)
{
    ZB_LOGI(TAG, "0x%llx - %s()", device->ieee_addr, __func__);
}

static const s_zb_attr_read_t*
pm10_sensor_get_read_attrs(s_zb_device_t *device, s_zb_function_t *func)
{
    return s_pm10_sensor_read_attrs;
}

static void
pm10_sensor_notify_state(s_zb_device_t *device, s_zb_function_t *func, s_zb_device_pm10_sensor_ctx_t *ctx)
{
    ZB_LOGD(TAG, "0x%llx - %s()", device->ieee_addr, __func__);
    s_zb_event_t event;
    event.type = ZB_EVENT_SENSOR_PM10;
    event.sensor.value = ctx->value;
    event.sensor.unit = zb_sensor_unit_for_event(event.type);
    zb_device_manager_notify_event(device, func, &event);
}

static void
pm10_sensor_on_attr_report(s_zb_device_t *device, s_zb_function_t *func, s_zb_zcl_incoming_msg_t *msg)
{
    s_zb_device_pm10_sensor_ctx_t *ctx = (s_zb_device_pm10_sensor_ctx_t *)func->ctx;
    bool changed = false;

    if (msg->msg->cluster_id == ZCL_CLUSTER_ID_MS_PM10_MEASUREMENT)
    {
        s_zb_zcl_report_attr_cmd_t *report_attr_cmd = (s_zb_zcl_report_attr_cmd_t *)msg->attr_cmd;
        for (uint8_t i = 0; i < report_attr_cmd->num_attr; i++)
        {
            s_zb_zcl_report_attr_info_t *report_attr_info = &report_attr_cmd->attr_list[i];
            if (report_attr_info->attr_id == ATTRID_CONCENTRATION_MEASUREMENT_MEASURED_VALUE)
            {
                if (report_attr_info->data_type != ZCL_DATATYPE_UINT16)
                {
                    ZB_LOGW(TAG, "0x%llx - %s() - Unexpected PM10 data type: 0x%02x", device->ieee_addr, __func__, report_attr_info->data_type);
                    continue;
                }
                uint16_t raw_value = BUILD_UINT16(report_attr_info->attr_data[0], report_attr_info->attr_data[1]);
                if (ctx->raw_value != raw_value)
                {
                    ctx->raw_value = raw_value;
                    ctx->value = (float)raw_value;
                    ZB_LOGI(TAG, "0x%llx - %s() - PM10: %.1f ug/m3", device->ieee_addr, __func__, ctx->value);
                    changed = true;
                }
            }
            else
            {
                ZB_LOGW(TAG, "0x%llx - %s() - Unsupported attr ID: 0x%04x", device->ieee_addr, __func__, report_attr_info->attr_id);
            }
        }
    }
    else
    {
        ZB_LOGW(TAG, "0x%llx - %s() - Unsupported cluster ID: 0x%04x", device->ieee_addr, __func__, msg->msg->cluster_id);
    }
    if (changed)
    {
        pm10_sensor_notify_state(device, func, ctx);
    }
}


static void
pm10_sensor_on_read_rsp(s_zb_device_t *device, s_zb_function_t *func, s_zb_zcl_incoming_msg_t *msg)
{
    s_zb_device_pm10_sensor_ctx_t *ctx = (s_zb_device_pm10_sensor_ctx_t *)func->ctx;
    bool changed = false;

    if (msg->msg->cluster_id == ZCL_CLUSTER_ID_MS_PM10_MEASUREMENT)
    {
        s_zb_zcl_read_attr_rsp_cmd_t *read_attr_rsp_cmd = (s_zb_zcl_read_attr_rsp_cmd_t *)msg->attr_cmd;
        for (uint8_t i = 0; i < read_attr_rsp_cmd->num_attr; i++)
        {
            s_zb_zcl_read_attr_rsp_info_t *read_attr_rsp_info = &read_attr_rsp_cmd->attr_list[i];
            if (read_attr_rsp_info->attr_id == ATTRID_CONCENTRATION_MEASUREMENT_MEASURED_VALUE && read_attr_rsp_info->status == ZCL_STATUS_SUCCESS)
            {
                if (read_attr_rsp_info->data_type != ZCL_DATATYPE_UINT16)
                {
                    ZB_LOGW(TAG, "0x%llx - %s() - Unexpected PM10 data type: 0x%02x", device->ieee_addr, __func__, read_attr_rsp_info->data_type);
                    continue;
                }
                uint16_t raw_value = BUILD_UINT16(read_attr_rsp_info->data[0], read_attr_rsp_info->data[1]);
                if (ctx->raw_value != raw_value)
                {
                    ctx->raw_value = raw_value;
                    ctx->value = (float)raw_value;
                    ZB_LOGI(TAG, "0x%llx - %s() - PM10: %.1f ug/m3", device->ieee_addr, __func__, ctx->value);
                    changed = true;
                }
            }
            else
            {
                ZB_LOGW(TAG, "0x%llx - %s() - Unhandled attr ID: 0x%04x - status: 0x%02x", device->ieee_addr, __func__, read_attr_rsp_info->attr_id, read_attr_rsp_info->status);
            }
        }
    }
    else
    {
        ZB_LOGW(TAG, "0x%llx - %s() - Unsupported cluster ID: 0x%04x", device->ieee_addr, __func__, msg->msg->cluster_id);
    }
    if (changed)
    {
        pm10_sensor_notify_state(device, func, (s_zb_device_pm10_sensor_ctx_t *)func->ctx);
    }
}

static zb_status_t
pm10_sensor_on_command(s_zb_device_t *device, s_zb_function_t *func, const s_zb_cmd_t *cmd)
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
            read_attr_cmd->attr_id[0] = ATTRID_CONCENTRATION_MEASUREMENT_MEASURED_VALUE;
            status = zb_zcl_send_read(ZB_HUB_ENDPOINT, &addr, ZCL_CLUSTER_ID_MS_PM10_MEASUREMENT, read_attr_cmd, ZCL_FRAME_CLIENT_SERVER_DIR, false, seq_num);
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
pm10_sensor_emit_state(s_zb_device_t *device, s_zb_function_t *func)
{
    pm10_sensor_notify_state(device, func, (s_zb_device_pm10_sensor_ctx_t *)func->ctx);
}

s_zb_function_ops_t zb_device_pm10_sensor_ops = {
    .emit_state = pm10_sensor_emit_state,
    .ctx_size = sizeof(s_zb_device_pm10_sensor_ctx_t),
    .init = pm10_sensor_init,
    .destroy = pm10_sensor_destroy,
    .get_read_attrs = pm10_sensor_get_read_attrs,
    .on_attr_report = pm10_sensor_on_attr_report,
    .on_read_rsp = pm10_sensor_on_read_rsp,
    .on_command = pm10_sensor_on_command,
};
