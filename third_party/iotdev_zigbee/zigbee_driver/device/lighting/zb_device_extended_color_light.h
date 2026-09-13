#ifndef DEVICE_ZB_DEVICE_EXTENDED_COLOR_LIGHT_H_
#define DEVICE_ZB_DEVICE_EXTENDED_COLOR_LIGHT_H_

#include "device/zb_device.h"

typedef struct s_zb_device_extended_color_light_ctx
{
    uint8_t hue;
    uint8_t saturation;
    uint16_t color_temp;
} s_zb_device_extended_color_light_ctx_t;

extern s_zb_function_ops_t zb_device_extended_color_light_ops;

#endif /* DEVICE_ZB_DEVICE_EXTENDED_COLOR_LIGHT_H_ */