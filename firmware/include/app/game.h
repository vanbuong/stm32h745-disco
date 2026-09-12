#ifndef APP_GAME_H
#define APP_GAME_H

#include "game/game_sim.h"
#include "hal/input.h"

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

#define GAME_SAVE_PATH "/user/game/brick.sav"

void game_open(uint16_t w, uint16_t h);
void game_close(void);
void game_resize(uint16_t w, uint16_t h);
void game_step(uint32_t dt_ms);
void game_pointer(input_kind_t kind, int16_t x, int16_t y);
void game_pause(void);
void game_resume(void);
void game_new(void);
uint8_t game_on_back(void);

game_phase_t game_phase(void);
uint32_t game_score(void);
uint32_t game_high(void);
uint8_t game_lives(void);
uint32_t game_gen(void);
const uint16_t *game_pixels(void);
uint16_t game_field_w(void);
uint16_t game_field_h(void);
uint16_t game_field_stride(void);
const char *game_score_str(void);
const char *game_high_str(void);
const char *game_lives_str(void);
game_t *game_self(void);
const game_module_t *game_module(void);

#ifdef __cplusplus
}
#endif

#endif /* APP_GAME_H */
