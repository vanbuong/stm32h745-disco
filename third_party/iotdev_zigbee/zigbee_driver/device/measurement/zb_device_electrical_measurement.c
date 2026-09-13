#include "device/measurement/zb_device_electrical_measurement.h"

#include "common/zb_common.h"
#include "af/zb_af.h"
#include "core/zb_core.h"
#include "device/zb_device_manager.h"
#include "zcl/zb_zcl.h"
#include "zcl/zb_zcl_electrical_measurement.h"

#define TAG "ZB_DEV"

/* ZCL "invalid / unknown" sentinels for the analog attributes */
#define EM_UINT16_INVALID 0xFFFFu
#define EM_INT16_INVALID  0x8000u
#define EM_INT8_INVALID   0x80u

/******************************************************************************
 * Per-device scaling quirks
 *
 * Some devices (notably Tuya TS011F plugs) carry live V/I/P on the standard
 * Electrical Measurement cluster but do NOT advertise the AC Multiplier/Divisor
 * formatting attributes (reads return UNSUPPORTED_ATTRIBUTE). Without them the
 * unity fallback misreports any quantity whose native unit is not the ZCL base
 * unit. These quirks seed ctx->scale with the known fixed factors; any factor a
 * device *does* report later overrides the seed (electrical_measurement_apply_attr).
 *
 * A 0 multiplier/divisor means "unity" (see electrical_measurement_recompute),
 * so only the non-unity factors need to be listed.
 ******************************************************************************/
typedef struct
{
    const char *manufacturer;   /* exact ManufacturerName, NULL = any */
    const char *model;          /* exact ModelIdentifier,  NULL = any */
    s_zb_device_em_scaling_t scale;
} s_em_scale_quirk_t;

static const s_em_scale_quirk_t s_em_scale_quirks[] = {
    /* Tuya TS011F: RMSVoltage in V, RMSCurrent in mA, ActivePower in W.
     * Derived from a _TZ3000_wzmuk9ai plug driving an ~18 W LED:
     *   V=237 (->237 V), I=85 (->0.085 A), P=18 (->18 W); S=V*I=20.1 VA, PF~0.9.
     * Only current is non-unity (mA -> A). */
    { "_TZ3000_wzmuk9ai", "TS011F", { .ac_curr_div = 1000 } },
};

static void
electrical_measurement_apply_scale_quirk(s_zb_device_t *device, s_zb_device_electrical_measurement_ctx_t *ctx)
{
    for (size_t i = 0; i < ARRAY_SIZE(s_em_scale_quirks); i++)
    {
        const s_em_scale_quirk_t *q = &s_em_scale_quirks[i];
        if (q->manufacturer && strcmp(q->manufacturer, device->manufacturer) != 0)
            continue;
        if (q->model && strcmp(q->model, device->model) != 0)
            continue;
        ctx->scale = q->scale;
        ZB_LOGI(TAG, "0x%llx - %s() - applied scaling quirk for '%s' / '%s'", device->ieee_addr, __func__, device->manufacturer, device->model);
        return;
    }
}

/* Full-state sync set: the live phase-A values plus the measurement type. */
static const s_zb_attr_read_t s_electrical_measurement_read_attrs[] = {
    { ZCL_CLUSTER_ID_MS_ELECTRICAL_MEASUREMENT, 8,
      { ATTRID_ELECTRICAL_MEASUREMENT_RMS_VOLTAGE,
        ATTRID_ELECTRICAL_MEASUREMENT_RMS_CURRENT,
        ATTRID_ELECTRICAL_MEASUREMENT_ACTIVE_POWER,
        ATTRID_ELECTRICAL_MEASUREMENT_REACTIVE_POWER,
        ATTRID_ELECTRICAL_MEASUREMENT_APPARENT_POWER,
        ATTRID_ELECTRICAL_MEASUREMENT_POWER_FACTOR,
        ATTRID_ELECTRICAL_MEASUREMENT_AC_FREQUENCY,
        ATTRID_ELECTRICAL_MEASUREMENT_MEASUREMENT_TYPE } },
};

/******************************************************************************
 * Scaling — actual = raw * multiplier / divisor (ZCL §); a 0 multiplier or
 * divisor means the device omits the attribute, so fall back to unity.
 ******************************************************************************/

static void
electrical_measurement_recompute(s_zb_device_electrical_measurement_ctx_t *ctx)
{
    uint16_t vmul = ctx->scale.ac_volt_mult  ? ctx->scale.ac_volt_mult  : 1;
    uint16_t vdiv = ctx->scale.ac_volt_div   ? ctx->scale.ac_volt_div   : 1;
    uint16_t cmul = ctx->scale.ac_curr_mult  ? ctx->scale.ac_curr_mult  : 1;
    uint16_t cdiv = ctx->scale.ac_curr_div   ? ctx->scale.ac_curr_div   : 1;
    uint16_t pmul = ctx->scale.ac_power_mult ? ctx->scale.ac_power_mult : 1;
    uint16_t pdiv = ctx->scale.ac_power_div  ? ctx->scale.ac_power_div  : 1;
    uint16_t fmul = ctx->scale.ac_freq_mult  ? ctx->scale.ac_freq_mult  : 1;
    uint16_t fdiv = ctx->scale.ac_freq_div   ? ctx->scale.ac_freq_div   : 1;

    /* A field the device never reported stays at its invalid sentinel; surface
     * it as 0.0 rather than scaling the sentinel into garbage. */
    ctx->voltage_a        = (ctx->rms_voltage_raw == EM_UINT16_INVALID)
                              ? 0.0f : (float)ctx->rms_voltage_raw  * (float)vmul / (float)vdiv;
    ctx->current_a        = (ctx->rms_current_raw == EM_UINT16_INVALID)
                              ? 0.0f : (float)ctx->rms_current_raw  * (float)cmul / (float)cdiv;
    ctx->active_power_a   = ((uint16_t)ctx->active_power_raw == EM_INT16_INVALID)
                              ? 0.0f : (float)ctx->active_power_raw * (float)pmul / (float)pdiv;
    ctx->ac_frequency     = (ctx->ac_frequency_raw == EM_UINT16_INVALID)
                              ? 0.0f : (float)ctx->ac_frequency_raw * (float)fmul / (float)fdiv;
    /* Reactive/apparent power share the AC power multiplier+divisor (ZCL §). */
    ctx->reactive_power_a = ((uint16_t)ctx->reactive_power_raw == EM_INT16_INVALID)
                              ? 0.0f : (float)ctx->reactive_power_raw * (float)pmul / (float)pdiv;
    ctx->apparent_power_a = (ctx->apparent_power_raw == EM_UINT16_INVALID)
                              ? 0.0f : (float)ctx->apparent_power_raw * (float)pmul / (float)pdiv;
}

/******************************************************************************
 * Lifecycle
 ******************************************************************************/

static void
electrical_measurement_init(s_zb_device_t *device, s_zb_function_t *func)
{
    ZB_LOGI(TAG, "0x%llx - %s()", device->ieee_addr, __func__);
    s_zb_device_electrical_measurement_ctx_t *ctx = (s_zb_device_electrical_measurement_ctx_t *)func->ctx;
    memset(ctx, 0, sizeof(s_zb_device_electrical_measurement_ctx_t));
    ctx->rms_voltage_raw    = EM_UINT16_INVALID;
    ctx->rms_current_raw    = EM_UINT16_INVALID;
    ctx->active_power_raw   = (int16_t)EM_INT16_INVALID;
    ctx->ac_frequency_raw   = EM_UINT16_INVALID;
    ctx->reactive_power_raw = (int16_t)EM_INT16_INVALID;
    ctx->apparent_power_raw = EM_UINT16_INVALID;
    ctx->power_factor_a     = (int8_t)EM_INT8_INVALID;
    /* Seed fixed scaling for devices that omit the AC Multiplier/Divisor attrs;
     * real advertised factors (if any) override this in apply_attr. */
    electrical_measurement_apply_scale_quirk(device, ctx);
}

static void
electrical_measurement_destroy(s_zb_device_t *device, s_zb_function_t *func)
{
    ZB_LOGI(TAG, "0x%llx - %s()", device->ieee_addr, __func__);
}

static const s_zb_attr_read_t*
electrical_measurement_get_read_attrs(s_zb_device_t *device, s_zb_function_t *func)
{
    return s_electrical_measurement_read_attrs;
}

static void
electrical_measurement_notify_state(s_zb_device_t *device, s_zb_function_t *func, s_zb_device_electrical_measurement_ctx_t *ctx)
{
    ZB_LOGD(TAG, "0x%llx - %s()", device->ieee_addr, __func__);
    s_zb_event_t event;
    event.type = ZB_EVENT_SENSOR_ELECTRICAL_MEASUREMENT;
    event.electrical_measurement.measurement_type = ctx->measurement_type;
    event.electrical_measurement.voltage = ctx->voltage_a;
    event.electrical_measurement.current = ctx->current_a;
    event.electrical_measurement.power = ctx->active_power_a;
    event.electrical_measurement.frequency = ctx->ac_frequency;
    event.electrical_measurement.reactive_power = ctx->reactive_power_a;
    event.electrical_measurement.apparent_power = ctx->apparent_power_a;
    event.electrical_measurement.power_factor = ((uint8_t)ctx->power_factor_a == EM_INT8_INVALID)
                                                  ? 0.0f : (float)ctx->power_factor_a / 100.0f;
    zb_device_manager_notify_event(device, func, &event);
}

/******************************************************************************
 * Shared attribute decode - used by both report and read-response paths.
 * Returns true if a reportable field (V / I / P) changed.
 ******************************************************************************/

static bool
electrical_measurement_apply_attr(s_zb_device_t *device, s_zb_function_t *func,
                                  s_zb_device_electrical_measurement_ctx_t *ctx, uint16_t attr_id, const uint8_t *data)
{
    bool reportable = false;
    switch (attr_id)
    {
        case ATTRID_ELECTRICAL_MEASUREMENT_MEASUREMENT_TYPE:
        {
            uint32_t v = BUILD_UINT32(data[0], data[1], data[2], data[3]);
            if (ctx->measurement_type != v)
            {
                ctx->measurement_type = v;
                ZB_LOGI(TAG, "0x%llx - %s() - Measurement type: 0x%08x", device->ieee_addr, __func__, ctx->measurement_type);
            }
            break;
        }
        case ATTRID_ELECTRICAL_MEASUREMENT_RMS_VOLTAGE:
        {
            uint16_t raw = BUILD_UINT16(data[0], data[1]);
            if (raw != EM_UINT16_INVALID && ctx->rms_voltage_raw != raw)
            {
                ctx->rms_voltage_raw = raw;
                reportable = true;
            }
            break;
        }
        case ATTRID_ELECTRICAL_MEASUREMENT_RMS_CURRENT:
        {
            uint16_t raw = BUILD_UINT16(data[0], data[1]);
            if (raw != EM_UINT16_INVALID && ctx->rms_current_raw != raw)
            {
                ctx->rms_current_raw = raw;
                reportable = true;
            }
            break;
        }
        case ATTRID_ELECTRICAL_MEASUREMENT_ACTIVE_POWER:
        {
            int16_t raw = (int16_t)BUILD_UINT16(data[0], data[1]);
            if ((uint16_t)raw != EM_INT16_INVALID && ctx->active_power_raw != raw)
            {
                ctx->active_power_raw = raw;
                reportable = true;
            }
            break;
        }
        case ATTRID_ELECTRICAL_MEASUREMENT_REACTIVE_POWER:
        {
            int16_t raw = (int16_t)BUILD_UINT16(data[0], data[1]);
            if ((uint16_t)raw != EM_INT16_INVALID && ctx->reactive_power_raw != raw)
            {
                ctx->reactive_power_raw = raw;
                reportable = true;
            }
            break;
        }
        case ATTRID_ELECTRICAL_MEASUREMENT_APPARENT_POWER:
        {
            uint16_t raw = BUILD_UINT16(data[0], data[1]);
            if (raw != EM_UINT16_INVALID && ctx->apparent_power_raw != raw)
            {
                ctx->apparent_power_raw = raw;
                reportable = true;
            }
            break;
        }
        case ATTRID_ELECTRICAL_MEASUREMENT_POWER_FACTOR:
        {
            int8_t raw = (int8_t)data[0];
            if ((uint8_t)raw != EM_INT8_INVALID && ctx->power_factor_a != raw)
            {
                ctx->power_factor_a = raw; /* direct: no multiplier/divisor */
                reportable = true;
            }
            break;
        }
        case ATTRID_ELECTRICAL_MEASUREMENT_AC_FREQUENCY:
        {
            uint16_t raw = BUILD_UINT16(data[0], data[1]);
            if (raw != EM_UINT16_INVALID && ctx->ac_frequency_raw != raw)
            {
                ctx->ac_frequency_raw = raw;
                reportable = true;
            }
            break;
        }
        case ATTRID_ELECTRICAL_MEASUREMENT_AC_FREQUENCY_MULTIPLIER:
            ctx->scale.ac_freq_mult = BUILD_UINT16(data[0], data[1]);
            break;
        case ATTRID_ELECTRICAL_MEASUREMENT_AC_FREQUENCY_DIVISOR:
            ctx->scale.ac_freq_div = BUILD_UINT16(data[0], data[1]);
            break;
        case ATTRID_ELECTRICAL_MEASUREMENT_AC_VOLTAGE_MULTIPLIER:
            ctx->scale.ac_volt_mult = BUILD_UINT16(data[0], data[1]);
            break;
        case ATTRID_ELECTRICAL_MEASUREMENT_AC_VOLTAGE_DIVISOR:
            ctx->scale.ac_volt_div = BUILD_UINT16(data[0], data[1]);
            break;
        case ATTRID_ELECTRICAL_MEASUREMENT_AC_CURRENT_MULTIPLIER:
            ctx->scale.ac_curr_mult = BUILD_UINT16(data[0], data[1]);
            break;
        case ATTRID_ELECTRICAL_MEASUREMENT_AC_CURRENT_DIVISOR:
            ctx->scale.ac_curr_div = BUILD_UINT16(data[0], data[1]);
            break;
        case ATTRID_ELECTRICAL_MEASUREMENT_AC_POWER_MULTIPLIER:
            ctx->scale.ac_power_mult = BUILD_UINT16(data[0], data[1]);
            break;
        case ATTRID_ELECTRICAL_MEASUREMENT_AC_POWER_DIVISOR:
            ctx->scale.ac_power_div = BUILD_UINT16(data[0], data[1]);
            break;
        default:
            ZB_LOGW(TAG, "0x%llx - %s() - Unhandled attr ID: 0x%04x", device->ieee_addr, __func__, attr_id);
            return false;
    }

    electrical_measurement_recompute(ctx);
    return reportable;
}

/******************************************************************************
 * Incoming message handlers
 ******************************************************************************/

static void
electrical_measurement_on_attr_report(s_zb_device_t *device, s_zb_function_t *func, s_zb_zcl_incoming_msg_t *msg)
{
    s_zb_device_electrical_measurement_ctx_t *ctx = (s_zb_device_electrical_measurement_ctx_t *)func->ctx;
    bool changed = false;

    if (msg->msg->cluster_id == ZCL_CLUSTER_ID_MS_ELECTRICAL_MEASUREMENT)
    {
        s_zb_zcl_report_attr_cmd_t *report_attr_cmd = (s_zb_zcl_report_attr_cmd_t *)msg->attr_cmd;
        for (uint8_t i = 0; i < report_attr_cmd->num_attr; i++)
        {
            s_zb_zcl_report_attr_info_t *report_attr_info = &report_attr_cmd->attr_list[i];
            changed |= electrical_measurement_apply_attr(device, func, ctx, report_attr_info->attr_id, report_attr_info->attr_data);
        }
    }
    else
    {
        ZB_LOGW(TAG, "0x%llx - %s() - Unsupported cluster ID: 0x%04x", device->ieee_addr, __func__, msg->msg->cluster_id);
    }
    if (changed)
    {
        electrical_measurement_notify_state(device, func, ctx);
    }
}

static void
electrical_measurement_on_read_rsp(s_zb_device_t *device, s_zb_function_t *func, s_zb_zcl_incoming_msg_t *msg)
{
    s_zb_device_electrical_measurement_ctx_t *ctx = (s_zb_device_electrical_measurement_ctx_t *)func->ctx;
    bool changed = false;

    if (msg->msg->cluster_id == ZCL_CLUSTER_ID_MS_ELECTRICAL_MEASUREMENT)
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
            changed |= electrical_measurement_apply_attr(device, func, ctx, read_attr_rsp_info->attr_id, read_attr_rsp_info->data);
        }
    }
    else
    {
        ZB_LOGW(TAG, "0x%llx - %s() - Unsupported cluster ID: 0x%04x", device->ieee_addr, __func__, msg->msg->cluster_id);
    }
    if (changed)
    {
        electrical_measurement_notify_state(device, func, ctx);
    }
}

/******************************************************************************
 * Command handler
 ******************************************************************************/

static zb_status_t
electrical_measurement_on_command(s_zb_device_t *device, s_zb_function_t *func, const s_zb_cmd_t *cmd)
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

            uint8_t buf[17] = {0};
            s_zb_zcl_read_attr_cmd_t *read_attr_cmd = (s_zb_zcl_read_attr_cmd_t *)&buf[0];

            /* Live values: RMS voltage / current / active+reactive+apparent power /
             * power factor / AC frequency */
            uint8_t seq_num = zb_zcl_next_seq_num();
            read_attr_cmd->num_attr = 7;
            read_attr_cmd->attr_id[0] = ATTRID_ELECTRICAL_MEASUREMENT_RMS_VOLTAGE;
            read_attr_cmd->attr_id[1] = ATTRID_ELECTRICAL_MEASUREMENT_RMS_CURRENT;
            read_attr_cmd->attr_id[2] = ATTRID_ELECTRICAL_MEASUREMENT_ACTIVE_POWER;
            read_attr_cmd->attr_id[3] = ATTRID_ELECTRICAL_MEASUREMENT_REACTIVE_POWER;
            read_attr_cmd->attr_id[4] = ATTRID_ELECTRICAL_MEASUREMENT_APPARENT_POWER;
            read_attr_cmd->attr_id[5] = ATTRID_ELECTRICAL_MEASUREMENT_POWER_FACTOR;
            read_attr_cmd->attr_id[6] = ATTRID_ELECTRICAL_MEASUREMENT_AC_FREQUENCY;
            status = zb_zcl_send_read(ZB_HUB_ENDPOINT, &addr, ZCL_CLUSTER_ID_MS_ELECTRICAL_MEASUREMENT, read_attr_cmd, ZCL_FRAME_CLIENT_SERVER_DIR, false, seq_num);
            ZB_LOGI(TAG, "0x%llx - %s() - Sent read (%d)", device->ieee_addr, __func__, status);

            /* Scaling attributes are static — fetch them only on the first sync
             * (or until learned). A quirk-seeded scale also satisfies this. */
            if (!func->synced)
            {
                seq_num = zb_zcl_next_seq_num();
                read_attr_cmd->num_attr = 8;
                read_attr_cmd->attr_id[0] = ATTRID_ELECTRICAL_MEASUREMENT_AC_VOLTAGE_MULTIPLIER;
                read_attr_cmd->attr_id[1] = ATTRID_ELECTRICAL_MEASUREMENT_AC_VOLTAGE_DIVISOR;
                read_attr_cmd->attr_id[2] = ATTRID_ELECTRICAL_MEASUREMENT_AC_CURRENT_MULTIPLIER;
                read_attr_cmd->attr_id[3] = ATTRID_ELECTRICAL_MEASUREMENT_AC_CURRENT_DIVISOR;
                read_attr_cmd->attr_id[4] = ATTRID_ELECTRICAL_MEASUREMENT_AC_POWER_MULTIPLIER;
                read_attr_cmd->attr_id[5] = ATTRID_ELECTRICAL_MEASUREMENT_AC_POWER_DIVISOR;
                read_attr_cmd->attr_id[6] = ATTRID_ELECTRICAL_MEASUREMENT_AC_FREQUENCY_MULTIPLIER;
                read_attr_cmd->attr_id[7] = ATTRID_ELECTRICAL_MEASUREMENT_AC_FREQUENCY_DIVISOR;
                status = zb_zcl_send_read(ZB_HUB_ENDPOINT, &addr, ZCL_CLUSTER_ID_MS_ELECTRICAL_MEASUREMENT, read_attr_cmd, ZCL_FRAME_CLIENT_SERVER_DIR, false, seq_num);
                ZB_LOGI(TAG, "0x%llx - %s() - Sent read scaling (%d)", device->ieee_addr, __func__, status);
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
electrical_measurement_emit_state(s_zb_device_t *device, s_zb_function_t *func)
{
    electrical_measurement_notify_state(device, func, (s_zb_device_electrical_measurement_ctx_t *)func->ctx);
}

s_zb_function_ops_t zb_device_electrical_measurement_ops = {
    .emit_state = electrical_measurement_emit_state,
    .ctx_size = sizeof(s_zb_device_electrical_measurement_ctx_t),
    .init = electrical_measurement_init,
    .destroy = electrical_measurement_destroy,
    .get_read_attrs = electrical_measurement_get_read_attrs,
    .on_attr_report = electrical_measurement_on_attr_report,
    .on_read_rsp = electrical_measurement_on_read_rsp,
    .on_command = electrical_measurement_on_command,
};
