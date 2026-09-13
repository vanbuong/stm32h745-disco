#ifndef DEVICE_ZB_DEVICE_ON_OFF_SMART_PLUG_H_
#define DEVICE_ZB_DEVICE_ON_OFF_SMART_PLUG_H_

#include "device/zb_device.h"

typedef struct s_zb_device_on_off_smart_plug_ctx {
    uint8_t on_off;
} s_zb_device_on_off_smart_plug_ctx_t;

extern s_zb_function_ops_t zb_device_on_off_smart_plug_ops;

#endif /* DEVICE_ZB_DEVICE_ON_OFF_SMART_PLUG_H_ */