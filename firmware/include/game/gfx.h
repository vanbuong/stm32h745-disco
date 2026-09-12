#ifndef GFX_H
#define GFX_H

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct {
    int16_t x;
    int16_t y;
    uint16_t w;
    uint16_t h;
} gfx_rect_t;

typedef struct {
    const uint16_t *pixels;
    uint16_t w;
    uint16_t h;
} gfx_sprite_t;

typedef struct gfx {
    uint16_t *fb;
    uint16_t w;
    uint16_t h;
    uint16_t stride;
} gfx_t;

typedef struct {
    unsigned clears;
    unsigned fills;
    unsigned blits;
    gfx_rect_t last_fill;
    uint16_t last_clear;
    uint16_t last_fill_color;
} gfx_stats_t;

void gfx_clear(gfx_t *fx, uint16_t rgb565);
void gfx_fill(gfx_t *fx, gfx_rect_t r, uint16_t rgb565);
void gfx_blit(gfx_t *fx, int x, int y, const gfx_sprite_t *s);

void gfx_stats_reset(void);
const gfx_stats_t *gfx_stats(void);

#ifdef __cplusplus
}
#endif

#endif /* GFX_H */
