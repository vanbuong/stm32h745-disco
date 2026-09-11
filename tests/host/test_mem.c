#include "test.h"

#include "bsp/mpu_map.h"
#include "svc/memtest.h"

#include <stdint.h>
#include <string.h>

static void test_walking(void)
{
    uint32_t buf[64];
    uint32_t off = 0xFFFFFFFFu;

    memset(buf, 0, sizeof(buf));
    CHECK(memtest_walking(NULL, 64, &off) == ERR_INVAL);
    CHECK(memtest_walking(buf, 0, &off) == ERR_INVAL);
    CHECK(memtest_walking(buf, 64, NULL) == ERR_OK);
    CHECK(memtest_walking(buf, 64, &off) == ERR_OK);
    CHECK(off == 0);
    CHECK(buf[0] == ~0u);
    CHECK(buf[63] == ~63u);
}

static void test_mpu_map(void)
{
    unsigned i;

    CHECK(mpu_size_bytes(4) == 32u);
    CHECK(mpu_size_bytes(15) == 65536u);
    CHECK(mpu_size_bytes(22) == (8u * 1024u * 1024u));
    CHECK(mpu_size_bytes(25) == (64u * 1024u * 1024u));
    CHECK(mpu_size_bytes(31) == 0u);
    CHECK(mpu_size_bytes(200) == 0u);
    CHECK(mpu_base_aligned(0x38000000u, MPU_ENC_64K) == 1);
    CHECK(mpu_base_aligned(0x38000001u, MPU_ENC_64K) == 0);
    CHECK(mpu_base_aligned(0xD0000000u, MPU_ENC_8M) == 1);
    CHECK(mpu_base_aligned(0u, 31) == 0);

    CHECK(g_mpu_map_n >= 4u);
    for (i = 0; i < g_mpu_map_n; i++) {
        CHECK(g_mpu_map[i].name != NULL);
        CHECK(mpu_base_aligned(g_mpu_map[i].base, g_mpu_map[i].size_enc) == 1);
    }
    CHECK(g_mpu_map[0].base == 0x38000000u);
    CHECK(g_mpu_map[0].attr == MPU_ATTR_NORMAL_NC);
    CHECK(g_mpu_map[0].exec == 0);
    CHECK(g_mpu_map[1].base == 0xD0000000u);
    CHECK(g_mpu_map[1].attr == MPU_ATTR_WT);
    CHECK(g_mpu_map[1].exec == 1);
    CHECK(g_mpu_map[2].base == 0x90000000u);
    CHECK(g_mpu_map[3].base == 0x24000000u);
}

void test_mem_run(void)
{
    test_walking();
    test_mpu_map();
}
