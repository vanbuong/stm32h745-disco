#ifndef GAME_SIM_H
#define GAME_SIM_H

#include "hal/input.h"

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

struct gfx;

typedef struct game game_t;

typedef struct {
    const char *id;
    void (*reset)(game_t *g, uint16_t w, uint16_t h);
    void (*input)(game_t *g, const input_event_t *e);
    void (*tick)(game_t *g, uint32_t dt_ms);
    void (*draw)(const game_t *g, struct gfx *fx);
} game_module_t;

#ifdef __cplusplus
}
#endif

#endif /* GAME_SIM_H */
