#include "game/gfx.h"

#include <stddef.h>

static gfx_stats_t g_stats;

void gfx_stats_reset(void)
{
    g_stats.clears = 0u;
    g_stats.fills = 0u;
    g_stats.blits = 0u;
    g_stats.last_fill.x = 0;
    g_stats.last_fill.y = 0;
    g_stats.last_fill.w = 0u;
    g_stats.last_fill.h = 0u;
    g_stats.last_clear = 0u;
    g_stats.last_fill_color = 0u;
}

const gfx_stats_t *gfx_stats(void)
{
    return &g_stats;
}

void gfx_clear(gfx_t *fx, uint16_t rgb565)
{
    gfx_rect_t r;

    g_stats.clears++;
    g_stats.last_clear = rgb565;
    if (fx == NULL || fx->fb == NULL) {
        return;
    }
    r.x = 0;
    r.y = 0;
    r.w = fx->w;
    r.h = fx->h;
    gfx_fill(fx, r, rgb565);
    g_stats.fills--;
}

void gfx_fill(gfx_t *fx, gfx_rect_t r, uint16_t rgb565)
{
    int x0;
    int y0;
    int x1;
    int y1;
    int y;
    int x;

    g_stats.fills++;
    g_stats.last_fill = r;
    g_stats.last_fill_color = rgb565;
    if (fx == NULL || fx->fb == NULL || r.w == 0u || r.h == 0u) {
        return;
    }
    x0 = (int)r.x;
    y0 = (int)r.y;
    x1 = x0 + (int)r.w;
    y1 = y0 + (int)r.h;
    if (x0 < 0) {
        x0 = 0;
    }
    if (y0 < 0) {
        y0 = 0;
    }
    if (x1 > (int)fx->w) {
        x1 = (int)fx->w;
    }
    if (y1 > (int)fx->h) {
        y1 = (int)fx->h;
    }
    if (x0 >= x1 || y0 >= y1) {
        return;
    }
    for (y = y0; y < y1; y++) {
        uint16_t *row = fx->fb + (uint32_t)y * (uint32_t)fx->stride;

        for (x = x0; x < x1; x++) {
            row[x] = rgb565;
        }
    }
}

void gfx_blit(gfx_t *fx, int x, int y, const gfx_sprite_t *s)
{
    int x0;
    int y0;
    int x1;
    int y1;
    int src_x0;
    int src_y0;
    int yy;
    int xx;

    g_stats.blits++;
    if (fx == NULL || fx->fb == NULL || s == NULL || s->pixels == NULL || s->w == 0u ||
        s->h == 0u) {
        return;
    }
    x0 = x;
    y0 = y;
    x1 = x + (int)s->w;
    y1 = y + (int)s->h;
    src_x0 = 0;
    src_y0 = 0;
    if (x0 < 0) {
        src_x0 = -x0;
        x0 = 0;
    }
    if (y0 < 0) {
        src_y0 = -y0;
        y0 = 0;
    }
    if (x1 > (int)fx->w) {
        x1 = (int)fx->w;
    }
    if (y1 > (int)fx->h) {
        y1 = (int)fx->h;
    }
    if (x0 >= x1 || y0 >= y1) {
        return;
    }
    for (yy = y0; yy < y1; yy++) {
        const uint16_t *src = s->pixels + (uint32_t)(src_y0 + (yy - y0)) * (uint32_t)s->w;
        uint16_t *dst = fx->fb + (uint32_t)yy * (uint32_t)fx->stride;

        for (xx = x0; xx < x1; xx++) {
            uint16_t p = src[src_x0 + (xx - x0)];

            if (p != 0u) {
                dst[xx] = p;
            }
        }
    }
}
