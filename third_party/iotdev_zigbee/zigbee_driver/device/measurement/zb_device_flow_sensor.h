#ifndef DEVICE_ZB_DEVICE_FLOW_SENSOR_H_
#define DEVICE_ZB_DEVICE_FLOW_SENSOR_H_

#include "device/zb_device.h"

typedef struct s_zb_device_flow_sensor_ctx
{
    uint16_t raw_value;
    float value;
} s_zb_device_flow_sensor_ctx_t;

extern s_zb_function_ops_t zb_device_flow_sensor_ops;

#endif /* DEVICE_ZB_DEVICE_FLOW_SENSOR_H_ */