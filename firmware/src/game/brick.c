#include "game/game_sim.h"

#include "game/gfx.h"

#include <stddef.h>
#include <string.h>

#define Q 8
#define QONE (1 << Q)
#define BALL_S 6
#define PADDLE_H 8
#define GAP 4
#define DRAG_MIN 40
#define DRAG_PREF 56
#define SCORE_BRICK 10u
#define SPEED_X 90
#define SPEED_Y (-120)

static uint16_t rgb565(uint32_t rgb)
{
    return (uint16_t)(((rgb >> 8) & 0xF800u) | ((rgb >> 5) & 0x07E0u) | ((rgb >> 3) & 0x001Fu));
}

static int16_t q8_px(int32_t v)
{
    return (int16_t)(v >> Q);
}

static int32_t px_q8(int16_t v)
{
    return ((int32_t)v) << Q;
}

static int clampi(int v, int lo, int hi)
{
    if (v < lo) {
        return lo;
    }
    if (v > hi) {
        return hi;
    }
    return v;
}

static uint8_t rows_for_h(uint16_t h)
{
    if (h >= 180u) {
        return GAME_BRICK_ROWS;
    }
    if (h >= 150u) {
        return 4u;
    }
    return 3u;
}

static void layout(game_t *g)
{
    uint16_t inner;
    uint16_t pw;

    if (g == NULL) {
        return;
    }
    if (g->w < 160u) {
        g->w = 160u;
    }
    if (g->h < 100u) {
        g->h = 100u;
    }
    if (g->w > GAME_FIELD_MAX_W) {
        g->w = GAME_FIELD_MAX_W;
    }
    if (g->h > GAME_FIELD_MAX_H) {
        g->h = GAME_FIELD_MAX_H;
    }
    if (g->rows == 0u || g->rows > GAME_BRICK_ROWS) {
        g->rows = rows_for_h(g->h);
    }
    inner = (uint16_t)(g->w - (uint16_t)(GAP * (GAME_BRICK_COLS + 1u)));
    g->brick_w = (uint16_t)(inner / GAME_BRICK_COLS);
    g->brick_h = 12u;
    if ((uint16_t)(g->rows * (g->brick_h + GAP) + 40u) > g->h) {
        g->brick_h = 10u;
    }
    pw = 64u;
    if (pw + 8u > g->w) {
        pw = (uint16_t)(g->w / 3u);
    }
    if (pw < 40u) {
        pw = 40u;
    }
    g->paddle_w = pw;
    g->paddle_y = (uint16_t)(g->h - 8u - PADDLE_H);
}

static void clamp_paddle(game_t *g)
{
    int max_x;

    if (g == NULL) {
        return;
    }
    max_x = (int)g->w - (int)g->paddle_w;
    if (max_x < 0) {
        max_x = 0;
    }
    if (q8_px(g->paddle_x) < 0) {
        g->paddle_x = 0;
    }
    if (q8_px(g->paddle_x) > max_x) {
        g->paddle_x = px_q8((int16_t)max_x);
    }
}

static void serve(game_t *g)
{
    int16_t px;
    int16_t by;

    if (g == NULL) {
        return;
    }
    px = q8_px(g->paddle_x);
    g->ball_x = px_q8((int16_t)(px + ((int)g->paddle_w - BALL_S) / 2));
    by = (int16_t)((int)g->paddle_y - BALL_S - 2);
    if (by < 8) {
        by = 8;
    }
    g->ball_y = px_q8(by);
    g->ball_vx = px_q8(SPEED_X);
    g->ball_vy = px_q8(SPEED_Y);
    g->served = 1u;
}

static void fill_bricks(game_t *g)
{
    unsigned i;

    if (g == NULL) {
        return;
    }
    memset(g->bricks, 0, sizeof(g->bricks));
    for (i = 0u; i < (unsigned)g->rows * GAME_BRICK_COLS; i++) {
        g->bricks[i] = 1u;
    }
}

static unsigned brick_count(const game_t *g)
{
    unsigned i;
    unsigned n = 0u;

    if (g == NULL) {
        return 0u;
    }
    for (i = 0u; i < GAME_BRICK_MAX; i++) {
        if (g->bricks[i] != 0u) {
            n++;
        }
    }
    return n;
}

static int overlap(int ax, int ay, int aw, int ah, int bx, int by, int bw, int bh)
{
    return ax < bx + bw && ax + aw > bx && ay < by + bh && ay + ah > by;
}

static void brick_xy(const game_t *g, unsigned index, int16_t *x, int16_t *y)
{
    unsigned col = index % GAME_BRICK_COLS;
    unsigned row = index / GAME_BRICK_COLS;

    if (x != NULL) {
        *x = (int16_t)(GAP + col * (g->brick_w + GAP));
    }
    if (y != NULL) {
        *y = (int16_t)(GAP + row * (g->brick_h + GAP));
    }
}

static void bounce_brick(game_t *g, int bx, int by, int brx, int bry, int brw, int brh)
{
    int px1 = (bx + BALL_S) - brx;
    int px2 = (brx + brw) - bx;
    int py1 = (by + BALL_S) - bry;
    int py2 = (bry + brh) - by;
    int px = (px1 < px2) ? px1 : px2;
    int py = (py1 < py2) ? py1 : py2;

    if (px < py) {
        g->ball_vx = -g->ball_vx;
    } else {
        g->ball_vy = -g->ball_vy;
    }
}

static void hit_bricks(game_t *g, int bx, int by)
{
    unsigned i;
    unsigned n;

    n = (unsigned)g->rows * GAME_BRICK_COLS;
    for (i = 0u; i < n; i++) {
        int16_t rx;
        int16_t ry;

        if (g->bricks[i] == 0u) {
            continue;
        }
        brick_xy(g, i, &rx, &ry);
        if (overlap(bx, by, BALL_S, BALL_S, rx, ry, (int)g->brick_w, (int)g->brick_h) == 0) {
            continue;
        }
        g->bricks[i] = 0u;
        g->score += SCORE_BRICK;
        if (g->score > g->high) {
            g->high = g->score;
        }
        bounce_brick(g, bx, by, rx, ry, (int)g->brick_w, (int)g->brick_h);
        if (brick_count(g) == 0u) {
            fill_bricks(g);
            if (g->ball_vy > 0) {
                g->ball_vy += g->ball_vy / 8;
            } else {
                g->ball_vy -= (-g->ball_vy) / 8;
            }
            if (g->ball_vx > 0) {
                g->ball_vx += g->ball_vx / 8;
            } else {
                g->ball_vx -= (-g->ball_vx) / 8;
            }
        }
        return;
    }
}

static void hit_paddle(game_t *g, int bx, int by)
{
    int px = q8_px(g->paddle_x);
    int py = (int)g->paddle_y;
    int hit;
    int mid;

    if (g->ball_vy <= 0) {
        return;
    }
    if (overlap(bx, by, BALL_S, BALL_S, px, py, (int)g->paddle_w, PADDLE_H) == 0) {
        return;
    }
    g->ball_y = px_q8((int16_t)(py - BALL_S - 1));
    g->ball_vy = -((g->ball_vy < 0) ? -g->ball_vy : g->ball_vy);
    mid = px + (int)g->paddle_w / 2;
    hit = (bx + BALL_S / 2) - mid;
    g->ball_vx += px_q8((int16_t)clampi(hit / 2, -40, 40));
    if (g->ball_vx > px_q8(220)) {
        g->ball_vx = px_q8(220);
    }
    if (g->ball_vx < px_q8(-220)) {
        g->ball_vx = px_q8(-220);
    }
}

static void miss(game_t *g)
{
    if (g->lives > 0u) {
        g->lives--;
    }
    if (g->lives == 0u) {
        g->phase = (uint8_t)GAME_PHASE_OVER;
        if (g->score > g->high) {
            g->high = g->score;
        }
        return;
    }
    serve(g);
}

static void brick_reset(game_t *g, uint16_t w, uint16_t h)
{
    uint32_t high;

    if (g == NULL) {
        return;
    }
    high = g->high;
    memset(g, 0, sizeof(*g));
    g->w = w;
    g->h = h;
    g->high = high;
    g->lives = GAME_LIVES_MAX;
    g->phase = (uint8_t)GAME_PHASE_PLAY;
    g->rows = rows_for_h(h);
    layout(g);
    g->paddle_x = px_q8((int16_t)(((int)g->w - (int)g->paddle_w) / 2));
    fill_bricks(g);
    serve(g);
}

static void brick_input(game_t *g, const input_event_t *e)
{
    int drag;
    int x;
    int max_x;

    if (g == NULL || e == NULL || g->phase != (uint8_t)GAME_PHASE_PLAY) {
        return;
    }
    if (e->kind != INPUT_PTR_DOWN && e->kind != INPUT_PTR_MOVE) {
        return;
    }
    drag = (g->h >= 120u) ? DRAG_PREF : DRAG_MIN;
    if ((int)e->y < (int)g->h - drag) {
        return;
    }
    max_x = (int)g->w - (int)g->paddle_w;
    if (max_x < 0) {
        max_x = 0;
    }
    x = (int)e->x - (int)g->paddle_w / 2;
    x = clampi(x, 0, max_x);
    g->paddle_x = px_q8((int16_t)x);
}

static void brick_tick(game_t *g, uint32_t dt_ms)
{
    int32_t nx;
    int32_t ny;
    int bx;
    int by;
    int max_x;

    if (g == NULL || g->phase != (uint8_t)GAME_PHASE_PLAY) {
        return;
    }
    if (dt_ms == 0u) {
        return;
    }
    if (dt_ms > 32u) {
        dt_ms = 32u;
    }
    nx = g->ball_x + (g->ball_vx * (int32_t)dt_ms) / 1000;
    ny = g->ball_y + (g->ball_vy * (int32_t)dt_ms) / 1000;
    max_x = (int)g->w - BALL_S;
    if (max_x < 0) {
        max_x = 0;
    }
    bx = q8_px(nx);
    by = q8_px(ny);
    if (bx < 0) {
        bx = 0;
        g->ball_vx = (g->ball_vx < 0) ? -g->ball_vx : g->ball_vx;
        nx = px_q8((int16_t)bx);
    } else if (bx > max_x) {
        bx = max_x;
        g->ball_vx = (g->ball_vx > 0) ? -g->ball_vx : g->ball_vx;
        nx = px_q8((int16_t)bx);
    }
    if (by < 0) {
        by = 0;
        g->ball_vy = (g->ball_vy < 0) ? -g->ball_vy : g->ball_vy;
        ny = px_q8((int16_t)by);
    }
    g->ball_x = nx;
    g->ball_y = ny;
    hit_bricks(g, bx, by);
    hit_paddle(g, q8_px(g->ball_x), q8_px(g->ball_y));
    if (q8_px(g->ball_y) + BALL_S > (int)g->h) {
        miss(g);
    }
}

static void brick_draw(const game_t *g, struct gfx *fx)
{
    static const uint16_t k_row[GAME_BRICK_ROWS] = {
        0xFBE0u, /* orange */
        0xF99Eu, /* pink */
        0x07E0u, /* green */
        0x3C7Fu, /* blue */
        0xA81Fu, /* purple */
    };
    static uint16_t ball_px[BALL_S * BALL_S];
    static uint8_t ball_ready;
    gfx_sprite_t spr;
    unsigned i;
    unsigned n;
    gfx_rect_t r;

    if (g == NULL || fx == NULL) {
        return;
    }
    gfx_clear(fx, rgb565(0xF4F6FAu));
    n = (unsigned)g->rows * GAME_BRICK_COLS;
    for (i = 0u; i < n; i++) {
        int16_t x;
        int16_t y;

        if (g->bricks[i] == 0u) {
            continue;
        }
        brick_xy(g, i, &x, &y);
        r.x = x;
        r.y = y;
        r.w = g->brick_w;
        r.h = g->brick_h;
        gfx_fill(fx, r, k_row[i / GAME_BRICK_COLS]);
    }
    r.x = q8_px(g->paddle_x);
    r.y = (int16_t)g->paddle_y;
    r.w = g->paddle_w;
    r.h = PADDLE_H;
    gfx_fill(fx, r, rgb565(0x3D8BFFu));
    if (ball_ready == 0u) {
        int y;
        int x;
        uint16_t c = rgb565(0x1A2030u);

        for (y = 0; y < BALL_S; y++) {
            for (x = 0; x < BALL_S; x++) {
                int dx = x - 2;
                int dy = y - 2;

                ball_px[y * BALL_S + x] = (dx * dx + dy * dy <= 8) ? c : 0u;
            }
        }
        ball_ready = 1u;
    }
    spr.pixels = ball_px;
    spr.w = BALL_S;
    spr.h = BALL_S;
    gfx_blit(fx, q8_px(g->ball_x), q8_px(g->ball_y), &spr);
}

static const game_module_t g_brick = {
    "brick", brick_reset, brick_input, brick_tick, brick_draw, NULL,
};

const game_module_t *game_brick_module(void)
{
    return &g_brick;
}

game_phase_t game_get_phase(const game_t *g)
{
    return (g != NULL) ? (game_phase_t)g->phase : GAME_PHASE_OVER;
}

uint32_t game_get_score(const game_t *g)
{
    return (g != NULL) ? g->score : 0u;
}

uint32_t game_get_high(const game_t *g)
{
    return (g != NULL) ? g->high : 0u;
}

void game_set_high(game_t *g, uint32_t high)
{
    if (g != NULL) {
        g->high = high;
    }
}

uint8_t game_get_lives(const game_t *g)
{
    return (g != NULL) ? g->lives : 0u;
}

int16_t game_get_ball_x(const game_t *g)
{
    return (g != NULL) ? q8_px(g->ball_x) : 0;
}

int16_t game_get_ball_y(const game_t *g)
{
    return (g != NULL) ? q8_px(g->ball_y) : 0;
}

int16_t game_get_paddle_x(const game_t *g)
{
    return (g != NULL) ? q8_px(g->paddle_x) : 0;
}

uint8_t game_brick_alive(const game_t *g, unsigned index)
{
    if (g == NULL || index >= GAME_BRICK_MAX) {
        return 0u;
    }
    return g->bricks[index];
}

void game_brick_rect(const game_t *g, unsigned index, int16_t *x, int16_t *y, uint16_t *w,
                     uint16_t *h)
{
    if (x != NULL) {
        *x = 0;
    }
    if (y != NULL) {
        *y = 0;
    }
    if (w != NULL) {
        *w = 0u;
    }
    if (h != NULL) {
        *h = 0u;
    }
    if (g == NULL || index >= GAME_BRICK_MAX) {
        return;
    }
    brick_xy(g, index, x, y);
    if (w != NULL) {
        *w = g->brick_w;
    }
    if (h != NULL) {
        *h = g->brick_h;
    }
}

void game_pause_sim(game_t *g)
{
    if (g != NULL && g->phase == (uint8_t)GAME_PHASE_PLAY) {
        g->phase = (uint8_t)GAME_PHASE_PAUSE;
    }
}

void game_resume_sim(game_t *g)
{
    if (g != NULL && g->phase == (uint8_t)GAME_PHASE_PAUSE) {
        g->phase = (uint8_t)GAME_PHASE_PLAY;
    }
}

void game_test_set_ball(game_t *g, int16_t x, int16_t y, int16_t vx_px_s, int16_t vy_px_s)
{
    if (g == NULL) {
        return;
    }
    g->ball_x = px_q8(x);
    g->ball_y = px_q8(y);
    g->ball_vx = px_q8(vx_px_s);
    g->ball_vy = px_q8(vy_px_s);
    g->served = 1u;
}

void game_test_set_brick(game_t *g, unsigned index, uint8_t alive)
{
    if (g == NULL || index >= GAME_BRICK_MAX) {
        return;
    }
    g->bricks[index] = (alive != 0u) ? 1u : 0u;
}

void game_test_clear_bricks(game_t *g)
{
    if (g != NULL) {
        memset(g->bricks, 0, sizeof(g->bricks));
    }
}

void game_resize_layout(game_t *g, uint16_t w, uint16_t h)
{
    int16_t bx;
    int16_t by;
    int16_t px;

    if (g == NULL) {
        return;
    }
    bx = q8_px(g->ball_x);
    by = q8_px(g->ball_y);
    px = q8_px(g->paddle_x);
    g->w = w;
    g->h = h;
    layout(g);
    g->paddle_x = px_q8(px);
    clamp_paddle(g);
    if (bx < 0) {
        bx = 0;
    }
    if (by < 0) {
        by = 0;
    }
    if (bx + BALL_S > (int)g->w) {
        bx = (int16_t)((int)g->w - BALL_S);
    }
    if (by + BALL_S > (int)g->h) {
        by = (int16_t)((int)g->h - BALL_S - 2);
    }
    g->ball_x = px_q8(bx);
    g->ball_y = px_q8(by);
}
