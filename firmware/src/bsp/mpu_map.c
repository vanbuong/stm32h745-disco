#include "bsp/mpu_map.h"

const mpu_region_desc_t g_mpu_map[] = {
    {"sram4", 0x38000000u, MPU_ENC_64K, MPU_ATTR_NORMAL_NC, 0},
    {"sdram", 0xD0000000u, MPU_ENC_16M, MPU_ATTR_WT, 1},
    {"qspi", 0x90000000u, MPU_ENC_64M, MPU_ATTR_WT, 1},
    {"axi", 0x24000000u, MPU_ENC_512K, MPU_ATTR_WB, 1},
    {"sram3", 0x30040000u, MPU_ENC_32K, MPU_ATTR_NORMAL_NC, 0},
};

const unsigned g_mpu_map_n = sizeof(g_mpu_map) / sizeof(g_mpu_map[0]);

uint32_t mpu_size_bytes(uint8_t size_enc)
{
    /* 4 GB (size_enc 31) does not fit in uint32_t. */
    if (size_enc > 30u) {
        return 0;
    }
    return 1u << (size_enc + 1u);
}

int mpu_base_aligned(uint32_t base, uint8_t size_enc)
{
    uint32_t sz = mpu_size_bytes(size_enc);
    if (sz == 0u) {
        return 0;
    }
    return (base & (sz - 1u)) == 0u;
}
