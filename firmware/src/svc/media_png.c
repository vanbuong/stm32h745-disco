#include "media_priv.h"

#include "puff.h"

#include <string.h>

#if defined(STM32H745xx)
#include "bsp/board.h"
#define PNG_IDAT_MAX (256u * 1024u)
#define PNG_RAW_MAX (512u * 1024u)
#define PNG_WORK (BOARD_SDRAM_BASE + 0x00150000u)
static uint8_t *png_idat(void)
{
    return (uint8_t *)PNG_WORK;
}
static uint8_t *png_raw(void)
{
    return (uint8_t *)(PNG_WORK + PNG_IDAT_MAX);
}
#else
#define PNG_IDAT_MAX (32u * 1024u)
#define PNG_RAW_MAX (64u * 1024u)
static uint8_t s_idat[PNG_IDAT_MAX];
static uint8_t s_raw[PNG_RAW_MAX];
static uint8_t *png_idat(void)
{
    return s_idat;
}
static uint8_t *png_raw(void)
{
    return s_raw;
}
#endif

static const uint8_t k_sig[8] = {0x89u, 0x50u, 0x4Eu, 0x47u, 0x0Du, 0x0Au, 0x1Au, 0x0Au};

static int paeth(int a, int b, int c)
{
    int p = a + b - c;
    int pa = p - a;
    int pb = p - b;
    int pc = p - c;
    if (pa < 0) {
        pa = -pa;
    }
    if (pb < 0) {
        pb = -pb;
    }
    if (pc < 0) {
        pc = -pc;
    }
    if (pa <= pb && pa <= pc) {
        return a;
    }
    if (pb <= pc) {
        return b;
    }
    return c;
}

static void unfilter_row(uint8_t *row, const uint8_t *prev, uint32_t len, uint32_t bpp, uint8_t t)
{
    uint32_t i;
    for (i = 0u; i < len; i++) {
        uint8_t x = row[i];
        uint8_t a = (i >= bpp) ? row[i - bpp] : 0u;
        uint8_t b = (prev != NULL) ? prev[i] : 0u;
        uint8_t c = (prev != NULL && i >= bpp) ? prev[i - bpp] : 0u;
        if (t == 1u) {
            row[i] = (uint8_t)(x + a);
        } else if (t == 2u) {
            row[i] = (uint8_t)(x + b);
        } else if (t == 3u) {
            row[i] = (uint8_t)(x + (uint8_t)(((unsigned)a + (unsigned)b) / 2u));
        } else if (t == 4u) {
            row[i] = (uint8_t)(x + (uint8_t)paeth((int)a, (int)b, (int)c));
        }
    }
}

static void pixel_rgb(uint8_t ct, const uint8_t *plte, uint16_t nplte, const uint8_t *p, uint8_t *r,
                      uint8_t *g, uint8_t *b)
{
    if (ct == 0u || ct == 4u) {
        *r = *g = *b = p[0];
        return;
    }
    if (ct == 2u || ct == 6u) {
        *r = p[0];
        *g = p[1];
        *b = p[2];
        return;
    }
    if (ct == 3u) {
        uint8_t idx = p[0];
        if ((uint16_t)idx < nplte) {
            size_t o = (size_t)idx * 3u;
            *r = plte[o];
            *g = plte[o + 1u];
            *b = plte[o + 2u];
            return;
        }
    }
    *r = *g = *b = 0u;
}

static uint32_t samples_bpp(uint8_t ct)
{
    if (ct == 0u || ct == 3u) {
        return 1u;
    }
    if (ct == 2u) {
        return 3u;
    }
    if (ct == 4u) {
        return 2u;
    }
    if (ct == 6u) {
        return 4u;
    }
    return 0u;
}

static err_t inflate_zlib(const uint8_t *src, uint32_t src_n, uint8_t *dst, uint32_t dst_cap,
                          uint32_t *out_n)
{
    unsigned long dlen;
    unsigned long slen;
    int pr;

    if (src_n < 2u) {
        return ERR_CORRUPT;
    }
    if ((src[0] & 0x0Fu) != 8u) {
        return ERR_UNSUPPORTED;
    }
    if (((((uint32_t)src[0] << 8) + (uint32_t)src[1]) % 31u) != 0u) {
        return ERR_CORRUPT;
    }
    if ((src[1] & 0x20u) != 0u) {
        return ERR_UNSUPPORTED;
    }
    dlen = dst_cap;
    slen = src_n - 2u;
    pr = puff(dst, &dlen, src + 2, &slen);
    if (pr != 0) {
        return ERR_CORRUPT;
    }
    *out_n = (uint32_t)dlen;
    return ERR_OK;
}

static err_t blit_png(uint8_t *rows, uint32_t raw_n, uint16_t w, uint16_t h, uint8_t ct,
                      const uint8_t *plte, uint16_t nplte, image_buf_t *out)
{
    uint32_t bpp = samples_bpp(ct);
    uint32_t stride = (uint32_t)w * bpp;
    uint32_t y;
    const uint8_t *prev = NULL;

    if (bpp == 0u) {
        return ERR_UNSUPPORTED;
    }
    if (raw_n < (stride + 1u) * (uint32_t)h) {
        return ERR_CORRUPT;
    }
    for (y = 0u; y < h; y++) {
        uint8_t *row = rows + (size_t)y * (size_t)(stride + 1u);
        uint8_t t = row[0];
        uint16_t x;
        if (t > 4u) {
            return ERR_CORRUPT;
        }
        unfilter_row(row + 1, prev, stride, bpp, t);
        prev = row + 1;
        for (x = 0u; x < w; x++) {
            uint8_t r;
            uint8_t g;
            uint8_t b;
            pixel_rgb(ct, plte, nplte, row + 1u + (size_t)x * (size_t)bpp, &r, &g, &b);
            media_put_scaled(out, w, h, x, (uint16_t)y, r, g, b);
        }
    }
    return ERR_OK;
}

static err_t png_ihdr(const uint8_t *chunk, uint32_t len, uint16_t *w, uint16_t *h, uint8_t *depth,
                      uint8_t *ct, uint8_t *inter)
{
    uint32_t ww;
    uint32_t hh;

    if (len != 13u) {
        return ERR_CORRUPT;
    }
    ww = media_be32(chunk);
    hh = media_be32(chunk + 4);
    if (ww == 0u || hh == 0u || ww > 4096u || hh > 4096u) {
        return ERR_NOSPC;
    }
    *w = (uint16_t)ww;
    *h = (uint16_t)hh;
    *depth = chunk[8];
    *ct = chunk[9];
    *inter = chunk[12];
    if (*depth != 8u || *inter != 0u) {
        return ERR_UNSUPPORTED;
    }
    if (*ct != 0u && *ct != 2u && *ct != 3u && *ct != 4u && *ct != 6u) {
        return ERR_UNSUPPORTED;
    }
    return ERR_OK;
}

static err_t png_add_idat(uint8_t *idat, uint32_t *idat_n, const uint8_t *chunk, uint32_t len)
{
    if (*idat_n + len > PNG_IDAT_MAX) {
        return ERR_NOSPC;
    }
    memcpy(idat + *idat_n, chunk, len);
    *idat_n += len;
    return ERR_OK;
}

static err_t png_chunk(const uint8_t *type, const uint8_t *chunk, uint32_t len, uint8_t *have_ihdr,
                       uint16_t *w, uint16_t *h, uint8_t *depth, uint8_t *ct, uint8_t *inter,
                       uint8_t *plte, uint16_t *nplte, uint8_t *idat, uint32_t *idat_n, int *done)
{
    *done = 0;
    if (memcmp(type, "IHDR", 4) == 0) {
        err_t e;
        if (*have_ihdr != 0u) {
            return ERR_CORRUPT;
        }
        e = png_ihdr(chunk, len, w, h, depth, ct, inter);
        if (e != ERR_OK) {
            return e;
        }
        *have_ihdr = 1u;
        return ERR_OK;
    }
    if (memcmp(type, "PLTE", 4) == 0) {
        if (len == 0u || (len % 3u) != 0u || len > 256u * 3u) {
            return ERR_CORRUPT;
        }
        memcpy(plte, chunk, len);
        *nplte = (uint16_t)(len / 3u);
        return ERR_OK;
    }
    if (memcmp(type, "IDAT", 4) == 0) {
        return png_add_idat(idat, idat_n, chunk, len);
    }
    if (memcmp(type, "IEND", 4) == 0) {
        *done = 1;
        return ERR_OK;
    }
    if ((type[0] & 0x20u) == 0u) {
        return ERR_UNSUPPORTED;
    }
    return ERR_OK;
}

err_t media_decode_png_mem(const uint8_t *data, uint32_t size, image_buf_t *out,
                           const image_req_t *req)
{
    uint32_t pos = 8u;
    uint32_t idat_n = 0u;
    uint8_t *idat = png_idat();
    uint8_t plte[256 * 3];
    uint16_t nplte = 0u;
    uint16_t w = 0u;
    uint16_t h = 0u;
    uint8_t depth = 0u;
    uint8_t ct = 0u;
    uint8_t inter = 0u;
    uint8_t have_ihdr = 0u;
    uint32_t raw_n = 0u;
    err_t e;

    (void)req;
    if (data == NULL || size < 33u || memcmp(data, k_sig, 8) != 0) {
        return ERR_CORRUPT;
    }
    memset(plte, 0, sizeof(plte));
    while (pos + 12u <= size) {
        uint32_t len = media_be32(data + pos);
        int done = 0;
        if (len > size - (pos + 12u)) {
            return ERR_CORRUPT;
        }
        e = png_chunk(data + pos + 4u, data + pos + 8u, len, &have_ihdr, &w, &h, &depth, &ct,
                      &inter, plte, &nplte, idat, &idat_n, &done);
        if (e != ERR_OK) {
            return e;
        }
        if (done != 0) {
            break;
        }
        pos += 12u + len;
    }
    if (have_ihdr == 0u || idat_n == 0u || (ct == 3u && nplte == 0u)) {
        return ERR_CORRUPT;
    }
    e = inflate_zlib(idat, idat_n, png_raw(), PNG_RAW_MAX, &raw_n);
    if (e != ERR_OK) {
        return e;
    }
    return blit_png(png_raw(), raw_n, w, h, ct, plte, nplte, out);
}
