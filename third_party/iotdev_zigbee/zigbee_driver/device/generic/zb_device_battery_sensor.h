#ifndef DEVICE_ZB_DEVICE_BATTERY_SENSOR_H_
#define DEVICE_ZB_DEVICE_BATTERY_SENSOR_H_

#include "device/zb_device.h"

typedef struct s_zb_device_battery_sensor_ctx
{
    uint8_t percentage;
    uint8_t voltage;
} s_zb_device_battery_sensor_ctx_t;

extern s_zb_function_ops_t zb_device_battery_sensor_ops;

#endif /* DEVICE_ZB_DEVICE_BATTERY_SENSOR_H_ */