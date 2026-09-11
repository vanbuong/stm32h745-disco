#include "unity.h"

#include "svc/auto.h"
#include "svc/media.h"

#include <string.h>

static void test_media(void)
{
    TEST_ASSERT_TRUE(media_probe_ext("a.mp3") == MEDIA_KIND_AUDIO);
    TEST_ASSERT_TRUE(media_probe_ext("a.WAV") == MEDIA_KIND_AUDIO);
    TEST_ASSERT_TRUE(media_probe_ext("x.jpg") == MEDIA_KIND_IMAGE);
    TEST_ASSERT_TRUE(media_probe_ext("x.jpeg") == MEDIA_KIND_IMAGE);
    TEST_ASSERT_TRUE(media_probe_ext("x.png") == MEDIA_KIND_IMAGE);
    TEST_ASSERT_TRUE(media_probe_ext("x.bmp") == MEDIA_KIND_IMAGE);
    TEST_ASSERT_TRUE(media_probe_ext("n.txt") == MEDIA_KIND_TEXT);
    TEST_ASSERT_TRUE(media_probe_ext("n.md") == MEDIA_KIND_TEXT);
    TEST_ASSERT_TRUE(media_probe_ext("n.c") == MEDIA_KIND_TEXT);
    TEST_ASSERT_TRUE(media_probe_ext("n.h") == MEDIA_KIND_TEXT);
    TEST_ASSERT_TRUE(media_probe_ext("n.log") == MEDIA_KIND_TEXT);
    TEST_ASSERT_TRUE(media_probe_ext("n.bin") == MEDIA_KIND_NONE);
    TEST_ASSERT_TRUE(media_probe_ext(NULL) == MEDIA_KIND_NONE);
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
    TEST_ASSERT_TRUE(auto_add(&r) == ERR_OK);
    TEST_ASSERT_TRUE(auto_eval(&d) == ERR_OK);
    TEST_ASSERT_TRUE(auto_last_id() == 7);
    d.on = 0;
    TEST_ASSERT_TRUE(auto_eval(&d) == ERR_NOENT);
}

void test_media_auto_run(void)
{
    UnitySetTestFile(__FILE__);
    RUN_TEST(test_media);
    RUN_TEST(test_auto);
}
