#include "ui/launcher.h"
#include "ui/theme.h"

#include <stddef.h>

void launcher_tile_rect(unsigned index, ui_rect_t *out)
{
    unsigned col;
    unsigned row;
    uint16_t grid_w;
    uint16_t grid_h;
    uint16_t ox;
    uint16_t oy;
    uint16_t step;

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

    step = (uint16_t)(LAUNCHER_TILE_PX + LAUNCHER_GUTTER_PX);
    grid_w =
        (uint16_t)(LAUNCHER_COLS * LAUNCHER_TILE_PX + (LAUNCHER_COLS - 1u) * LAUNCHER_GUTTER_PX);
    grid_h =
        (uint16_t)(LAUNCHER_ROWS * LAUNCHER_TILE_PX + (LAUNCHER_ROWS - 1u) * LAUNCHER_GUTTER_PX);
    ox = (uint16_t)((THEME_PANEL_W - grid_w) / 2u);
    oy = (uint16_t)(THEME_STATUS_H + (THEME_CONTENT_H - grid_h) / 2u);
    col = index % LAUNCHER_COLS;
    row = index / LAUNCHER_COLS;
    out->x = (uint16_t)(ox + col * step);
    out->y = (uint16_t)(oy + row * step);
    out->w = LAUNCHER_TILE_PX;
    out->h = LAUNCHER_TILE_PX;
}

int launcher_hit(int16_t x, int16_t y)
{
    unsigned i;
    ui_rect_t r;

    if (x < 0 || y < (int16_t)THEME_STATUS_H) {
        return -1;
    }
    for (i = 0u; i < LAUNCHER_COUNT; i++) {
        launcher_tile_rect(i, &r);
        if ((uint16_t)x >= r.x && (uint16_t)x < (uint16_t)(r.x + r.w) && (uint16_t)y >= r.y &&
            (uint16_t)y < (uint16_t)(r.y + r.h)) {
            return (int)i;
        }
    }
    return -1;
}
