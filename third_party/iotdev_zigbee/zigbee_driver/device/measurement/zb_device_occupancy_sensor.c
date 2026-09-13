#include "device/measurement/zb_device_occupancy_sensor.h"

#include "common/zb_common.h"
#include "af/zb_af.h"
#include "core/zb_core.h"
#include "device/zb_device_manager.h"
#include "device/zb_device_schema.h"
#include "osal/zb_osal.h"
#include "zcl/zb_zcl.h"
#include "zcl/zb_zcl_ms.h"

#define TAG "ZB_DEV"

static const s_zb_attr_read_t s_occupancy_sensor_read_attrs[] = {
    { ZCL_CLUSTER_ID_MS_OCCUPANCY_SENSING, 3,
      { ATTRID_OCCUPANCY_SENSING_OCCUPANCY,
        ATTRID_OCCUPANCY_SENSING_OCCUPANCY_SENSOR_TYPE,
        ATTRID_OCCUPANCY_SENSING_OCCUPANCY_SENSOR_TYPE_BITMAP }
    },
    { ZCL_CLUSTER_ID_MS_OCCUPANCY_SENSING, 3,
      { ATTRID_OCCUPANCY_SENSING_PIR_OCCUPIED_TO_UNOCCUPIED_DELAY,
        ATTRID_OCCUPANCY_SENSING_PIR_UNOCCUPIED_TO_OCCUPIED_DELAY,
        ATTRID_OCCUPANCY_SENSING_PIR_UNOCCUPIED_TO_OCCUPIED_THRESHOLD }
    },
    { ZCL_CLUSTER_ID_MS_OCCUPANCY_SENSING, 3,
      { ATTRID_OCCUPANCY_SENSING_ULTRASONIC_OCCUPIED_TO_UNOCCUPIED_DELAY,
        ATTRID_OCCUPANCY_SENSING_ULTRASONIC_UNOCCUPIED_TO_OCCUPIED_DELAY,
        ATTRID_OCCUPANCY_SENSING_ULTRASONIC_UNOCCUPIED_TO_OCCUPIED_THRESHOLD }
    },
    { ZCL_CLUSTER_ID_MS_OCCUPANCY_SENSING, 3,
      { ATTRID_OCCUPANCY_SENSING_PHYSICAL_CONTACT_OCCUPIED_TO_UNOCCUPIED_DELAY,
        ATTRID_OCCUPANCY_SENSING_PHYSICAL_CONTACT_UNOCCUPIED_TO_OCCUPIED_DELAY,
        ATTRID_OCCUPANCY_SENSING_PHYSICAL_CONTACT_UNOCCUPIED_TO_OCCUPIED_THRESHOLD }
    },
};

static void
occupancy_sensor_init(s_zb_device_t *device, s_zb_function_t *func)
{
    ZB_LOGI(TAG, "0x%llx - %s()", device->ieee_addr, __func__);
    s_zb_device_occupancy_sensor_ctx_t *ctx = (s_zb_device_occupancy_sensor_ctx_t *)func->ctx;
    ctx->occupied = false;
    ctx->sensor_type = 0;
    ctx->clear_at_ms = 0;
    /* 0 for a standards-compliant sensor: it sends occupancy=0 itself and must
     * not be second-guessed. Non-zero only for models known to stay silent. */
    ctx->auto_clear_ms = zb_device_schema_resolve_occupancy_clear_ms(device->manufacturer, device->model);
    if (ctx->auto_clear_ms != 0)
    {
        ZB_LOGI(TAG, "0x%llx - %s() - motion auto-clear after %u ms",
                device->ieee_addr, __func__, ctx->auto_clear_ms);
    }
}

/* Apply a freshly received occupancy value, arming or cancelling the auto-clear
 * deadline with it. Returns true if the state changed. */
static bool
occupancy_sensor_set_occupied(s_zb_device_t *device, s_zb_device_occupancy_sensor_ctx_t *ctx, bool occupied)
{
    bool changed = (ctx->occupied != occupied);

    if (changed)
    {
        ZB_LOGI(TAG, "0x%llx - OCCUPANCY: (%d) -> (%d)", device->ieee_addr, ctx->occupied, occupied);
    }
    ctx->occupied = occupied;

    /* Re-armed on every detection - including one that did not change the
     * state - so sustained motion keeps pushing the deadline out. */
    ctx->clear_at_ms = (occupied && ctx->auto_clear_ms != 0)
                           ? (zb_os_now_ms() + ctx->auto_clear_ms)
                           : 0;
    return changed;
}

static void
occupancy_sensor_destroy(s_zb_device_t *device, s_zb_function_t *func)
{
    ZB_LOGI(TAG, "0x%llx - %s()", device->ieee_addr, __func__);
}

static const s_zb_attr_read_t*
occupancy_sensor_get_read_attrs(s_zb_device_t *device, s_zb_function_t *func)
{
    ZB_LOGI(TAG, "0x%llx - %s()", device->ieee_addr, __func__);
    return s_occupancy_sensor_read_attrs;
}

static void
occupancy_sensor_notify_state(s_zb_device_t *device, s_zb_function_t *func, s_zb_device_occupancy_sensor_ctx_t *ctx)
{
    ZB_LOGD(TAG, "0x%llx - %s()", device->ieee_addr, __func__);
    s_zb_event_t event;
    event.type = ZB_EVENT_SENSOR_OCCUPANCY;
    event.binary.active = ctx->occupied;
    zb_device_manager_notify_event(device, func, &event);
}

static void
occupancy_sensor_on_attr_report(s_zb_device_t *device, s_zb_function_t *func, s_zb_zcl_incoming_msg_t *msg)
{
    ZB_LOGI(TAG, "0x%llx - %s()", device->ieee_addr, __func__);
    s_zb_device_occupancy_sensor_ctx_t *ctx = (s_zb_device_occupancy_sensor_ctx_t *)func->ctx;
    bool changed = false;

    if (msg->msg->cluster_id == ZCL_CLUSTER_ID_MS_OCCUPANCY_SENSING)
    {
        s_zb_zcl_report_attr_cmd_t *report_attr_cmd = (s_zb_zcl_report_attr_cmd_t *)msg->attr_cmd;
        for (uint8_t i = 0; i < report_attr_cmd->num_attr; i++)
        {
            s_zb_zcl_report_attr_info_t *report_attr_info = &report_attr_cmd->attr_list[i];
            if (report_attr_info->attr_id == ATTRID_OCCUPANCY_SENSING_OCCUPANCY)
            {
                changed = occupancy_sensor_set_occupied(device, ctx, report_attr_info->attr_data[0] != 0);
                break;
            }
            else if (report_attr_info->attr_id == ATTRID_OCCUPANCY_SENSING_PIR_OCCUPIED_TO_UNOCCUPIED_DELAY)
            {
                /* The device's own idea of how long a detection lasts. For a
                 * sensor that never sends occupancy=0 this is what sizes the
                 * local timeout - prefer it over the per-model fallback. The
                 * 2 s of slack keeps a periodic re-detection from racing the
                 * deadline. */
                uint16_t delay_s = BUILD_UINT16(report_attr_info->attr_data[0], report_attr_info->attr_data[1]);
                ctx->pir_config.occupied_to_unoccupied_delay = delay_s;
                if (ctx->auto_clear_ms != 0 && delay_s != 0)
                {
                    ctx->auto_clear_ms = ((uint32_t)delay_s + 2u) * 1000u;
                    ZB_LOGI(TAG, "0x%llx - %s() - motion auto-clear now %u ms (device delay %u s)",
                            device->ieee_addr, __func__, ctx->auto_clear_ms, delay_s);
                }
                break;
            }
            else
            {
                ZB_LOGW(TAG, "0x%llx - %s() - Unsupported attribute ID: 0x%04x", device->ieee_addr, __func__, report_attr_info->attr_id);
            }
        }
    }
    else
    {
        ZB_LOGW(TAG, "0x%llx - %s() - Unsupported cluster ID: 0x%04x", device->ieee_addr, __func__, msg->msg->cluster_id);
    }
    if (changed)
    {
        occupancy_sensor_notify_state(device, func, ctx);
    }
}

static void
occupancy_sensor_on_read_rsp(s_zb_device_t *device, s_zb_function_t *func, s_zb_zcl_incoming_msg_t *msg)
{
    ZB_LOGI(TAG, "0x%llx - %s()", device->ieee_addr, __func__);
    s_zb_device_occupancy_sensor_ctx_t *ctx = (s_zb_device_occupancy_sensor_ctx_t *)func->ctx;
    bool changed = false;

    if (msg->msg->cluster_id == ZCL_CLUSTER_ID_MS_OCCUPANCY_SENSING)
    {
        s_zb_zcl_read_attr_rsp_cmd_t *read_attr_rsp_cmd = (s_zb_zcl_read_attr_rsp_cmd_t *)msg->attr_cmd;
        for (uint8_t i = 0; i < read_attr_rsp_cmd->num_attr; i++)
        {
            s_zb_zcl_read_attr_rsp_info_t *read_attr_rsp_info = &read_attr_rsp_cmd->attr_list[i];
            if (read_attr_rsp_info->status == ZCL_STATUS_SUCCESS)
            {
                switch (read_attr_rsp_info->attr_id)
                {
                    case ATTRID_OCCUPANCY_SENSING_OCCUPANCY:
                    {
                        changed = occupancy_sensor_set_occupied(device, ctx, read_attr_rsp_info->data[0] != 0);
                        break;
                    }
                    case ATTRID_OCCUPANCY_SENSING_PIR_OCCUPIED_TO_UNOCCUPIED_DELAY:
                    {
                        /* Already in the read set above; used to size the local
                         * motion timeout. See the report path for the rationale. */
                        uint16_t delay_s = BUILD_UINT16(read_attr_rsp_info->data[0], read_attr_rsp_info->data[1]);
                        ctx->pir_config.occupied_to_unoccupied_delay = delay_s;
                        if (ctx->auto_clear_ms != 0 && delay_s != 0)
                        {
                            ctx->auto_clear_ms = ((uint32_t)delay_s + 2u) * 1000u;
                            ZB_LOGI(TAG, "0x%llx - %s() - motion auto-clear now %u ms (device delay %u s)",
                                    device->ieee_addr, __func__, ctx->auto_clear_ms, delay_s);
                        }
                        break;
                    }
                    case ATTRID_OCCUPANCY_SENSING_OCCUPANCY_SENSOR_TYPE:
                        ctx->sensor_type = read_attr_rsp_info->data[0];
                        break;
                    case ATTRID_OCCUPANCY_SENSING_OCCUPANCY_SENSOR_TYPE_BITMAP:
                        ctx->sensor_type_bitmap = read_attr_rsp_info->data[0];
                        break;
                    default:
                    {
                        ZB_LOGW(TAG, "0x%llx - %s() - Unsupported attribute ID: 0x%04x", device->ieee_addr, __func__, read_attr_rsp_info->attr_id);
                        break;
                    }
                }
            }
            else
            {
                ZB_LOGW(TAG, "0x%llx - %s() - Failed to read attribute ID: 0x%04x, status: 0x%02x", device->ieee_addr, __func__, read_attr_rsp_info->attr_id, read_attr_rsp_info->status);
            }
        }
    }
    else
    {
        ZB_LOGW(TAG, "0x%llx - %s() - Unsupported cluster ID: 0x%04x", device->ieee_addr, __func__, msg->msg->cluster_id);
    }
    if (changed)
    {
        occupancy_sensor_notify_state(device, func, ctx);
    }
}

static zb_status_t
occupancy_sensor_on_command(s_zb_device_t *device, s_zb_function_t *func, const s_zb_cmd_t *cmd)
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
            uint8_t buf[7] = {0};
            s_zb_zcl_read_attr_cmd_t *read_attr_cmd = (s_zb_zcl_read_attr_cmd_t *)&buf[0];
            read_attr_cmd->num_attr = 3;
            read_attr_cmd->attr_id[0] = ATTRID_OCCUPANCY_SENSING_OCCUPANCY;
            read_attr_cmd->attr_id[1] = ATTRID_OCCUPANCY_SENSING_OCCUPANCY_SENSOR_TYPE;
            read_attr_cmd->attr_id[2] = ATTRID_OCCUPANCY_SENSING_OCCUPANCY_SENSOR_TYPE_BITMAP;
            status = zb_zcl_send_read(ZB_HUB_ENDPOINT, &addr, ZCL_CLUSTER_ID_MS_OCCUPANCY_SENSING, read_attr_cmd, ZCL_FRAME_CLIENT_SERVER_DIR, false, seq_num);
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
occupancy_sensor_emit_state(s_zb_device_t *device, s_zb_function_t *func)
{
    occupancy_sensor_notify_state(device, func, (s_zb_device_occupancy_sensor_ctx_t *)func->ctx);
}

/* Age out a detection from a sensor that never reports the end of one. Only
 * ever armed when auto_clear_ms is set, so a standards-compliant sensor is
 * untouched by this. */
static void
occupancy_sensor_tick(s_zb_device_t *device, s_zb_function_t *func, uint32_t now_ms)
{
    s_zb_device_occupancy_sensor_ctx_t *ctx = (s_zb_device_occupancy_sensor_ctx_t *)func->ctx;

    if (ctx->clear_at_ms == 0)
    {
        return;                                     /* not armed */
    }
    if ((int32_t)(now_ms - ctx->clear_at_ms) < 0)
    {
        return;                                     /* signed compare tolerates wrap */
    }

    ZB_LOGI(TAG, "0x%llx - %s() - motion timed out, clearing occupancy", device->ieee_addr, __func__);
    ctx->clear_at_ms = 0;
    ctx->occupied    = false;
    occupancy_sensor_notify_state(device, func, ctx);
}

s_zb_function_ops_t zb_device_occupancy_sensor_ops = {
    .tick = occupancy_sensor_tick,
    .emit_state = occupancy_sensor_emit_state,
    .ctx_size = sizeof(s_zb_device_occupancy_sensor_ctx_t),
    .init = occupancy_sensor_init,
    .destroy = occupancy_sensor_destroy,
    .get_read_attrs = occupancy_sensor_get_read_attrs,
    .on_attr_report = occupancy_sensor_on_attr_report,
    .on_read_rsp = occupancy_sensor_on_read_rsp,
    .on_command = occupancy_sensor_on_command,
};