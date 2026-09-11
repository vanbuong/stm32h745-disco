#include "media_priv.h"

#include "svc/vfs.h"

#include <string.h>

void media_buf_clear(image_buf_t *out)
{
    uint32_t n;
    uint32_t i;

    if (out == NULL || out->px == NULL) {
        return;
    }
    n = (uint32_t)out->stride * (uint32_t)out->h;
    for (i = 0u; i < n; i++) {
        out->px[i] = 0u;
    }
}

int media_buf_ok(const image_buf_t *out, const image_req_t *req)
{
    (void)req;
    if (out == NULL || out->px == NULL || out->w == 0u || out->h == 0u) {
        return 0;
    }
    if (out->stride < out->w) {
        return 0;
    }
    return 1;
}

uint16_t media_canvas_w(const image_buf_t *out, const image_req_t *req)
{
    uint16_t w = out->w;
    if (req != NULL && req->max_w != 0u && req->max_w < w) {
        w = req->max_w;
    }
    if (w > IMG_DST_MAX_W) {
        w = IMG_DST_MAX_W;
    }
    return w;
}

uint16_t media_canvas_h(const image_buf_t *out, const image_req_t *req)
{
    uint16_t h = out->h;
    if (req != NULL && req->max_h != 0u && req->max_h < h) {
        h = req->max_h;
    }
    if (h > IMG_DST_MAX_H) {
        h = IMG_DST_MAX_H;
    }
    return h;
}

void media_contain(uint16_t sw, uint16_t sh, uint16_t mw, uint16_t mh, uint16_t *dw, uint16_t *dh,
                   uint16_t *ox, uint16_t *oy)
{
    uint32_t dw32;
    uint32_t dh32;

    if (sw == 0u || sh == 0u || mw == 0u || mh == 0u) {
        *dw = 0u;
        *dh = 0u;
        *ox = 0u;
        *oy = 0u;
        return;
    }
    dw32 = (uint32_t)mw;
    dh32 = (dw32 * (uint32_t)sh) / (uint32_t)sw;
    if (dh32 > (uint32_t)mh) {
        dh32 = (uint32_t)mh;
        dw32 = (dh32 * (uint32_t)sw) / (uint32_t)sh;
    }
    if (dw32 == 0u) {
        dw32 = 1u;
    }
    if (dh32 == 0u) {
        dh32 = 1u;
    }
    if (dw32 > (uint32_t)mw) {
        dw32 = mw;
    }
    if (dh32 > (uint32_t)mh) {
        dh32 = mh;
    }
    *dw = (uint16_t)dw32;
    *dh = (uint16_t)dh32;
    *ox = (uint16_t)((mw - (uint16_t)dw32) / 2u);
    *oy = (uint16_t)((mh - (uint16_t)dh32) / 2u);
}

void media_put_scaled(image_buf_t *out, uint16_t sw, uint16_t sh, uint16_t sx, uint16_t sy,
                      uint8_t r, uint8_t g, uint8_t b)
{
    uint16_t dw;
    uint16_t dh;
    uint16_t ox;
    uint16_t oy;
    uint16_t x0;
    uint16_t x1;
    uint16_t y0;
    uint16_t y1;
    uint16_t x;
    uint16_t y;
    uint16_t c;

    if (out == NULL || out->px == NULL || sw == 0u || sh == 0u) {
        return;
    }
    media_contain(sw, sh, out->w, out->h, &dw, &dh, &ox, &oy);
    x0 = (uint16_t)(ox + ((uint32_t)sx * (uint32_t)dw) / (uint32_t)sw);
    x1 = (uint16_t)(ox + ((uint32_t)(sx + 1u) * (uint32_t)dw) / (uint32_t)sw);
    y0 = (uint16_t)(oy + ((uint32_t)sy * (uint32_t)dh) / (uint32_t)sh);
    y1 = (uint16_t)(oy + ((uint32_t)(sy + 1u) * (uint32_t)dh) / (uint32_t)sh);
    if (x1 <= x0) {
        x1 = (uint16_t)(x0 + 1u);
    }
    if (y1 <= y0) {
        y1 = (uint16_t)(y0 + 1u);
    }
    c = MEDIA_RGB565(r, g, b);
    for (y = y0; y < y1 && y < out->h; y++) {
        for (x = x0; x < x1 && x < out->w; x++) {
            out->px[(uint32_t)y * (uint32_t)out->stride + x] = c;
        }
    }
}

uint16_t media_le16(const uint8_t *p)
{
    return (uint16_t)((uint16_t)p[0] | ((uint16_t)p[1] << 8));
}

uint32_t media_le32(const uint8_t *p)
{
    return (uint32_t)p[0] | ((uint32_t)p[1] << 8) | ((uint32_t)p[2] << 16) | ((uint32_t)p[3] << 24);
}

uint32_t media_be32(const uint8_t *p)
{
    return ((uint32_t)p[0] << 24) | ((uint32_t)p[1] << 16) | ((uint32_t)p[2] << 8) | (uint32_t)p[3];
}

err_t media_jpeg_hw_decode(const uint8_t *data, uint32_t size, image_buf_t *out,
                           const image_req_t *req)
{
    (void)data;
    (void)size;
    (void)out;
    (void)req;
    return ERR_UNSUPPORTED;
}

static err_t sniff_decode(const uint8_t *data, uint32_t size, image_buf_t *out,
                          const image_req_t *req)
{
    err_t hw;

    if (size >= 2u && data[0] == 0xFFu && data[1] == 0xD8u) {
        hw = media_jpeg_hw_decode(data, size, out, req);
        if (hw != ERR_UNSUPPORTED) {
            return hw;
        }
        return media_decode_jpeg_mem(data, size, out, req);
    }
    if (size >= 8u && data[0] == 0x89u && data[1] == (uint8_t)'P' && data[2] == (uint8_t)'N' &&
        data[3] == (uint8_t)'G') {
        return media_decode_png_mem(data, size, out, req);
    }
    if (size >= 2u && data[0] == (uint8_t)'B' && data[1] == (uint8_t)'M') {
        return media_decode_bmp_mem(data, size, out, req);
    }
    return ERR_CORRUPT;
}

err_t media_decode_image_mem(const uint8_t *data, uint32_t size, image_buf_t *out,
                             const image_req_t *req)
{
    if (!media_buf_ok(out, req) || data == NULL || size == 0u) {
        return ERR_INVAL;
    }
    media_buf_clear(out);
    {
        uint16_t cw = media_canvas_w(out, req);
        uint16_t ch = media_canvas_h(out, req);
        out->w = cw;
        out->h = ch;
    }
    return sniff_decode(data, size, out, req);
}

#if defined(STM32H745xx)
#include "bsp/board.h"
#define MEDIA_FILE_MAX (384u * 1024u)
static uint8_t *media_file_buf(void)
{
    return (uint8_t *)(BOARD_SDRAM_BASE + 0x000F0000u);
}
#else
#define MEDIA_FILE_MAX (128u * 1024u)
static uint8_t s_file[MEDIA_FILE_MAX];
static uint8_t *media_file_buf(void)
{
    return s_file;
}
#endif

err_t media_decode_image(const char *path, image_buf_t *out, const image_req_t *req)
{
    vfs_stat_t st;
    vfs_file_t fd = -1;
    uint8_t *scratch = media_file_buf();
    size_t got = 0u;
    err_t e;

    if (path == NULL) {
        return ERR_INVAL;
    }
    e = vfs_stat(path, &st);
    if (e != ERR_OK) {
        return e;
    }
    if (st.is_dir != 0u || st.size == 0u) {
        return ERR_INVAL;
    }
    if (st.size > MEDIA_FILE_MAX) {
        return ERR_NOSPC;
    }
    e = vfs_open(path, VFS_O_RD, &fd);
    if (e != ERR_OK) {
        return e;
    }
    e = vfs_read(fd, scratch, st.size, &got);
    (void)vfs_close(fd);
    if (e != ERR_OK) {
        return e;
    }
    return media_decode_image_mem(scratch, (uint32_t)got, out, req);
}
