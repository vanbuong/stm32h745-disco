#ifndef DEVICE_ZB_DEVICE_COLOR_TEMP_LIGHT_H_
#define DEVICE_ZB_DEVICE_COLOR_TEMP_LIGHT_H_

#include "device/zb_device.h"

typedef struct s_zb_device_color_temp_light_ctx
{
    uint16_t color_temp;
} s_zb_device_color_temp_light_ctx_t;

extern s_zb_function_ops_t zb_device_color_temp_light_ops;

#endif /* DEVICE_ZB_DEVICE_COLOR_TEMP_LIGHT_H_ */