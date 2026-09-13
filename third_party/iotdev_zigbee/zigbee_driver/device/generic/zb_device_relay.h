#ifndef DEVICE_ZB_DEVICE_RELAY_H_
#define DEVICE_ZB_DEVICE_RELAY_H_

#include "device/zb_device.h"

typedef struct s_zb_device_relay_ctx {
    uint8_t on_off;
} s_zb_device_relay_ctx_t;

extern s_zb_function_ops_t zb_device_relay_ops;

#endif /* DEVICE_ZB_DEVICE_RELAY_H_ */
