#ifndef LAUNCHER_H
#define LAUNCHER_H

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

#define LAUNCHER_COLS 3
#define LAUNCHER_ROWS 2
#define LAUNCHER_COUNT (LAUNCHER_COLS * LAUNCHER_ROWS)
#define LAUNCHER_TILE_PX 72
#define LAUNCHER_GUTTER_PX 16

typedef struct {
    uint16_t x;
    uint16_t y;
    uint16_t w;
    uint16_t h;
} ui_rect_t;

void launcher_tile_rect(unsigned index, ui_rect_t *out);
int launcher_hit(int16_t x, int16_t y);

#ifdef __cplusplus
}
#endif

#endif /* LAUNCHER_H */
