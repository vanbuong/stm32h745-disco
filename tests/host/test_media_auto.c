#include "unity.h"

#include "svc/auto.h"
#include "svc/media.h"

#include <string.h>

static void test_media(void)
{
    TEST_ASSERT_EQUAL_INT(MEDIA_KIND_AUDIO, media_probe_ext("a.mp3"));
    TEST_ASSERT_EQUAL_INT(MEDIA_KIND_AUDIO, media_probe_ext("a.WAV"));
    TEST_ASSERT_EQUAL_INT(MEDIA_KIND_IMAGE, media_probe_ext("x.jpg"));
    TEST_ASSERT_EQUAL_INT(MEDIA_KIND_IMAGE, media_probe_ext("x.jpeg"));
    TEST_ASSERT_EQUAL_INT(MEDIA_KIND_IMAGE, media_probe_ext("x.png"));
    TEST_ASSERT_EQUAL_INT(MEDIA_KIND_IMAGE, media_probe_ext("x.bmp"));
    TEST_ASSERT_EQUAL_INT(MEDIA_KIND_TEXT, media_probe_ext("n.txt"));
    TEST_ASSERT_EQUAL_INT(MEDIA_KIND_TEXT, media_probe_ext("n.md"));
    TEST_ASSERT_EQUAL_INT(MEDIA_KIND_TEXT, media_probe_ext("n.c"));
    TEST_ASSERT_EQUAL_INT(MEDIA_KIND_TEXT, media_probe_ext("n.h"));
    TEST_ASSERT_EQUAL_INT(MEDIA_KIND_TEXT, media_probe_ext("n.log"));
    TEST_ASSERT_EQUAL_INT(MEDIA_KIND_NONE, media_probe_ext("n.bin"));
    TEST_ASSERT_EQUAL_INT(MEDIA_KIND_NONE, media_probe_ext(NULL));
}

static void test_auto(void)
{
    auto_rule_t r;
    home_device_t d;
    memset(&r, 0, sizeof(r));
    memset(&d, 0, sizeof(d));
    auto_reset();
    r.id = 7;
    r.enabled = 1;
    r.trig = AUTO_TRIG_OCCUPIED;
    r.trig_ieee[0] = 0x11;
    d.ieee[0] = 0x11;
    d.on = 1;
    TEST_ASSERT_EQUAL_INT(ERR_OK, auto_add(&r));
    TEST_ASSERT_EQUAL_INT(ERR_OK, auto_eval(&d));
    TEST_ASSERT_EQUAL_UINT16(7, auto_last_id());
    d.on = 0;
    TEST_ASSERT_EQUAL_INT(ERR_NOENT, auto_eval(&d));
}

void test_media_auto_run(void)
{
    UnitySetTestFile(__FILE__);
    RUN_TEST(test_media);
    RUN_TEST(test_auto);
}
