#include "unity.h"

#include "app/image_view.h"
#include "fixtures.h"
#include "svc/media.h"
#include "svc/vfs.h"

#include <string.h>

static uint16_t g_px[IMG_DST_MAX_W * IMG_DST_MAX_H];

static void buf_init(image_buf_t *b, uint16_t w, uint16_t h)
{
    memset(g_px, 0xFF, sizeof(g_px));
    b->w = w;
    b->h = h;
    b->stride = IMG_DST_MAX_W;
    b->px = g_px;
}

static uint32_t crop_sum(uint16_t x0, uint16_t y0, uint16_t stride)
{
    uint32_t s = 0u;
    uint16_t y;
    uint16_t x;

    for (y = 0u; y < 16u; y++) {
        for (x = 0u; x < 16u; x++) {
            s += g_px[(uint32_t)(y0 + y) * (uint32_t)stride + (uint32_t)(x0 + x)];
        }
    }
    return s;
}

static void test_bmp(void)
{
    image_buf_t b;
    image_req_t r = {32, 32};
    uint16_t expect;

    buf_init(&b, 32, 32);
    TEST_ASSERT_EQUAL_INT(ERR_OK, media_decode_image_mem(k_golden_bmp, k_golden_bmp_len, &b, &r));
    expect = (uint16_t)((((uint16_t)56 & 0xF8u) << 8) | (((uint16_t)72 & 0xFCu) << 3) |
                        ((uint16_t)16 >> 3));
    TEST_ASSERT_EQUAL_HEX16(expect, g_px[8u * IMG_DST_MAX_W + 8u]);
    TEST_ASSERT_NOT_EQUAL_UINT32(0u, crop_sum(8, 8, IMG_DST_MAX_W));

    buf_init(&b, 4, 2);
    r.max_w = 4;
    r.max_h = 2;
    TEST_ASSERT_EQUAL_INT(ERR_OK, media_decode_image_mem(k_top_bmp, k_top_bmp_len, &b, &r));
    buf_init(&b, 2, 2);
    r.max_w = 2;
    r.max_h = 2;
    TEST_ASSERT_EQUAL_INT(ERR_OK, media_decode_image_mem(k_bgra_bmp, k_bgra_bmp_len, &b, &r));
}

static void test_png(void)
{
    image_buf_t b;
    image_req_t r = {8, 8};

    buf_init(&b, 8, 8);
    TEST_ASSERT_EQUAL_INT(ERR_OK, media_decode_image_mem(k_tiny_png, k_tiny_png_len, &b, &r));
    TEST_ASSERT_NOT_EQUAL_HEX16(0xFFFFu, g_px[0]);

    buf_init(&b, 4, 4);
    r.max_w = 4;
    r.max_h = 4;
    TEST_ASSERT_EQUAL_INT(ERR_OK, media_decode_image_mem(k_alpha_png, k_alpha_png_len, &b, &r));

    buf_init(&b, 4, 2);
    r.max_w = 4;
    r.max_h = 2;
    TEST_ASSERT_EQUAL_INT(ERR_OK, media_decode_image_mem(k_gray_png, k_gray_png_len, &b, &r));

    buf_init(&b, 2, 2);
    r.max_w = 2;
    r.max_h = 2;
    TEST_ASSERT_EQUAL_INT(ERR_OK, media_decode_image_mem(k_pal_png, k_pal_png_len, &b, &r));
    TEST_ASSERT_EQUAL_INT(ERR_OK, media_decode_image_mem(k_ga_png, k_ga_png_len, &b, &r));
    buf_init(&b, 4, 3);
    r.max_w = 4;
    r.max_h = 3;
    TEST_ASSERT_EQUAL_INT(ERR_OK, media_decode_image_mem(k_paeth_png, k_paeth_png_len, &b, &r));
}

static uint32_t g_jpeg_crop;

static void test_jpeg(void)
{
    image_buf_t b;
    image_req_t r = {32, 32};

    buf_init(&b, 32, 32);
    TEST_ASSERT_EQUAL_INT(ERR_OK, media_decode_image_mem(k_golden_jpg, k_golden_jpg_len, &b, &r));
    g_jpeg_crop = crop_sum(8, 8, IMG_DST_MAX_W);
    TEST_ASSERT_NOT_EQUAL_UINT32(0u, g_jpeg_crop);
    /* TC-IMG-03: 16×16 center crop of the 32×32 golden JPEG. */
    TEST_ASSERT_EQUAL_HEX32(0x001ebd7du, g_jpeg_crop);

    buf_init(&b, 32, 32);
    TEST_ASSERT_EQUAL_INT(ERR_CORRUPT,
                          media_decode_image_mem(k_trunc_jpg, k_trunc_jpg_len, &b, &r));
}

static void test_errors_and_hw(void)
{
    image_buf_t b;
    image_req_t r = {16, 16};
    const uint8_t junk[4] = {1, 2, 3, 4};
    const uint8_t fake_png[3] = {(uint8_t)'P', (uint8_t)'N', (uint8_t)'G'};

    buf_init(&b, 16, 16);
    TEST_ASSERT_EQUAL_INT(ERR_INVAL, media_decode_image_mem(NULL, 4u, &b, &r));
    TEST_ASSERT_EQUAL_INT(ERR_CORRUPT, media_decode_image_mem(junk, 4u, &b, &r));
    TEST_ASSERT_EQUAL_INT(ERR_CORRUPT, media_decode_image_mem(fake_png, 3u, &b, &r));
    TEST_ASSERT_EQUAL_INT(ERR_UNSUPPORTED,
                          media_jpeg_hw_decode(k_golden_jpg, k_golden_jpg_len, &b, &r));

    TEST_ASSERT_EQUAL_INT(ERR_OK, vfs_mount());
    TEST_ASSERT_EQUAL_INT(ERR_CORRUPT, media_decode_image("/user/photo.png", &b, &r));
    TEST_ASSERT_EQUAL_INT(ERR_CORRUPT, media_decode_image("/user/hello.txt", &b, &r));
    TEST_ASSERT_EQUAL_INT(ERR_INVAL, media_decode_image(NULL, &b, &r));
    TEST_ASSERT_EQUAL_INT(ERR_NOENT, media_decode_image("/user/nope.jpg", &b, &r));
    {
        image_buf_t bad = {0, 0, 0, NULL};
        TEST_ASSERT_EQUAL_INT(ERR_INVAL,
                              media_decode_image_mem(k_tiny_png, k_tiny_png_len, &bad, &r));
    }
}

static void test_path_and_view(void)
{
    TEST_ASSERT_EQUAL_INT(ERR_OK, vfs_mount());
    TEST_ASSERT_NOT_EQUAL_INT(ERR_OK, image_view_open("/user/photo.png"));
    TEST_ASSERT_EQUAL_INT(ERR_CORRUPT, image_view_status());
    TEST_ASSERT_NOT_EQUAL_CHAR('\0', image_view_err_str()[0]);
    TEST_ASSERT_EQUAL_STRING("photo.png", image_view_name());
    TEST_ASSERT_EQUAL_INT(ERR_NOENT, image_view_next(1));
    image_view_close();
    TEST_ASSERT_EQUAL_INT(ERR_INVAL, image_view_open(NULL));
    TEST_ASSERT_EQUAL_INT(ERR_NOENT, image_view_open("/user/missing.jpg"));
}

void test_image_run(void)
{
    UnitySetTestFile(__FILE__);
    RUN_TEST(test_bmp);
    RUN_TEST(test_png);
    RUN_TEST(test_jpeg);
    RUN_TEST(test_errors_and_hw);
    RUN_TEST(test_path_and_view);
}
