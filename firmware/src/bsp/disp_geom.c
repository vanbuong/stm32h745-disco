#include "bsp/disp_geom.h"

#include <stddef.h>

int disp_clip_rect(disp_rect_t *r, uint16_t max_w, uint16_t max_h)
{
    uint32_t x1;
    uint32_t y1;

    if (r == NULL || r->w == 0u || r->h == 0u) {
        return 0;
    }
    if (r->x >= max_w || r->y >= max_h) {
        r->w = 0;
        r->h = 0;
        return 0;
    }

    x1 = (uint32_t)r->x + (uint32_t)r->w;
    y1 = (uint32_t)r->y + (uint32_t)r->h;
    if (x1 > max_w) {
        r->w = (uint16_t)(max_w - r->x);
    }
    if (y1 > max_h) {
        r->h = (uint16_t)(max_h - r->y);
    }
    return (r->w > 0u && r->h > 0u) ? 1 : 0;
}

uint16_t disp_rgb565(uint8_t r, uint8_t g, uint8_t b)
{
    return (uint16_t)(((r & 0xF8u) << 8) | ((g & 0xFCu) << 3) | (b >> 3));
}
