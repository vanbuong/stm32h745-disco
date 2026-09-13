#include "device/measurement/zb_device_illuminance_sensor.h"

#include "common/zb_common.h"
#include "common/zb_sensor_units.h"
#include "af/zb_af.h"
#include "device/zb_device_manager.h"
#include "device/zb_device_schema.h"
#include "zcl/zb_zcl.h"
#include "zcl/zb_zcl_ms.h"

#define TAG "ZB_DEV"

static const s_zb_attr_read_t s_illuminance_sensor_read_attrs[] = {
    { ZCL_CLUSTER_ID_MS_ILLUMINANCE_MEASUREMENT, 1, { ATTRID_ILLUMINANCE_MEASUREMENT_MEASURED_VALUE } },
};

static void
illuminance_sensor_init(s_zb_device_t *device, s_zb_function_t *func)
{
    ZB_LOGI(TAG, "0x%llx - %s()", device->ieee_addr, __func__);
    s_zb_device_illuminance_sensor_ctx_t *ctx = (s_zb_device_illuminance_sensor_ctx_t *)func->ctx;
    ctx->raw_value = 0;
    ctx->lux = 0.0f;
}

static void
illuminance_sensor_destroy(s_zb_device_t *device, s_zb_function_t *func)
{
    ZB_LOGI(TAG, "0x%llx - %s()", device->ieee_addr, __func__);
}

static const s_zb_attr_read_t*
illuminance_sensor_get_read_attrs(s_zb_device_t *device, s_zb_function_t *func)
{
    return s_illuminance_sensor_read_attrs;
}

/* 0xFFFF is the ZCL "invalid / unknown" MeasuredValue, and Aqara sensors also
 * emit 0xFFFE when they have no reading - both would otherwise surface as a
 * blinding 65534 lx. */
#define ILLUMINANCE_RAW_INVALID_MIN     0xFFFEu

/*
 * MeasuredValue -> lux.
 *
 * ZCL defines the attribute logarithmically, but some vendors put lux straight
 * in it; the device schema says which (see s_zb_illuminance_quirk_t). Getting
 * this wrong is quiet rather than obvious - a raw 3000 lx decoded as
 * logarithmic gives 1.99 lx, a plausible-looking number in a bright room.
 */
static float
illuminance_sensor_raw_to_lux(const s_zb_device_t *device, uint16_t raw_value)
{
    if (raw_value >= ILLUMINANCE_RAW_INVALID_MIN)
    {
        return 0.0f;
    }
    if (zb_device_schema_illuminance_is_raw_lux(device->manufacturer, device->model))
    {
        return (float)raw_value;
    }
    /* Logarithmic per ZCL: 0 means "too dark to measure", and 1 decodes to
     * 1 lx, so anything at or below 1 is reported as zero. */
    return (raw_value > 1u) ? powf(10.0f, ((float)raw_value - 1.0f) / 10000.0f) : 0.0f;
}

static void
illuminance_sensor_notify_state(s_zb_device_t *device, s_zb_function_t *func, s_zb_device_illuminance_sensor_ctx_t *ctx)
{
    ZB_LOGD(TAG, "0x%llx - %s()", device->ieee_addr, __func__);
    s_zb_event_t event;
    event.type = ZB_EVENT_SENSOR_ILLUMINANCE;
    event.sensor.value = ctx->lux;
    event.sensor.unit = zb_sensor_unit_for_event(event.type);
    zb_device_manager_notify_event(device, func, &event);
}

static void
illuminance_sensor_on_event(s_zb_device_t *device, s_zb_function_t *func, s_zb_zcl_incoming_msg_t *msg)
{
    s_zb_device_illuminance_sensor_ctx_t *ctx = (s_zb_device_illuminance_sensor_ctx_t *)func->ctx;
    bool changed = false;

    if (msg->msg->cluster_id == ZCL_CLUSTER_ID_MS_ILLUMINANCE_MEASUREMENT)
    {
        if (msg->hdr.command_id == ZCL_CMD_REPORT)
        {
            s_zb_zcl_report_attr_cmd_t *report_attr_cmd = (s_zb_zcl_report_attr_cmd_t *)msg->attr_cmd;
            for (uint8_t i = 0; i < report_attr_cmd->num_attr; i++)
            {
                s_zb_zcl_report_attr_info_t *report_attr_info = &report_attr_cmd->attr_list[i];
                if (report_attr_info->attr_id == ATTRID_ILLUMINANCE_MEASUREMENT_MEASURED_VALUE)
                {
                    uint16_t raw_value = BUILD_UINT16(report_attr_info->attr_data[0], report_attr_info->attr_data[1]);
                    if (ctx->raw_value != raw_value)
                    {
                        ZB_LOGI(TAG, "0x%llx - %s() - RAW_VALUE: (%d) -> (%d)", device->ieee_addr, __func__, ctx->raw_value, raw_value);
                        changed = true;
                    }
                    ctx->raw_value = raw_value;
                    ctx->lux = illuminance_sensor_raw_to_lux(device, raw_value);
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
                if (read_attr_rsp_info->attr_id == ATTRID_ILLUMINANCE_MEASUREMENT_MEASURED_VALUE && read_attr_rsp_info->status == ZCL_STATUS_SUCCESS)
                {
                    uint16_t raw_value = BUILD_UINT16(read_attr_rsp_info->data[0], read_attr_rsp_info->data[1]);
                    if (ctx->raw_value != raw_value)
                    {
                        ZB_LOGI(TAG, "0x%llx - %s() - RAW_VALUE: (%d) -> (%d)", device->ieee_addr, __func__, ctx->raw_value, raw_value);
                        changed = true;
                        ctx->raw_value = raw_value;
                        ctx->lux = illuminance_sensor_raw_to_lux(device, raw_value);
                    }
                }
            }
        }
        else
        {
            ZB_LOGW(TAG, "0x%llx - %s() - Unsupported command ID: 0x%02x", device->ieee_addr, __func__, msg->hdr.command_id);
        }
    }
    else
    {
        ZB_LOGW(TAG, "0x%llx - %s() - Unsupported cluster ID: 0x%04x", device->ieee_addr, __func__, msg->msg->cluster_id);
    }
    if (changed)
    {
        illuminance_sensor_notify_state(device, func, ctx);
    }

}

static void
illuminance_sensor_on_attr_report(s_zb_device_t *device, s_zb_function_t *func, s_zb_zcl_incoming_msg_t *msg)
{
    ZB_LOGI(TAG, "0x%llx - %s()", device->ieee_addr, __func__);
    illuminance_sensor_on_event(device, func, msg);
}

static void
illuminance_sensor_on_read_rsp(s_zb_device_t *device, s_zb_function_t *func, s_zb_zcl_incoming_msg_t *msg)
{
    ZB_LOGI(TAG, "0x%llx - %s()", device->ieee_addr, __func__);
    illuminance_sensor_on_event(device, func, msg);
}

static zb_status_t
illuminance_sensor_on_command(s_zb_device_t *device, s_zb_function_t *func, const s_zb_cmd_t *cmd)
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
            uint8_t buf[3] = {0};
            s_zb_zcl_read_attr_cmd_t *read_attr_cmd = (s_zb_zcl_read_attr_cmd_t *)&buf[0];
            read_attr_cmd->num_attr = 1;
            read_attr_cmd->attr_id[0] = ATTRID_ILLUMINANCE_MEASUREMENT_MEASURED_VALUE;
            status = zb_zcl_send_read(ZB_HUB_ENDPOINT, &addr, ZCL_CLUSTER_ID_MS_ILLUMINANCE_MEASUREMENT, read_attr_cmd, ZCL_FRAME_CLIENT_SERVER_DIR, false, seq_num);
            ZB_LOGI(TAG, "0x%llx - %s() - Sent read (%d)", device->ieee_addr, __func__, status);
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
illuminance_sensor_emit_state(s_zb_device_t *device, s_zb_function_t *func)
{
    illuminance_sensor_notify_state(device, func, (s_zb_device_illuminance_sensor_ctx_t *)func->ctx);
}

s_zb_function_ops_t zb_device_illuminance_sensor_ops = {
    .emit_state = illuminance_sensor_emit_state,
    .ctx_size = sizeof(s_zb_device_illuminance_sensor_ctx_t),
    .init = illuminance_sensor_init,
    .destroy = illuminance_sensor_destroy,
    .on_attr_report = illuminance_sensor_on_attr_report,
    .on_read_rsp = illuminance_sensor_on_read_rsp,
    .get_read_attrs = illuminance_sensor_get_read_attrs,
    .on_command = illuminance_sensor_on_command,
};