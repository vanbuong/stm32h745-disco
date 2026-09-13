/*
 * zb_common.h
 * 
 * Author: Vo Van Buong (BRT-SG)
 */

#ifndef ZB_COMMON_H_
#define ZB_COMMON_H_

#ifdef __cplusplus
extern "C"
{
#endif /* __cplusplus */

#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>
#include <string.h>
#include <stdlib.h>
#include <math.h>

#define HI_UINT16(a)                                (((a) >> 8) & 0xFF)
#define LO_UINT16(a)                                ((a) & 0xFF)

#define BREAK_UINT32(value, index)                  (uint8_t)((uint32_t)(((value)>>((index) * 8)) & 0x00FF))

#define BUILD_UINT16(lo_byte, hi_byte)              ((uint16_t)(((lo_byte) & 0x00FF) + (((hi_byte) & 0x00FF) << 8)))

#define BUILD_UINT32(byte0, byte1, byte2, byte3)    ((uint32_t)((uint32_t)((byte0) & 0x00FF) + \
                                                    ((uint32_t)((byte1) & 0x00FF) << 8) + \
                                                    ((uint32_t)((byte2) & 0x00FF) << 16) + \
                                                    ((uint32_t)((byte3) & 0x00FF) << 24)))

#define BUILD_UINT32_TO_BUFFER(buffer, value)       do { \
                                                        *buffer++ = (uint8_t)BREAK_UINT32(value, 0); \
                                                        *buffer++ = (uint8_t)BREAK_UINT32(value, 1); \
                                                        *buffer++ = (uint8_t)BREAK_UINT32(value, 2); \
                                                        *buffer++ = (uint8_t)BREAK_UINT32(value, 3); \
                                                    } while (0)

#define ZB_OK                                       0
#define ZB_FAIL                                     -1

#define ZB_UNUSED(x)                                (void)x

//------------------------------------------------------------------------------
// Security modes
//------------------------------------------------------------------------------
#define ZG_SECURE_ENABLED                           1

typedef int zb_status_t;

#if defined(ZB_PLATFORM_IOTDEV)
#include "common/priv/zb_common_iotdev.h"
#else
#include "common/priv/zb_common_unknown.h"
#endif

#include "common/priv/zb_common_error_codes.h"
#include "common/zb_common_types.h"

int
zb_mkdir(const char *dir);

int
zb_file_read(const char *filename, uint8_t *data, uint16_t data_len, uint32_t seek_offset);

int
zb_file_write(const char *filename, const uint8_t *data, uint16_t data_len);

uint64_t
zb_get_utc_epoch_time(void);

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* ZB_COMMON_H_ */