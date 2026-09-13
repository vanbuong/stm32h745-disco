#ifndef DEVICE_ZB_DEVICE_DEVELCO_VOC_SENSOR_H_
#define DEVICE_ZB_DEVICE_DEVELCO_VOC_SENSOR_H_

#include "device/zb_device.h"
#include "manu/develco/zb_manu_develco.h"

/* Develco/frient manufacturer-specific VOC measurement
 * (ZCL_CLUSTER_ID_MS_DEVELCO_VOC, ZB_MANUFACTURER_CODE_DEVELCO - both from
 * manu/develco/). Used by the AQSZB-110 air-quality sensor, which
 * reports VOC here instead of on a standard measurement cluster. MeasuredValue
 * is a uint16 in ppb. Reported through the ZB_EVENT_SENSOR_TVOC event. */
typedef struct s_zb_device_develco_voc_sensor_ctx
{
    uint16_t raw_value; /* last MeasuredValue (uint16), for change detection */
    float    value;     /* VOC concentration in ppb */
} s_zb_device_develco_voc_sensor_ctx_t;

extern s_zb_function_ops_t zb_device_develco_voc_sensor_ops;

#endif /* DEVICE_ZB_DEVICE_DEVELCO_VOC_SENSOR_H_ */
