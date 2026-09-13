#ifndef DEVICE_ZB_DEVICE_MAINS_POWER_OUTLET_H_
#define DEVICE_ZB_DEVICE_MAINS_POWER_OUTLET_H_

#include "device/zb_device.h"

typedef struct s_zb_device_mains_power_outlet_ctx
{
    bool on_off;
} s_zb_device_mains_power_outlet_ctx_t;

extern s_zb_function_ops_t zb_device_mains_power_outlet_ops;

#endif /* DEVICE_ZB_DEVICE_MAINS_POWER_OUTLET_H_ */