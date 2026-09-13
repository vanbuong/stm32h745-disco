#ifndef DEVICE_ZB_DEVICE_ECO2_SENSOR_H_
#define DEVICE_ZB_DEVICE_ECO2_SENSOR_H_

#include "device/zb_device.h"

typedef struct s_zb_device_eco2_sensor_ctx
{
    uint16_t raw_value; /* last MeasuredValue (uint16), for change detection */
    float    value;     /* equivalent CO2 in ppm */
} s_zb_device_eco2_sensor_ctx_t;

extern s_zb_function_ops_t zb_device_eco2_sensor_ops;

#endif /* DEVICE_ZB_DEVICE_ECO2_SENSOR_H_ */
