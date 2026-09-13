#ifndef DEVICE_ZB_DEVICE_FORMALDEHYDE_SENSOR_H_
#define DEVICE_ZB_DEVICE_FORMALDEHYDE_SENSOR_H_

#include "device/zb_device.h"

typedef struct s_zb_device_formaldehyde_sensor_ctx
{
    uint16_t raw_value; /* last MeasuredValue (uint16), for change detection */
    float    value;     /* Formaldehyde (CH2O) concentration in ppb */
} s_zb_device_formaldehyde_sensor_ctx_t;

extern s_zb_function_ops_t zb_device_formaldehyde_sensor_ops;

#endif /* DEVICE_ZB_DEVICE_FORMALDEHYDE_SENSOR_H_ */
