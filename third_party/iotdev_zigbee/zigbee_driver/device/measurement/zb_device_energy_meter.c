#include "device/measurement/zb_device_energy_meter.h"

#include "common/zb_common.h"
#include "af/zb_af.h"
#include "core/zb_core.h"
#include "device/zb_device_manager.h"
#include "zcl/zb_zcl.h"
#include "zcl/zb_zcl_smart_energy.h"

#define TAG "ZB_DEV"

/*
 * Full-state sync set: cumulative energy + live power. The formatting
 * attributes (multiplier / divisor / UOM / device type) are pulled in the
 * explicit ZB_CMD_READ_STATE path so scaling is established up front.
 */
static const s_zb_attr_read_t s_energy_meter_read_attrs[] = {
    { ZCL_CLUSTER_ID_SE_METERING, 2, { ATTRID_SE_METERING_CURR_SUMM_DLVD, ATTRID_SE_METERING_INST_DMD } },
};

/******************************************************************************
 * Little-endian byte assemblers for the wide metering types
 ******************************************************************************/

static uint64_t energy_meter_build_uint48(const uint8_t *p)
{
    return  (uint64_t)p[0]        | ((uint64_t)p[1] << 8)  | ((uint64_t)p[2] << 16) |
           ((uint64_t)p[3] << 24) | ((uint64_t)p[4] << 32) | ((uint64_t)p[5] << 40);
}

static uint32_t energy_meter_build_uint24(const uint8_t *p)
{
    return (uint32_t)p[0] | ((uint32_t)p[1] << 8) | ((uint32_t)p[2] << 16);
}

static int32_t energy_meter_build_int24(const uint8_t *p)
{
    uint32_t v = energy_meter_build_uint24(p);
    if (v & 0x00800000u)
    {
        v |= 0xFF000000u; /* sign-extend the 24-bit value */
    }
    return (int32_t)v;
}

/******************************************************************************
 * Per-device scaling quirks
 *
 * Like the Electrical Measurement cluster, some devices (Tuya TS011F plugs)
 * report CurrentSummationDelivered on the standard Metering cluster but do NOT
 * advertise the Multiplier/Divisor attributes. These quirks seed ctx->multiplier
 * / ctx->divisor with the known fixed factors; a value a device *does* report
 * later overrides the seed (energy_meter_apply_attr).
 ******************************************************************************/
typedef struct
{
    const char *manufacturer;   /* exact ManufacturerName, NULL = any */
    const char *model;          /* exact ModelIdentifier,  NULL = any */
    uint32_t multiplier;
    uint32_t divisor;
} s_energy_scale_quirk_t;

static const s_energy_scale_quirk_t s_energy_scale_quirks[] = {
    /* Tuya TS011F: CurrentSummationDelivered in 0.01 kWh (x1/100). Confirmed on
     * a _TZ3000_wzmuk9ai plug: summation went 0 -> 1 after ~13.5 Wh consumed
     * (~18 W LED for 45 min), i.e. 1 raw unit = 10 Wh = 0.01 kWh. */
    { "_TZ3000_wzmuk9ai", "TS011F", 1, 100 },
};

static void
energy_meter_apply_scale_quirk(s_zb_device_t *device, s_zb_device_energy_meter_ctx_t *ctx)
{
    for (size_t i = 0; i < ARRAY_SIZE(s_energy_scale_quirks); i++)
    {
        const s_energy_scale_quirk_t *q = &s_energy_scale_quirks[i];
        if (q->manufacturer && strcmp(q->manufacturer, device->manufacturer) != 0)
            continue;
        if (q->model && strcmp(q->model, device->model) != 0)
            continue;
        ctx->multiplier = q->multiplier;
        ctx->divisor = q->divisor;
        ZB_LOGI(TAG, "0x%llx - %s() - applied scaling quirk for '%s' / '%s' (mult=%u div=%u)",
                device->ieee_addr, __func__, device->manufacturer, device->model, ctx->multiplier, ctx->divisor);
        return;
    }
}

/******************************************************************************
 * Scaling
 ******************************************************************************/

static void
energy_meter_recompute(s_zb_device_energy_meter_ctx_t *ctx)
{
    uint32_t mult = ctx->multiplier ? ctx->multiplier : 1;
    uint32_t div  = ctx->divisor    ? ctx->divisor    : 1;
    ctx->energy = (float)ctx->summation_delivered_raw * (float)mult / (float)div;
    ctx->power  = (float)ctx->instantaneous_demand_raw * (float)mult / (float)div;
}

/******************************************************************************
 * Lifecycle
 ******************************************************************************/

static void
energy_meter_init(s_zb_device_t *device, s_zb_function_t *func)
{
    ZB_LOGI(TAG, "0x%llx - %s()", device->ieee_addr, __func__);
    s_zb_device_energy_meter_ctx_t *ctx = (s_zb_device_energy_meter_ctx_t *)func->ctx;
    memset(ctx, 0, sizeof(s_zb_device_energy_meter_ctx_t));
    ctx->multiplier = 1;
    ctx->divisor = 1;
    /* Seed fixed scaling for devices that omit the Multiplier/Divisor attrs;
     * real advertised factors (if any) override this in apply_attr. */
    energy_meter_apply_scale_quirk(device, ctx);
}

static void
energy_meter_destroy(s_zb_device_t *device, s_zb_function_t *func)
{
    ZB_LOGI(TAG, "0x%llx - %s()", device->ieee_addr, __func__);
}

static const s_zb_attr_read_t*
energy_meter_get_read_attrs(s_zb_device_t *device, s_zb_function_t *func)
{
    return s_energy_meter_read_attrs;
}

static void
energy_meter_notify_state(s_zb_device_t *device, s_zb_function_t *func, s_zb_device_energy_meter_ctx_t *ctx)
{
    ZB_LOGD(TAG, "0x%llx - %s()", device->ieee_addr, __func__);
    s_zb_event_t event;
    event.type = ZB_EVENT_SENSOR_ENERGY_METERING;
    event.energy = ctx->energy;
    zb_device_manager_notify_event(device, func, &event);
}

/******************************************************************************
 * Shared attribute decode — used by both report and read-response paths.
 * Returns true if a reportable field (energy / power) changed.
 ******************************************************************************/

static bool
energy_meter_apply_attr(s_zb_device_t *device, s_zb_function_t *func,
                        s_zb_device_energy_meter_ctx_t *ctx, uint16_t attr_id, const uint8_t *data)
{
    bool reportable = false;
    switch (attr_id)
    {
        case ATTRID_SE_METERING_CURR_SUMM_DLVD:
        {
            uint64_t raw = energy_meter_build_uint48(data);
            if (ctx->summation_delivered_raw != raw)
            {
                ctx->summation_delivered_raw = raw;
                reportable = true;
            }
            break;
        }
        case ATTRID_SE_METERING_INST_DMD:
        {
            int32_t raw = energy_meter_build_int24(data);
            if (ctx->instantaneous_demand_raw != raw)
            {
                ctx->instantaneous_demand_raw = raw;
                reportable = true;
            }
            break;
        }
        case ATTRID_SE_METERING_MULT:
            ctx->multiplier = energy_meter_build_uint24(data);
            ZB_LOGI(TAG, "0x%llx - %s() - Multiplier: %u", device->ieee_addr, __func__, ctx->multiplier);
            break;
        case ATTRID_SE_METERING_DIV:
            ctx->divisor = energy_meter_build_uint24(data);
            ZB_LOGI(TAG, "0x%llx - %s() - Divisor: %u", device->ieee_addr, __func__, ctx->divisor);
            break;
        case ATTRID_SE_METERING_UOM:
            ctx->unit_of_measure = data[0];
            break;
        case ATTRID_SE_METERING_DEVICE_TYPE:
            ctx->device_type = data[0];
            break;
        default:
            ZB_LOGW(TAG, "0x%llx - %s() - Unhandled attr ID: 0x%04x", device->ieee_addr, __func__, attr_id);
            return false;
    }

    energy_meter_recompute(ctx);
    return reportable;
}

/******************************************************************************
 * Incoming message handlers
 ******************************************************************************/

static void
energy_meter_on_attr_report(s_zb_device_t *device, s_zb_function_t *func, s_zb_zcl_incoming_msg_t *msg)
{
    s_zb_device_energy_meter_ctx_t *ctx = (s_zb_device_energy_meter_ctx_t *)func->ctx;
    bool changed = false;

    if (msg->msg->cluster_id == ZCL_CLUSTER_ID_SE_METERING)
    {
        s_zb_zcl_report_attr_cmd_t *report_attr_cmd = (s_zb_zcl_report_attr_cmd_t *)msg->attr_cmd;
        for (uint8_t i = 0; i < report_attr_cmd->num_attr; i++)
        {
            s_zb_zcl_report_attr_info_t *report_attr_info = &report_attr_cmd->attr_list[i];
            changed |= energy_meter_apply_attr(device, func, ctx, report_attr_info->attr_id, report_attr_info->attr_data);
        }
    }
    else
    {
        ZB_LOGW(TAG, "0x%llx - %s() - Unsupported cluster ID: 0x%04x", device->ieee_addr, __func__, msg->msg->cluster_id);
    }
    if (changed)
    {
        energy_meter_notify_state(device, func, ctx);
    }
}

static void
energy_meter_on_read_rsp(s_zb_device_t *device, s_zb_function_t *func, s_zb_zcl_incoming_msg_t *msg)
{
    s_zb_device_energy_meter_ctx_t *ctx = (s_zb_device_energy_meter_ctx_t *)func->ctx;
    bool changed = false;

    if (msg->msg->cluster_id == ZCL_CLUSTER_ID_SE_METERING)
    {
        s_zb_zcl_read_attr_rsp_cmd_t *read_attr_rsp_cmd = (s_zb_zcl_read_attr_rsp_cmd_t *)msg->attr_cmd;
        for (uint8_t i = 0; i < read_attr_rsp_cmd->num_attr; i++)
        {
            s_zb_zcl_read_attr_rsp_info_t *read_attr_rsp_info = &read_attr_rsp_cmd->attr_list[i];
            if (read_attr_rsp_info->status != ZCL_STATUS_SUCCESS)
            {
                ZB_LOGW(TAG, "0x%llx - %s() - attr ID: 0x%04x - status: 0x%02x", device->ieee_addr, __func__, read_attr_rsp_info->attr_id, read_attr_rsp_info->status);
                continue;
            }
            changed |= energy_meter_apply_attr(device, func, ctx, read_attr_rsp_info->attr_id, read_attr_rsp_info->data);
        }
    }
    else
    {
        ZB_LOGW(TAG, "0x%llx - %s() - Unsupported cluster ID: 0x%04x", device->ieee_addr, __func__, msg->msg->cluster_id);
    }
    if (changed)
    {
        energy_meter_notify_state(device, func, ctx);
    }
}

/******************************************************************************
 * Command handler
 ******************************************************************************/

static zb_status_t
energy_meter_on_command(s_zb_device_t *device, s_zb_function_t *func, const s_zb_cmd_t *cmd)
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

            uint8_t buf[13] = {0};
            s_zb_zcl_read_attr_cmd_t *read_attr_cmd = (s_zb_zcl_read_attr_cmd_t *)&buf[0];

            /* Live values: cumulative energy + instantaneous demand */
            uint8_t seq_num = zb_zcl_next_seq_num();
            read_attr_cmd->num_attr = 2;
            read_attr_cmd->attr_id[0] = ATTRID_SE_METERING_CURR_SUMM_DLVD;
            read_attr_cmd->attr_id[1] = ATTRID_SE_METERING_INST_DMD;
            status = zb_zcl_send_read(ZB_HUB_ENDPOINT, &addr, ZCL_CLUSTER_ID_SE_METERING, read_attr_cmd, ZCL_FRAME_CLIENT_SERVER_DIR, false, seq_num);
            ZB_LOGI(TAG, "0x%llx - %s() - Sent read (%d)", device->ieee_addr, __func__, status);

            /* Formatting attributes are static — fetch them only on the first
             * sync. A quirk-seeded multiplier/divisor also satisfies this. */
            if (!func->synced)
            {
                seq_num = zb_zcl_next_seq_num();
                read_attr_cmd->num_attr = 4;
                read_attr_cmd->attr_id[0] = ATTRID_SE_METERING_MULT;
                read_attr_cmd->attr_id[1] = ATTRID_SE_METERING_DIV;
                read_attr_cmd->attr_id[2] = ATTRID_SE_METERING_UOM;
                read_attr_cmd->attr_id[3] = ATTRID_SE_METERING_DEVICE_TYPE;
                status = zb_zcl_send_read(ZB_HUB_ENDPOINT, &addr, ZCL_CLUSTER_ID_SE_METERING, read_attr_cmd, ZCL_FRAME_CLIENT_SERVER_DIR, false, seq_num);
                ZB_LOGI(TAG, "0x%llx - %s() - Sent read formatting (%d)", device->ieee_addr, __func__, status);
            }
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
energy_meter_emit_state(s_zb_device_t *device, s_zb_function_t *func)
{
    energy_meter_notify_state(device, func, (s_zb_device_energy_meter_ctx_t *)func->ctx);
}

s_zb_function_ops_t zb_device_energy_meter_ops = {
    .emit_state = energy_meter_emit_state,
    .ctx_size = sizeof(s_zb_device_energy_meter_ctx_t),
    .init = energy_meter_init,
    .destroy = energy_meter_destroy,
    .get_read_attrs = energy_meter_get_read_attrs,
    .on_attr_report = energy_meter_on_attr_report,
    .on_read_rsp = energy_meter_on_read_rsp,
    .on_command = energy_meter_on_command,
};
