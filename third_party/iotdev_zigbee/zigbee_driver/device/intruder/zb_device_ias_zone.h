#ifndef DEVICE_ZB_DEVICE_IAS_ZONE_H_
#define DEVICE_ZB_DEVICE_IAS_ZONE_H_

#include "device/zb_device.h"

typedef struct s_zb_device_ias_zone_ctx
{
    uint16_t zone_type;
    uint16_t zone_status;
} s_zb_device_ias_zone_ctx_t;

extern s_zb_function_ops_t zb_device_ias_zone_ops;

#endif /* DEVICE_ZB_DEVICE_IAS_ZONE_H_ */