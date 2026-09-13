#ifndef DEVICE_ZB_DEVICE_PM25_SENSOR_H_
#define DEVICE_ZB_DEVICE_PM25_SENSOR_H_

#include "device/zb_device.h"

typedef struct s_zb_device_pm25_sensor_ctx
{
    uint16_t raw_value; /* last MeasuredValue (uint16), for change detection */
    float    value;     /* PM2.5 concentration in ug/m3 */
} s_zb_device_pm25_sensor_ctx_t;

extern s_zb_function_ops_t zb_device_pm25_sensor_ops;

#endif /* DEVICE_ZB_DEVICE_PM25_SENSOR_H_ */