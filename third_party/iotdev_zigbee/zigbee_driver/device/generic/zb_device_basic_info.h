#ifndef DEVICE_ZB_DEVICE_BASIC_INFO_H_
#define DEVICE_ZB_DEVICE_BASIC_INFO_H_

#include "device/zb_device.h"

typedef struct s_zb_device_basic_info_ctx
{
    char manufacturer[32];
    char model[32];
    char date_code[16];         /* Basic DateCode (0x0006) — build/date code */
    uint8_t zcl_version;
    uint8_t app_version;
    uint8_t hw_version;         /* Basic HWVersion (0x0003) */
    uint8_t power_source;
    uint8_t physical_environment; /* Basic PhysicalEnvironment (0x0011) */
} s_zb_device_basic_info_ctx_t;

extern s_zb_function_ops_t zb_device_basic_info_ops;

#endif /* DEVICE_ZB_DEVICE_BASIC_INFO_H_ */