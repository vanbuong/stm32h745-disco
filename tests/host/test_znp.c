#include "test.h"

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

    CHECK(znp_mt_encode(0x21, 0x02, pl, 3, frame, sizeof(frame), &n) == ERR_OK);
    CHECK(n == 8);
    CHECK(frame[0] == ZNP_SOF);
    CHECK(znp_mt_decode(frame, n, &cmd0, &cmd1, outp, sizeof(outp), &len) == ERR_OK);
    CHECK(cmd0 == 0x21);
    CHECK(cmd1 == 0x02);
    CHECK(len == 3);
    CHECK(memcmp(outp, pl, 3) == 0);
}

static void test_bad(void)
{
    uint8_t frame[16];
    uint8_t cmd0;
    uint8_t cmd1;
    uint8_t len;
    uint8_t outp[8];
    size_t n;

    CHECK(znp_mt_encode(0x01, 0x00, NULL, 0, frame, sizeof(frame), &n) == ERR_OK);
    frame[n - 1u] ^= 0xFFu;
    CHECK(znp_mt_decode(frame, n, &cmd0, &cmd1, outp, sizeof(outp), &len) == ERR_CORRUPT);
    frame[0] = 0x00;
    CHECK(znp_mt_decode(frame, n, &cmd0, &cmd1, outp, sizeof(outp), &len) == ERR_CORRUPT);
    CHECK(znp_mt_encode(0x01, 0x00, NULL, 1, frame, sizeof(frame), &n) == ERR_INVAL);
    CHECK(znp_mt_encode(0x01, 0x00, NULL, 0, frame, 3, &n) == ERR_NOSPC);
    CHECK(znp_mt_decode(NULL, 8, &cmd0, &cmd1, outp, sizeof(outp), &len) == ERR_INVAL);
}

void test_znp_run(void)
{
    test_roundtrip();
    test_bad();
}
