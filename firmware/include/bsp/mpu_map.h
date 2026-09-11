#ifndef MPU_MAP_H
#define MPU_MAP_H

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/* RASR.SIZE: region bytes = 1 << (size_enc + 1) */
#define MPU_ENC_32B 4u
#define MPU_ENC_32K 14u
#define MPU_ENC_64K 15u
#define MPU_ENC_512K 18u
#define MPU_ENC_2M 20u
#define MPU_ENC_8M 22u
#define MPU_ENC_64M 25u
#define MPU_ENC_4G 31u

#define MPU_ATTR_DEVICE 0u
#define MPU_ATTR_NORMAL_NC 1u
#define MPU_ATTR_WT 2u
#define MPU_ATTR_WB 3u

typedef struct {
    const char *name;
    uint32_t base;
    uint8_t size_enc;
    uint8_t attr;
    uint8_t exec;
} mpu_region_desc_t;

extern const mpu_region_desc_t g_mpu_map[];
extern const unsigned g_mpu_map_n;

uint32_t mpu_size_bytes(uint8_t size_enc);
int mpu_base_aligned(uint32_t base, uint8_t size_enc);

#ifdef __cplusplus
}
#endif

#endif /* MPU_MAP_H */
