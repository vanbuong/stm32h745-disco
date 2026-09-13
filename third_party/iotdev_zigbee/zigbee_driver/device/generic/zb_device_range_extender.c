#include "device/generic/zb_device_range_extender.h"

#include "common/zb_common.h"
#include "af/zb_af.h"
#include "core/zb_core.h"
#include "device/zb_device_manager.h"
#include "zcl/zb_zcl.h"
#include "zcl/zb_zcl_general.h"

#define TAG "ZB_DEV"

static void
range_extender_init(s_zb_device_t *device, s_zb_function_t *func)
{
    ZB_LOGI(TAG, "0x%llx - %s()", device->ieee_addr, __func__);
}

static void
range_extender_destroy(s_zb_device_t *device, s_zb_function_t *func)
{
    ZB_LOGI(TAG, "0x%llx - %s()", device->ieee_addr, __func__);
}

static const s_zb_attr_read_t*
range_extender_get_read_attrs(s_zb_device_t *device, s_zb_function_t *func)
{
    ZB_LOGI(TAG, "0x%llx - %s()", device->ieee_addr, __func__);
    return NULL;
}

static void
range_extender_notify_state(s_zb_device_t *device, s_zb_function_t *func, s_zb_device_range_extender_ctx_t *ctx)
{
    ZB_LOGD(TAG, "0x%llx - %s()", device->ieee_addr, __func__);
    s_zb_event_t event;
    event.type = ZB_EVENT_DEVICE_UPDATED;
    zb_device_manager_notify_event(device, func, &event);
}

static void
range_extender_on_read_rsp(s_zb_device_t *device, s_zb_function_t *func, s_zb_zcl_incoming_msg_t *msg)
{
    ZB_LOGI(TAG, "0x%llx - %s()", device->ieee_addr, __func__);
}

static void
range_extender_on_attr_report(s_zb_device_t *device, s_zb_function_t *func, s_zb_zcl_incoming_msg_t *msg)
{
    ZB_LOGI(TAG, "0x%llx - %s()", device->ieee_addr, __func__);
}

static zb_status_t
range_extender_on_command(s_zb_device_t *device, s_zb_function_t *func, const s_zb_cmd_t *cmd)
{
    ZB_LOGI(TAG, "0x%llx - %s()", device->ieee_addr, __func__);
    zb_status_t status = ZB_OK;

    return status;
}

s_zb_function_ops_t zb_device_range_extender_ops = {
    .ctx_size = sizeof(s_zb_device_range_extender_ctx_t),
    .init = range_extender_init,
    .destroy = range_extender_destroy,
    .get_read_attrs = range_extender_get_read_attrs,
    .on_read_rsp = range_extender_on_read_rsp,
    .on_attr_report = range_extender_on_attr_report,
    .on_command = range_extender_on_command,
};
