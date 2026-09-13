#ifndef DEVICE_ZB_DEVICE_RANGE_EXTENDER_H_
#define DEVICE_ZB_DEVICE_RANGE_EXTENDER_H_

#include "device/zb_device.h"

typedef struct s_zb_device_range_extender_ctx
{
    bool reachable;
} s_zb_device_range_extender_ctx_t;

extern s_zb_function_ops_t zb_device_range_extender_ops;

#endif /* DEVICE_ZB_DEVICE_RANGE_EXTENDER_H_ */