#include "unity.h"

#include "bsp/disp_geom.h"

static void test_clip(void)
{
    disp_rect_t r;

    r.x = 0;
    r.y = 0;
    r.w = 480;
    r.h = 272;
    TEST_ASSERT_TRUE(disp_clip_rect(&r, 480, 272) == 1);
    TEST_ASSERT_TRUE(r.w == 480);
    TEST_ASSERT_TRUE(r.h == 272);

    r.x = 470;
    r.y = 0;
    r.w = 20;
    r.h = 10;
    TEST_ASSERT_TRUE(disp_clip_rect(&r, 480, 272) == 1);
    TEST_ASSERT_TRUE(r.x == 470);
    TEST_ASSERT_TRUE(r.w == 10);

    r.x = 480;
    r.y = 0;
    r.w = 10;
    r.h = 10;
    TEST_ASSERT_TRUE(disp_clip_rect(&r, 480, 272) == 0);
    TEST_ASSERT_TRUE(r.w == 0);
    TEST_ASSERT_TRUE(r.h == 0);

    r.x = 0;
    r.y = 270;
    r.w = 10;
    r.h = 10;
    TEST_ASSERT_TRUE(disp_clip_rect(&r, 480, 272) == 1);
    TEST_ASSERT_TRUE(r.h == 2);

    r.x = 10;
    r.y = 10;
    r.w = 0;
    r.h = 10;
    TEST_ASSERT_TRUE(disp_clip_rect(&r, 480, 272) == 0);

    TEST_ASSERT_TRUE(disp_clip_rect(NULL, 480, 272) == 0);
}

static void test_rgb565(void)
{
    TEST_ASSERT_TRUE(disp_rgb565(0, 0, 0) == 0u);
    TEST_ASSERT_TRUE(disp_rgb565(255, 255, 255) == 0xFFFFu);
    TEST_ASSERT_TRUE(disp_rgb565(255, 0, 0) == 0xF800u);
    TEST_ASSERT_TRUE(disp_rgb565(0, 255, 0) == 0x07E0u);
    TEST_ASSERT_TRUE(disp_rgb565(0, 0, 255) == 0x001Fu);
}

void test_disp_run(void)
{
    UnitySetTestFile(__FILE__);
    RUN_TEST(test_clip);
    RUN_TEST(test_rgb565);
}
