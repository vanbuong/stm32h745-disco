#include "unity.h"

#include "hal/uart.h"
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

static void test_sys_ping_stub(void)
{
    uart_cfg_t cfg;
    uint8_t tx[16];
    uint8_t rx[16];
    uint8_t pl[8];
    uint8_t cmd0 = 0u;
    uint8_t cmd1 = 0u;
    uint8_t plen = 0u;
    size_t n = 0u;
    size_t got = 0u;

    memset(&cfg, 0, sizeof(cfg));
    cfg.baud = 115200u;
    cfg.data_bits = 8u;
    cfg.stop_bits = 1u;
    TEST_ASSERT_EQUAL_INT(ERR_OK, uart_open(UART_ID_ZNP, &cfg));
    TEST_ASSERT_EQUAL_INT(ERR_UNSUPPORTED, uart_open(UART_ID_CONSOLE, &cfg));
    TEST_ASSERT_EQUAL_INT(ERR_OK, znp_mt_encode(0x21u, 0x01u, NULL, 0u, tx, sizeof(tx), &n));
    TEST_ASSERT_EQUAL_INT(ERR_OK, uart_write(UART_ID_ZNP, tx, n));
    TEST_ASSERT_EQUAL_INT(ERR_OK, uart_read(UART_ID_ZNP, rx, sizeof(rx), &got, 10u));
    TEST_ASSERT_TRUE(got >= 5u);
    TEST_ASSERT_EQUAL_INT(ERR_OK,
                          znp_mt_decode(rx, got, &cmd0, &cmd1, pl, (uint8_t)sizeof(pl), &plen));
    TEST_ASSERT_EQUAL_HEX8(0x61u, cmd0);
    TEST_ASSERT_EQUAL_HEX8(0x01u, cmd1);
    TEST_ASSERT_EQUAL_UINT8(2u, plen);
}

void test_znp_run(void)
{
    UnitySetTestFile(__FILE__);
    RUN_TEST(test_roundtrip);
    RUN_TEST(test_bad);
    RUN_TEST(test_sys_ping_stub);
}
