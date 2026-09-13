#include "device/generic/zb_device_button.h"

#include "common/zb_common.h"
#include "af/zb_af.h"
#include "device/zb_device_manager.h"
#include "zcl/zb_zcl.h"
#include "zcl/zb_zcl_general.h"

#define TAG "ZB_DEV"

static void
button_init(s_zb_device_t *device, s_zb_function_t *func)
{
    ZB_LOGI(TAG, "0x%llx - %s()", device->ieee_addr, __func__);
    s_zb_device_button_ctx_t *ctx = (s_zb_device_button_ctx_t *)func->ctx;
    ctx->last_action = ZB_BUTTON_SINGLE;
    ctx->has_action  = false;
}

static const s_zb_attr_read_t*
button_get_read_attrs(s_zb_device_t *device, s_zb_function_t *func)
{
    /* Momentary: nothing to poll - the action arrives as a PresentValue report. */
    return NULL;
}

static void
button_notify_action(s_zb_device_t *device, s_zb_function_t *func, e_zb_button_action_t action)
{
    s_zb_event_t event = {0};
    event.type          = ZB_EVENT_BUTTON_ACTION;
    event.button.action = (uint8_t)action;
    zb_device_manager_notify_event(device, func, &event);
}

static void
button_on_attr_report(s_zb_device_t *device, s_zb_function_t *func, s_zb_zcl_incoming_msg_t *msg)
{
    ZB_LOGI(TAG, "0x%llx - %s()", device->ieee_addr, __func__);

    if (msg->msg->cluster_id != ZCL_CLUSTER_ID_GENERAL_MULTISTATE_INPUT_BASIC)
    {
        ZB_LOGW(TAG, "0x%llx - %s() - Unsupported cluster ID: 0x%04x",
                device->ieee_addr, __func__, msg->msg->cluster_id);
        return;
    }

    s_zb_device_button_ctx_t *ctx = (s_zb_device_button_ctx_t *)func->ctx;
    s_zb_zcl_report_attr_cmd_t *report_attr_cmd = (s_zb_zcl_report_attr_cmd_t *)msg->attr_cmd;

    for (uint8_t i = 0; i < report_attr_cmd->num_attr; i++)
    {
        s_zb_zcl_report_attr_info_t *report_attr_info = &report_attr_cmd->attr_list[i];
        if (report_attr_info->attr_id != ATTRID_IOV_BASIC_PRESENT_VALUE)
        {
            continue;
        }

        /* MultistateInput PresentValue -> action. Mapping for Lumi/Aqara
         * lumi.remote.b1acn01: 1 = single, 2 = double, 0 = long press
         * (255 = long-press release, ignored). */
        uint8_t pv = report_attr_info->attr_data[0];
        e_zb_button_action_t action;
        switch (pv)
        {
            case 1:  action = ZB_BUTTON_SINGLE; break;
            case 2:  action = ZB_BUTTON_DOUBLE; break;
            case 0:  action = ZB_BUTTON_LONG;   break;
            default: continue; /* release / unknown value: emit nothing */
        }

        ctx->last_action = (uint8_t)action;
        ctx->has_action  = true;
        /* Momentary: one event per press, even if the same action repeats. */
        button_notify_action(device, func, action);
    }
}

static void
button_emit_state(s_zb_device_t *device, s_zb_function_t *func)
{
    s_zb_device_button_ctx_t *ctx = (s_zb_device_button_ctx_t *)func->ctx;
    if (!ctx->has_action)
    {
        return; /* nothing pressed yet - no state to report */
    }
    button_notify_action(device, func, (e_zb_button_action_t)ctx->last_action);
}

s_zb_function_ops_t zb_device_button_ops = {
    .ctx_size       = sizeof(s_zb_device_button_ctx_t),
    .init           = button_init,
    .get_read_attrs = button_get_read_attrs,
    .on_attr_report = button_on_attr_report,
    .emit_state     = button_emit_state,
};
