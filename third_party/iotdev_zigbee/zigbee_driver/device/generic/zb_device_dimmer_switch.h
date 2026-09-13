#ifndef DEVICE_ZB_DEVICE_DIMMER_SWITCH_H_
#define DEVICE_ZB_DEVICE_DIMMER_SWITCH_H_

#include "device/zb_device.h"

typedef struct s_zb_device_dimmer_switch_ctx {
    uint8_t level;
} s_zb_device_dimmer_switch_ctx_t;

extern s_zb_function_ops_t zb_device_dimmer_switch_ops;

#endif /* DEVICE_ZB_DEVICE_DIMMER_SWITCH_H_ */