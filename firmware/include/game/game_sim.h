#ifndef GAME_SIM_H
#define GAME_SIM_H

#include "err.h"
#include "hal/input.h"

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

struct gfx;

#define GAME_BRICK_COLS 8u
#define GAME_BRICK_ROWS 5u
#define GAME_BRICK_MAX (GAME_BRICK_COLS * GAME_BRICK_ROWS)
#define GAME_FIELD_MAX_W 480u
#define GAME_FIELD_MAX_H 240u
#define GAME_LIVES_MAX 3u

typedef enum { GAME_PHASE_PLAY = 0, GAME_PHASE_PAUSE, GAME_PHASE_OVER } game_phase_t;

typedef struct game {
    uint16_t w;
    uint16_t h;
    uint8_t phase;
    uint8_t lives;
    uint8_t rows;
    uint8_t served;
    uint8_t bricks[GAME_BRICK_MAX];
    uint16_t brick_w;
    uint16_t brick_h;
    uint16_t paddle_w;
    uint16_t paddle_y;
    int32_t paddle_x; /* Q8 pixels */
    int32_t ball_x;
    int32_t ball_y;
    int32_t ball_vx; /* Q8 pixels / second */
    int32_t ball_vy;
    uint32_t score;
    uint32_t high;
} game_t;

typedef struct {
    const char *id;
    void (*reset)(game_t *g, uint16_t w, uint16_t h);
    void (*input)(game_t *g, const input_event_t *e);
    void (*tick)(game_t *g, uint32_t dt_ms);
    void (*draw)(const game_t *g, struct gfx *fx);
    err_t (*load)(game_t *g, const uint8_t *rom, uint32_t n);
} game_module_t;

const game_module_t *game_brick_module(void);
const game_module_t *game_chip8_module(void);
const game_module_t *game_module_by_id(const char *id);

#define CHIP8_ROM_MAX 3584u

typedef struct {
    const char *id;
    const char *name;
    const uint8_t *bytes;
    uint32_t n;
} chip8_cart_t;

const uint8_t *chip8_demo_rom(uint32_t *n);
unsigned chip8_cart_count(void);
const chip8_cart_t *chip8_cart_at(unsigned index);
const chip8_cart_t *chip8_cart_by_id(const char *id);
uint8_t chip8_pixel(unsigned x, unsigned y);

game_phase_t game_get_phase(const game_t *g);
uint32_t game_get_score(const game_t *g);
uint32_t game_get_high(const game_t *g);
void game_set_high(game_t *g, uint32_t high);
uint8_t game_get_lives(const game_t *g);
int16_t game_get_ball_x(const game_t *g);
int16_t game_get_ball_y(const game_t *g);
int16_t game_get_paddle_x(const game_t *g);
uint8_t game_brick_alive(const game_t *g, unsigned index);
void game_brick_rect(const game_t *g, unsigned index, int16_t *x, int16_t *y, uint16_t *w,
                     uint16_t *h);
void game_pause_sim(game_t *g);
void game_resume_sim(game_t *g);
void game_resize_layout(game_t *g, uint16_t w, uint16_t h);

void game_test_set_ball(game_t *g, int16_t x, int16_t y, int16_t vx_px_s, int16_t vy_px_s);
void game_test_set_brick(game_t *g, unsigned index, uint8_t alive);
void game_test_clear_bricks(game_t *g);

#ifdef __cplusplus
}
#endif

#endif /* GAME_SIM_H */
