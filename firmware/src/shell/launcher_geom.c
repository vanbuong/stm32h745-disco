#include "ui/launcher.h"
#include "ui/theme.h"

#include <stddef.h>

void launcher_tile_rect_in(unsigned index, uint16_t content_h, ui_rect_t *out)
{
    unsigned col;
    unsigned row;
    uint16_t grid_w;
    uint16_t grid_h;
    uint16_t ox;
    uint16_t oy;
    uint16_t step_x;
    uint16_t step_y;
    uint16_t tile_h;
    uint16_t gutter;
    uint16_t greet;
    uint16_t inner;

    if (out == NULL) {
        return;
    }
    out->w = 0u;
    out->h = 0u;
    out->x = 0u;
    out->y = 0u;
    if (index >= LAUNCHER_COUNT) {
        return;
    }
    if (content_h < LAUNCHER_GREET_H + THEME_HIT_MIN_PX) {
        content_h = (uint16_t)(LAUNCHER_GREET_H + THEME_HIT_MIN_PX);
    }

    tile_h = LAUNCHER_TILE_H;
    gutter = LAUNCHER_GUTTER_PX;
    greet = LAUNCHER_GREET_H;
    inner = (uint16_t)(content_h - greet);
    if ((uint16_t)(LAUNCHER_ROWS * tile_h + (LAUNCHER_ROWS - 1u) * gutter) > inner) {
        uint16_t gaps = (uint16_t)((LAUNCHER_ROWS - 1u) * gutter);
        uint16_t room = (inner > gaps) ? (uint16_t)(inner - gaps) : inner;

        tile_h = (uint16_t)(room / LAUNCHER_ROWS);
        if (tile_h < THEME_HIT_MIN_PX) {
            tile_h = THEME_HIT_MIN_PX;
        }
    }

    step_x = (uint16_t)(LAUNCHER_TILE_PX + LAUNCHER_GUTTER_PX);
    step_y = (uint16_t)(tile_h + gutter);
    grid_w =
        (uint16_t)(LAUNCHER_COLS * LAUNCHER_TILE_PX + (LAUNCHER_COLS - 1u) * LAUNCHER_GUTTER_PX);
    grid_h = (uint16_t)(LAUNCHER_ROWS * tile_h + (LAUNCHER_ROWS - 1u) * gutter);
    ox = (uint16_t)((THEME_PANEL_W - grid_w) / 2u);
    oy = (uint16_t)(THEME_STATUS_H + greet);
    if (greet + grid_h < content_h) {
        oy = (uint16_t)(THEME_STATUS_H + greet + (content_h - greet - grid_h) / 2u);
    }
    col = index % LAUNCHER_COLS;
    row = index / LAUNCHER_COLS;
    out->x = (uint16_t)(ox + col * step_x);
    out->y = (uint16_t)(oy + row * step_y);
    out->w = LAUNCHER_TILE_PX;
    out->h = tile_h;
}

void launcher_tile_rect(unsigned index, ui_rect_t *out)
{
    launcher_tile_rect_in(index, THEME_CONTENT_H, out);
}

int launcher_hit_in(int16_t x, int16_t y, uint16_t content_h)
{
    unsigned i;
    ui_rect_t r;

    if (x < 0 || y < (int16_t)THEME_STATUS_H) {
        return -1;
    }
    for (i = 0u; i < LAUNCHER_COUNT; i++) {
        launcher_tile_rect_in(i, content_h, &r);
        if ((uint16_t)x >= r.x && (uint16_t)x < (uint16_t)(r.x + r.w) && (uint16_t)y >= r.y &&
            (uint16_t)y < (uint16_t)(r.y + r.h)) {
            return (int)i;
        }
    }
    return -1;
}

int launcher_hit(int16_t x, int16_t y)
{
    return launcher_hit_in(x, y, THEME_CONTENT_H);
}
