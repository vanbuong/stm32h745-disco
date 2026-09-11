#include "unity.h"

#include "bsp/disp_geom.h"

static void test_clip(void)
{
    disp_rect_t r;

    r.x = 0;
    r.y = 0;
    r.w = 480;
    r.h = 272;
    TEST_ASSERT_EQUAL_INT(1, disp_clip_rect(&r, 480, 272));
    TEST_ASSERT_EQUAL_UINT16(480, r.w);
    TEST_ASSERT_EQUAL_UINT16(272, r.h);

    r.x = 470;
    r.y = 0;
    r.w = 20;
    r.h = 10;
    TEST_ASSERT_EQUAL_INT(1, disp_clip_rect(&r, 480, 272));
    TEST_ASSERT_EQUAL_UINT16(470, r.x);
    TEST_ASSERT_EQUAL_UINT16(10, r.w);

    r.x = 480;
    r.y = 0;
    r.w = 10;
    r.h = 10;
    TEST_ASSERT_EQUAL_INT(0, disp_clip_rect(&r, 480, 272));
    TEST_ASSERT_EQUAL_UINT16(0, r.w);
    TEST_ASSERT_EQUAL_UINT16(0, r.h);

    r.x = 0;
    r.y = 270;
    r.w = 10;
    r.h = 10;
    TEST_ASSERT_EQUAL_INT(1, disp_clip_rect(&r, 480, 272));
    TEST_ASSERT_EQUAL_UINT16(2, r.h);

    r.x = 10;
    r.y = 10;
    r.w = 0;
    r.h = 10;
    TEST_ASSERT_EQUAL_INT(0, disp_clip_rect(&r, 480, 272));

    TEST_ASSERT_EQUAL_INT(0, disp_clip_rect(NULL, 480, 272));
}

static void test_rgb565(void)
{
    TEST_ASSERT_EQUAL_HEX16(0u, disp_rgb565(0, 0, 0));
    TEST_ASSERT_EQUAL_HEX16(0xFFFFu, disp_rgb565(255, 255, 255));
    TEST_ASSERT_EQUAL_HEX16(0xF800u, disp_rgb565(255, 0, 0));
    TEST_ASSERT_EQUAL_HEX16(0x07E0u, disp_rgb565(0, 255, 0));
    TEST_ASSERT_EQUAL_HEX16(0x001Fu, disp_rgb565(0, 0, 255));
}

void test_disp_run(void)
{
    UnitySetTestFile(__FILE__);
    RUN_TEST(test_clip);
    RUN_TEST(test_rgb565);
}
