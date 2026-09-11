#include "test.h"

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
    CHECK(media_decode_image_mem(k_golden_bmp, k_golden_bmp_len, &b, &r) == ERR_OK);
    expect = (uint16_t)((((uint16_t)56 & 0xF8u) << 8) | (((uint16_t)72 & 0xFCu) << 3) |
                        ((uint16_t)16 >> 3));
    CHECK(g_px[8u * IMG_DST_MAX_W + 8u] == expect);
    CHECK(crop_sum(8, 8, IMG_DST_MAX_W) != 0u);

    buf_init(&b, 4, 2);
    r.max_w = 4;
    r.max_h = 2;
    CHECK(media_decode_image_mem(k_top_bmp, k_top_bmp_len, &b, &r) == ERR_OK);
    buf_init(&b, 2, 2);
    r.max_w = 2;
    r.max_h = 2;
    CHECK(media_decode_image_mem(k_bgra_bmp, k_bgra_bmp_len, &b, &r) == ERR_OK);
}

static void test_png(void)
{
    image_buf_t b;
    image_req_t r = {8, 8};

    buf_init(&b, 8, 8);
    CHECK(media_decode_image_mem(k_tiny_png, k_tiny_png_len, &b, &r) == ERR_OK);
    CHECK(g_px[0] != 0xFFFFu);

    buf_init(&b, 4, 4);
    r.max_w = 4;
    r.max_h = 4;
    CHECK(media_decode_image_mem(k_alpha_png, k_alpha_png_len, &b, &r) == ERR_OK);

    buf_init(&b, 4, 2);
    r.max_w = 4;
    r.max_h = 2;
    CHECK(media_decode_image_mem(k_gray_png, k_gray_png_len, &b, &r) == ERR_OK);

    buf_init(&b, 2, 2);
    r.max_w = 2;
    r.max_h = 2;
    CHECK(media_decode_image_mem(k_pal_png, k_pal_png_len, &b, &r) == ERR_OK);
    CHECK(media_decode_image_mem(k_ga_png, k_ga_png_len, &b, &r) == ERR_OK);
    buf_init(&b, 4, 3);
    r.max_w = 4;
    r.max_h = 3;
    CHECK(media_decode_image_mem(k_paeth_png, k_paeth_png_len, &b, &r) == ERR_OK);
}

static uint32_t g_jpeg_crop;

static void test_jpeg(void)
{
    image_buf_t b;
    image_req_t r = {32, 32};

    buf_init(&b, 32, 32);
    CHECK(media_decode_image_mem(k_golden_jpg, k_golden_jpg_len, &b, &r) == ERR_OK);
    g_jpeg_crop = crop_sum(8, 8, IMG_DST_MAX_W);
    CHECK(g_jpeg_crop != 0u);
    /* TC-IMG-03: 16×16 center crop of the 32×32 golden JPEG. */
    CHECK(g_jpeg_crop == 0x001ebd7du);

    buf_init(&b, 32, 32);
    CHECK(media_decode_image_mem(k_trunc_jpg, k_trunc_jpg_len, &b, &r) == ERR_CORRUPT);
}

static void test_errors_and_hw(void)
{
    image_buf_t b;
    image_req_t r = {16, 16};
    const uint8_t junk[4] = {1, 2, 3, 4};
    const uint8_t fake_png[3] = {(uint8_t)'P', (uint8_t)'N', (uint8_t)'G'};

    buf_init(&b, 16, 16);
    CHECK(media_decode_image_mem(NULL, 4u, &b, &r) == ERR_INVAL);
    CHECK(media_decode_image_mem(junk, 4u, &b, &r) == ERR_CORRUPT);
    CHECK(media_decode_image_mem(fake_png, 3u, &b, &r) == ERR_CORRUPT);
    CHECK(media_jpeg_hw_decode(k_golden_jpg, k_golden_jpg_len, &b, &r) == ERR_UNSUPPORTED);

    CHECK(vfs_mount() == ERR_OK);
    CHECK(media_decode_image("/user/photo.png", &b, &r) == ERR_CORRUPT);
    CHECK(media_decode_image("/user/hello.txt", &b, &r) == ERR_CORRUPT);
    CHECK(media_decode_image(NULL, &b, &r) == ERR_INVAL);
    CHECK(media_decode_image("/user/nope.jpg", &b, &r) == ERR_NOENT);
    {
        image_buf_t bad = {0, 0, 0, NULL};
        CHECK(media_decode_image_mem(k_tiny_png, k_tiny_png_len, &bad, &r) == ERR_INVAL);
    }
}

static void test_path_and_view(void)
{
    CHECK(vfs_mount() == ERR_OK);
    CHECK(image_view_open("/user/photo.png") != ERR_OK);
    CHECK(image_view_status() == ERR_CORRUPT);
    CHECK(image_view_err_str()[0] != '\0');
    CHECK(strcmp(image_view_name(), "photo.png") == 0);
    CHECK(image_view_next(1) == ERR_NOENT);
    image_view_close();
    CHECK(image_view_open(NULL) == ERR_INVAL);
    CHECK(image_view_open("/user/missing.jpg") == ERR_NOENT);
}

void test_image_run(void)
{
    test_bmp();
    test_png();
    test_jpeg();
    test_errors_and_hw();
    test_path_and_view();
}
