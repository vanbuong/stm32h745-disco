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

    TEST_ASSERT_EQUAL_INT(ERR_OK, znp_mt_encode(0x21, 0x02, pl, 3, frame, sizeof(frame), &n));
    TEST_ASSERT_EQUAL_UINT(8u, n);
    TEST_ASSERT_EQUAL_HEX8(ZNP_SOF, frame[0]);
    TEST_ASSERT_EQUAL_INT(ERR_OK, znp_mt_decode(frame, n, &cmd0, &cmd1, outp, sizeof(outp), &len));
    TEST_ASSERT_EQUAL_HEX8(0x21, cmd0);
    TEST_ASSERT_EQUAL_HEX8(0x02, cmd1);
    TEST_ASSERT_EQUAL_UINT8(3, len);
    TEST_ASSERT_EQUAL_HEX8_ARRAY(pl, outp, 3);
}

static void test_bad(void)
{
    uint8_t frame[16];
    uint8_t cmd0;
    uint8_t cmd1;
    uint8_t len;
    uint8_t outp[8];
    size_t n;

    TEST_ASSERT_EQUAL_INT(ERR_OK, znp_mt_encode(0x01, 0x00, NULL, 0, frame, sizeof(frame), &n));
    frame[n - 1u] ^= 0xFFu;
    TEST_ASSERT_EQUAL_INT(ERR_CORRUPT,
                          znp_mt_decode(frame, n, &cmd0, &cmd1, outp, sizeof(outp), &len));
    frame[0] = 0x00;
    TEST_ASSERT_EQUAL_INT(ERR_CORRUPT,
                          znp_mt_decode(frame, n, &cmd0, &cmd1, outp, sizeof(outp), &len));
    TEST_ASSERT_EQUAL_INT(ERR_INVAL, znp_mt_encode(0x01, 0x00, NULL, 1, frame, sizeof(frame), &n));
    TEST_ASSERT_EQUAL_INT(ERR_NOSPC, znp_mt_encode(0x01, 0x00, NULL, 0, frame, 3, &n));
    TEST_ASSERT_EQUAL_INT(ERR_INVAL,
                          znp_mt_decode(NULL, 8, &cmd0, &cmd1, outp, sizeof(outp), &len));
}

void test_znp_run(void)
{
    UnitySetTestFile(__FILE__);
    RUN_TEST(test_roundtrip);
    RUN_TEST(test_bad);
}
