#ifndef DEVICE_ZB_DEVICE_DIMMABLE_LIGHT_H_
#define DEVICE_ZB_DEVICE_DIMMABLE_LIGHT_H_

#include "device/zb_device.h"

typedef struct s_zb_device_dimmable_light_ctx
{
    uint8_t level;
} s_zb_device_dimmable_light_ctx_t;

extern s_zb_function_ops_t zb_device_dimmable_light_ops;


#endif /* DEVICE_ZB_DEVICE_DIMMABLE_LIGHT_H_ */