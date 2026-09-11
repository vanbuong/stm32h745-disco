#include "unity.h"

#include "svc/znp_mt.h"

#include <string.h>

static void test_roundtrip(void)
{
    uint8_t pl[3] = {1, 2, 3};
    uint8_t frame[32];
    uint8_t outp[8];
    uint8_t cmd0;
    uint8_t cmd1;
    uint8_t len;
    size_t n;

    TEST_ASSERT_TRUE(znp_mt_encode(0x21, 0x02, pl, 3, frame, sizeof(frame), &n) == ERR_OK);
    TEST_ASSERT_TRUE(n == 8);
    TEST_ASSERT_TRUE(frame[0] == ZNP_SOF);
    TEST_ASSERT_TRUE(znp_mt_decode(frame, n, &cmd0, &cmd1, outp, sizeof(outp), &len) == ERR_OK);
    TEST_ASSERT_TRUE(cmd0 == 0x21);
    TEST_ASSERT_TRUE(cmd1 == 0x02);
    TEST_ASSERT_TRUE(len == 3);
    TEST_ASSERT_TRUE(memcmp(outp, pl, 3) == 0);
}

static void test_bad(void)
{
    uint8_t frame[16];
    uint8_t cmd0;
    uint8_t cmd1;
    uint8_t len;
    uint8_t outp[8];
    size_t n;

    TEST_ASSERT_TRUE(znp_mt_encode(0x01, 0x00, NULL, 0, frame, sizeof(frame), &n) == ERR_OK);
    frame[n - 1u] ^= 0xFFu;
    TEST_ASSERT_TRUE(znp_mt_decode(frame, n, &cmd0, &cmd1, outp, sizeof(outp), &len) ==
                     ERR_CORRUPT);
    frame[0] = 0x00;
    TEST_ASSERT_TRUE(znp_mt_decode(frame, n, &cmd0, &cmd1, outp, sizeof(outp), &len) ==
                     ERR_CORRUPT);
    TEST_ASSERT_TRUE(znp_mt_encode(0x01, 0x00, NULL, 1, frame, sizeof(frame), &n) == ERR_INVAL);
    TEST_ASSERT_TRUE(znp_mt_encode(0x01, 0x00, NULL, 0, frame, 3, &n) == ERR_NOSPC);
    TEST_ASSERT_TRUE(znp_mt_decode(NULL, 8, &cmd0, &cmd1, outp, sizeof(outp), &len) == ERR_INVAL);
}

void test_znp_run(void)
{
    UnitySetTestFile(__FILE__);
    RUN_TEST(test_roundtrip);
    RUN_TEST(test_bad);
}
