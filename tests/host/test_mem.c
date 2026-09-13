#include "unity.h"

#include "bsp/board.h"
#include "bsp/mpu_map.h"
#include "svc/memtest.h"

#include <stdint.h>
#include <string.h>

static void test_walking(void)
{
    uint32_t buf[64];
    uint32_t off = 0xFFFFFFFFu;

    memset(buf, 0, sizeof(buf));
    TEST_ASSERT_EQUAL_INT(ERR_INVAL, memtest_walking(NULL, 64, &off));
    TEST_ASSERT_EQUAL_INT(ERR_INVAL, memtest_walking(buf, 0, &off));
    TEST_ASSERT_EQUAL_INT(ERR_OK, memtest_walking(buf, 64, NULL));
    TEST_ASSERT_EQUAL_INT(ERR_OK, memtest_walking(buf, 64, &off));
    TEST_ASSERT_EQUAL_UINT32(0u, off);
    TEST_ASSERT_EQUAL_HEX32(~0u, buf[0]);
    TEST_ASSERT_EQUAL_HEX32(~63u, buf[63]);
}

static void test_mpu_map(void)
{
    unsigned i;

    TEST_ASSERT_EQUAL_UINT32(32u, mpu_size_bytes(4));
    TEST_ASSERT_EQUAL_UINT32(65536u, mpu_size_bytes(15));
    TEST_ASSERT_EQUAL_UINT32(8u * 1024u * 1024u, mpu_size_bytes(22));
    TEST_ASSERT_EQUAL_UINT32(16u * 1024u * 1024u, mpu_size_bytes(MPU_ENC_16M));
    TEST_ASSERT_EQUAL_UINT32(64u * 1024u * 1024u, mpu_size_bytes(25));
    TEST_ASSERT_EQUAL_UINT32(0u, mpu_size_bytes(31));
    TEST_ASSERT_EQUAL_UINT32(0u, mpu_size_bytes(200));
    TEST_ASSERT_EQUAL_INT(1, mpu_base_aligned(0x38000000u, MPU_ENC_64K));
    TEST_ASSERT_EQUAL_INT(0, mpu_base_aligned(0x38000001u, MPU_ENC_64K));
    TEST_ASSERT_EQUAL_INT(1, mpu_base_aligned(0xD0000000u, MPU_ENC_8M));
    TEST_ASSERT_EQUAL_INT(1, mpu_base_aligned(0xD0000000u, MPU_ENC_16M));
    TEST_ASSERT_EQUAL_INT(0, mpu_base_aligned(0u, 31));

    TEST_ASSERT_GREATER_OR_EQUAL_UINT(4u, g_mpu_map_n);
    for (i = 0; i < g_mpu_map_n; i++) {
        TEST_ASSERT_NOT_NULL(g_mpu_map[i].name);
        TEST_ASSERT_EQUAL_INT(1, mpu_base_aligned(g_mpu_map[i].base, g_mpu_map[i].size_enc));
    }
    TEST_ASSERT_EQUAL_HEX32(0x38000000u, g_mpu_map[0].base);
    TEST_ASSERT_EQUAL_UINT8(MPU_ATTR_NORMAL_NC, g_mpu_map[0].attr);
    TEST_ASSERT_EQUAL_UINT8(0, g_mpu_map[0].exec);
    TEST_ASSERT_EQUAL_HEX32(0xD0000000u, g_mpu_map[1].base);
    TEST_ASSERT_EQUAL_UINT8(MPU_ENC_16M, g_mpu_map[1].size_enc);
    TEST_ASSERT_EQUAL_UINT32(BOARD_SDRAM_BYTES, mpu_size_bytes(g_mpu_map[1].size_enc));
    TEST_ASSERT_EQUAL_UINT32(16u * 1024u * 1024u, BOARD_SDRAM_BYTES);
    TEST_ASSERT_EQUAL_UINT8(MPU_ATTR_WT, g_mpu_map[1].attr);
    TEST_ASSERT_EQUAL_UINT8(1, g_mpu_map[1].exec);
    TEST_ASSERT_EQUAL_HEX32(0x90000000u, g_mpu_map[2].base);
    TEST_ASSERT_EQUAL_HEX32(0x24000000u, g_mpu_map[3].base);
    TEST_ASSERT_GREATER_OR_EQUAL_UINT(5u, g_mpu_map_n);
    TEST_ASSERT_EQUAL_HEX32(0x30040000u, g_mpu_map[4].base);
    TEST_ASSERT_EQUAL_UINT8(MPU_ATTR_NORMAL_NC, g_mpu_map[4].attr);
    TEST_ASSERT_EQUAL_UINT8(0, g_mpu_map[4].exec);
    TEST_ASSERT_EQUAL_UINT32(32768u, mpu_size_bytes(MPU_ENC_32K));
}

void test_mem_run(void)
{
    UnitySetTestFile(__FILE__);
    RUN_TEST(test_walking);
    RUN_TEST(test_mpu_map);
}
