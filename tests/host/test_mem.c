#include "unity.h"

#include "bsp/mpu_map.h"
#include "svc/memtest.h"

#include <stdint.h>
#include <string.h>

static void test_walking(void)
{
    uint32_t buf[64];
    uint32_t off = 0xFFFFFFFFu;

    memset(buf, 0, sizeof(buf));
    TEST_ASSERT_TRUE(memtest_walking(NULL, 64, &off) == ERR_INVAL);
    TEST_ASSERT_TRUE(memtest_walking(buf, 0, &off) == ERR_INVAL);
    TEST_ASSERT_TRUE(memtest_walking(buf, 64, NULL) == ERR_OK);
    TEST_ASSERT_TRUE(memtest_walking(buf, 64, &off) == ERR_OK);
    TEST_ASSERT_TRUE(off == 0);
    TEST_ASSERT_TRUE(buf[0] == ~0u);
    TEST_ASSERT_TRUE(buf[63] == ~63u);
}

static void test_mpu_map(void)
{
    unsigned i;

    TEST_ASSERT_TRUE(mpu_size_bytes(4) == 32u);
    TEST_ASSERT_TRUE(mpu_size_bytes(15) == 65536u);
    TEST_ASSERT_TRUE(mpu_size_bytes(22) == (8u * 1024u * 1024u));
    TEST_ASSERT_TRUE(mpu_size_bytes(25) == (64u * 1024u * 1024u));
    TEST_ASSERT_TRUE(mpu_size_bytes(31) == 0u);
    TEST_ASSERT_TRUE(mpu_size_bytes(200) == 0u);
    TEST_ASSERT_TRUE(mpu_base_aligned(0x38000000u, MPU_ENC_64K) == 1);
    TEST_ASSERT_TRUE(mpu_base_aligned(0x38000001u, MPU_ENC_64K) == 0);
    TEST_ASSERT_TRUE(mpu_base_aligned(0xD0000000u, MPU_ENC_8M) == 1);
    TEST_ASSERT_TRUE(mpu_base_aligned(0u, 31) == 0);

    TEST_ASSERT_TRUE(g_mpu_map_n >= 4u);
    for (i = 0; i < g_mpu_map_n; i++) {
        TEST_ASSERT_TRUE(g_mpu_map[i].name != NULL);
        TEST_ASSERT_TRUE(mpu_base_aligned(g_mpu_map[i].base, g_mpu_map[i].size_enc) == 1);
    }
    TEST_ASSERT_TRUE(g_mpu_map[0].base == 0x38000000u);
    TEST_ASSERT_TRUE(g_mpu_map[0].attr == MPU_ATTR_NORMAL_NC);
    TEST_ASSERT_TRUE(g_mpu_map[0].exec == 0);
    TEST_ASSERT_TRUE(g_mpu_map[1].base == 0xD0000000u);
    TEST_ASSERT_TRUE(g_mpu_map[1].attr == MPU_ATTR_WT);
    TEST_ASSERT_TRUE(g_mpu_map[1].exec == 1);
    TEST_ASSERT_TRUE(g_mpu_map[2].base == 0x90000000u);
    TEST_ASSERT_TRUE(g_mpu_map[3].base == 0x24000000u);
}

void test_mem_run(void)
{
    UnitySetTestFile(__FILE__);
    RUN_TEST(test_walking);
    RUN_TEST(test_mpu_map);
}
