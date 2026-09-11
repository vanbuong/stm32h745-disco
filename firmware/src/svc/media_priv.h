#ifndef MEDIA_PRIV_H
#define MEDIA_PRIV_H

#include "svc/media.h"

#include <stdint.h>

#define MEDIA_RGB565(r, g, b)                                                                      \
    (uint16_t)((((uint16_t)(r) & 0xF8u) << 8) | (((uint16_t)(g) & 0xFCu) << 3) |                   \
               ((uint16_t)(b) >> 3))

void media_buf_clear(image_buf_t *out);
int media_buf_ok(const image_buf_t *out, const image_req_t *req);
uint16_t media_canvas_w(const image_buf_t *out, const image_req_t *req);
uint16_t media_canvas_h(const image_buf_t *out, const image_req_t *req);
void media_contain(uint16_t sw, uint16_t sh, uint16_t mw, uint16_t mh, uint16_t *dw, uint16_t *dh,
                   uint16_t *ox, uint16_t *oy);
void media_put_scaled(image_buf_t *out, uint16_t sw, uint16_t sh, uint16_t sx, uint16_t sy,
                      uint8_t r, uint8_t g, uint8_t b);

err_t media_decode_bmp_mem(const uint8_t *data, uint32_t size, image_buf_t *out,
                           const image_req_t *req);
err_t media_decode_jpeg_mem(const uint8_t *data, uint32_t size, image_buf_t *out,
                            const image_req_t *req);
err_t media_decode_png_mem(const uint8_t *data, uint32_t size, image_buf_t *out,
                           const image_req_t *req);

uint16_t media_le16(const uint8_t *p);
uint32_t media_le32(const uint8_t *p);
uint32_t media_be32(const uint8_t *p);

#endif /* MEDIA_PRIV_H */
