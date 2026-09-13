#ifndef DEVICE_ZB_DEVICE_IAS_WARNING_H_
#define DEVICE_ZB_DEVICE_IAS_WARNING_H_

#include "device/zb_device.h"

typedef struct s_zb_device_ias_warning_ctx
{
    uint16_t max_duration;
} s_zb_device_ias_warning_ctx_t;

extern s_zb_function_ops_t zb_device_ias_warning_ops;

#endif /* DEVICE_ZB_DEVICE_IAS_WARNING_H_ */