#include "media_priv.h"

#include <string.h>

#define BMP_MAX_DIM 4096u

static int32_t s32_le(const uint8_t *p)
{
    return (int32_t)media_le32(p);
}

err_t media_decode_bmp_mem(const uint8_t *data, uint32_t size, image_buf_t *out,
                           const image_req_t *req)
{
    uint32_t pix_off;
    uint32_t dib;
    int32_t width;
    int32_t height;
    uint16_t bpp;
    uint32_t comp;
    uint16_t sw;
    uint16_t sh;
    uint32_t row_bytes;
    int top_down;
    uint32_t y;

    (void)req;
    if (data == NULL || size < 54u) {
        return ERR_CORRUPT;
    }
    if (data[0] != (uint8_t)'B' || data[1] != (uint8_t)'M') {
        return ERR_CORRUPT;
    }
    pix_off = media_le32(data + 10);
    dib = media_le32(data + 14);
    if (dib < 40u || pix_off < 14u + dib || pix_off >= size) {
        return ERR_CORRUPT;
    }
    width = s32_le(data + 18);
    height = s32_le(data + 22);
    bpp = media_le16(data + 28);
    comp = media_le32(data + 30);
    if (width <= 0 || height == 0 || comp != 0u) {
        return ERR_UNSUPPORTED;
    }
    if (bpp != 24u && bpp != 32u) {
        return ERR_UNSUPPORTED;
    }
    top_down = 0;
    if (height < 0) {
        height = -height;
        top_down = 1;
    }
    if ((uint32_t)width > BMP_MAX_DIM || (uint32_t)height > BMP_MAX_DIM) {
        return ERR_NOSPC;
    }
    sw = (uint16_t)width;
    sh = (uint16_t)height;
    row_bytes = ((uint32_t)sw * (bpp / 8u) + 3u) & ~3u;
    if (pix_off + row_bytes * (uint32_t)sh > size) {
        return ERR_CORRUPT;
    }
    for (y = 0u; y < sh; y++) {
        uint32_t src_y = top_down ? y : ((uint32_t)sh - 1u - y);
        const uint8_t *row = data + (size_t)pix_off + (size_t)src_y * (size_t)row_bytes;
        uint16_t x;
        for (x = 0u; x < sw; x++) {
            const uint8_t *px = row + (size_t)x * (size_t)(bpp / 8u);
            media_put_scaled(out, sw, sh, x, (uint16_t)y, px[2], px[1], px[0]);
        }
    }
    return ERR_OK;
}
