#include "media_priv.h"

#include "tjpgd.h"

#include <string.h>

#define JPEG_POOL_SZ 8192u

typedef struct {
    const uint8_t *p;
    uint32_t n;
    uint32_t off;
    image_buf_t *out;
    uint16_t sw;
    uint16_t sh;
} jpeg_sess_t;

static size_t jpeg_in(JDEC *jd, uint8_t *buf, size_t n)
{
    jpeg_sess_t *s = (jpeg_sess_t *)jd->device;
    uint32_t left;

    if (s == NULL) {
        return 0u;
    }
    left = (s->off < s->n) ? (s->n - s->off) : 0u;
    if (n > left) {
        n = left;
    }
    if (buf != NULL && n > 0u) {
        memcpy(buf, s->p + s->off, n);
    }
    s->off += (uint32_t)n;
    return n;
}

static int jpeg_out(JDEC *jd, void *bitmap, JRECT *rect)
{
    jpeg_sess_t *s = (jpeg_sess_t *)jd->device;
    const uint8_t *src = (const uint8_t *)bitmap;
    uint16_t x;
    uint16_t y;
    uint16_t w;

    if (s == NULL || s->out == NULL || rect == NULL) {
        return 0;
    }
    w = (uint16_t)((rect->right - rect->left) + 1u);
    for (y = rect->top; y <= rect->bottom; y++) {
        for (x = rect->left; x <= rect->right; x++) {
            const uint8_t *p =
                src + (size_t)3u * ((size_t)(y - rect->top) * (size_t)w + (size_t)(x - rect->left));
            media_put_scaled(s->out, s->sw, s->sh, x, y, p[0], p[1], p[2]);
        }
    }
    return 1;
}

static uint8_t pick_scale(uint16_t w, uint16_t h, uint16_t mw, uint16_t mh)
{
    uint8_t scale = 0u;
    uint16_t tw = w;
    uint16_t th = h;

    while (scale < 3u) {
        tw = (uint16_t)(w >> scale);
        th = (uint16_t)(h >> scale);
        if (tw <= (uint16_t)(mw * 2u) && th <= (uint16_t)(mh * 2u)) {
            break;
        }
        scale++;
    }
    (void)tw;
    (void)th;
    return scale;
}

err_t media_decode_jpeg_mem(const uint8_t *data, uint32_t size, image_buf_t *out,
                            const image_req_t *req)
{
    static uint8_t pool[JPEG_POOL_SZ];
    JDEC jd;
    jpeg_sess_t sess;
    JRESULT rc;
    uint8_t scale;
    uint16_t mw;
    uint16_t mh;

    if (data == NULL || size < 4u) {
        return ERR_CORRUPT;
    }
    memset(&jd, 0, sizeof(jd));
    memset(&sess, 0, sizeof(sess));
    sess.p = data;
    sess.n = size;
    sess.out = out;
    rc = jd_prepare(&jd, jpeg_in, pool, JPEG_POOL_SZ, &sess);
    if (rc != JDR_OK) {
        return (rc == JDR_MEM1 || rc == JDR_MEM2) ? ERR_NOSPC : ERR_CORRUPT;
    }
    mw = media_canvas_w(out, req);
    mh = media_canvas_h(out, req);
    scale = pick_scale(jd.width, jd.height, mw, mh);
    sess.sw = (uint16_t)(jd.width >> scale);
    sess.sh = (uint16_t)(jd.height >> scale);
    if (sess.sw == 0u) {
        sess.sw = 1u;
    }
    if (sess.sh == 0u) {
        sess.sh = 1u;
    }
    rc = jd_decomp(&jd, jpeg_out, scale);
    if (rc != JDR_OK) {
        return ERR_CORRUPT;
    }
    return ERR_OK;
}
