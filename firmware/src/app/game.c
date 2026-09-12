#include "app/game.h"

#include "game/gfx.h"
#include "svc/vfs.h"

#include <stddef.h>
#include <string.h>

#if defined(STM32H745xx)
#include "bsp/board.h"
#define GAME_DST_BASE (BOARD_SDRAM_BASE + 0x000C0000u)
static uint16_t *dst_px(void)
{
    return (uint16_t *)GAME_DST_BASE;
}
#else
static uint16_t s_px[GAME_FIELD_MAX_W * GAME_FIELD_MAX_H];
static uint16_t *dst_px(void)
{
    return s_px;
}
#endif

static game_t g_game;
static const game_module_t *g_mod;
static uint32_t g_gen;
static uint8_t g_open;
static char g_score[12];
static char g_high[12];
static char g_lives[4];

static void bump(void)
{
    g_gen++;
}

static void put_u32(char *out, size_t n, uint32_t v)
{
    char tmp[11];
    int i = 10;
    size_t o = 0u;

    if (out == NULL || n == 0u) {
        return;
    }
    tmp[10] = '\0';
    if (v == 0u) {
        tmp[--i] = '0';
    }
    while (v > 0u && i > 0) {
        tmp[--i] = (char)('0' + (v % 10u));
        v /= 10u;
    }
    while (tmp[i] != '\0' && o + 1u < n) {
        out[o++] = tmp[i++];
    }
    out[o] = '\0';
}

static uint32_t parse_u32(const char *s, size_t n)
{
    uint32_t v = 0u;
    size_t i;

    if (s == NULL) {
        return 0u;
    }
    for (i = 0u; i < n; i++) {
        if (s[i] < '0' || s[i] > '9') {
            break;
        }
        v = v * 10u + (uint32_t)(s[i] - '0');
    }
    return v;
}

static uint32_t load_high(void)
{
    vfs_file_t fd = -1;
    char buf[16];
    size_t got = 0u;

    if (vfs_mounted() == 0) {
        return 0u;
    }
    if (vfs_open(GAME_SAVE_PATH, VFS_O_RD, &fd) != ERR_OK) {
        return 0u;
    }
    memset(buf, 0, sizeof(buf));
    (void)vfs_read(fd, buf, sizeof(buf) - 1u, &got);
    (void)vfs_close(fd);
    return parse_u32(buf, got);
}

static void save_high(uint32_t high)
{
    vfs_file_t fd = -1;
    char buf[12];
    size_t n;
    size_t put = 0u;
    err_t e;

    if (vfs_mounted() == 0) {
        return;
    }
    e = vfs_mkdir("/user/game");
    if (e != ERR_OK && e != ERR_DENIED) {
        return;
    }
    put_u32(buf, sizeof(buf), high);
    n = strlen(buf);
    if (n + 1u < sizeof(buf)) {
        buf[n++] = '\n';
        buf[n] = '\0';
    }
    if (vfs_open(GAME_SAVE_PATH, VFS_O_WR | VFS_O_CREAT | VFS_O_TRUNC, &fd) != ERR_OK) {
        return;
    }
    (void)vfs_write(fd, buf, n, &put);
    (void)vfs_close(fd);
}

static void refresh_str(void)
{
    put_u32(g_score, sizeof(g_score), g_game.score);
    put_u32(g_high, sizeof(g_high), g_game.high);
    g_lives[0] = (char)('0' + (g_game.lives % 10u));
    g_lives[1] = '\0';
}

static void present(void)
{
    gfx_t fx;

    if (g_mod == NULL || g_mod->draw == NULL) {
        return;
    }
    fx.fb = dst_px();
    fx.w = g_game.w;
    fx.h = g_game.h;
    fx.stride = g_game.w;
    g_mod->draw(&g_game, &fx);
    refresh_str();
}

static void apply_size(uint16_t w, uint16_t h)
{
    if (w < 160u) {
        w = GAME_FIELD_MAX_W;
    }
    if (h < 80u) {
        h = 200u;
    }
    if (w > GAME_FIELD_MAX_W) {
        w = GAME_FIELD_MAX_W;
    }
    if (h > GAME_FIELD_MAX_H) {
        h = GAME_FIELD_MAX_H;
    }
    if (g_open == 0u) {
        if (g_mod != NULL && g_mod->reset != NULL) {
            g_mod->reset(&g_game, w, h);
        }
    } else {
        game_resize_layout(&g_game, w, h);
    }
}

void game_open(uint16_t w, uint16_t h)
{
    g_mod = game_brick_module();
    memset(&g_game, 0, sizeof(g_game));
    g_game.high = load_high();
    g_open = 0u;
    apply_size(w, h);
    g_open = 1u;
    present();
    bump();
}

void game_close(void)
{
    if (g_open != 0u) {
        if (g_game.score > g_game.high) {
            g_game.high = g_game.score;
        }
        save_high(g_game.high);
    }
    g_open = 0u;
}

void game_resize(uint16_t w, uint16_t h)
{
    if (g_open == 0u) {
        game_open(w, h);
        return;
    }
    if (w == g_game.w && h == g_game.h) {
        return;
    }
    apply_size(w, h);
    present();
}

void game_step(uint32_t dt_ms)
{
    game_phase_t before;

    if (g_open == 0u || g_mod == NULL || g_mod->tick == NULL) {
        return;
    }
    before = (game_phase_t)g_game.phase;
    g_mod->tick(&g_game, dt_ms);
    present();
    if ((game_phase_t)g_game.phase != before) {
        if (g_game.phase == (uint8_t)GAME_PHASE_OVER) {
            save_high(g_game.high);
        }
        bump();
    }
}

void game_pointer(input_kind_t kind, int16_t x, int16_t y)
{
    input_event_t e;

    if (g_open == 0u || g_mod == NULL || g_mod->input == NULL) {
        return;
    }
    e.kind = kind;
    e.x = x;
    e.y = y;
    e.id = 0u;
    e.t_ms = 0u;
    g_mod->input(&g_game, &e);
}

void game_pause(void)
{
    if (g_open == 0u) {
        return;
    }
    if (g_game.phase == (uint8_t)GAME_PHASE_PLAY) {
        game_pause_sim(&g_game);
        bump();
    }
}

void game_resume(void)
{
    if (g_open == 0u) {
        return;
    }
    if (g_game.phase == (uint8_t)GAME_PHASE_PAUSE) {
        game_resume_sim(&g_game);
        bump();
    }
}

void game_new(void)
{
    uint32_t high = g_game.high;
    uint16_t w = g_game.w;
    uint16_t h = g_game.h;

    if (g_mod == NULL || g_mod->reset == NULL) {
        return;
    }
    g_game.high = high;
    g_mod->reset(&g_game, w, h);
    g_game.high = high;
    g_open = 1u;
    present();
    bump();
}

uint8_t game_on_back(void)
{
    if (g_open == 0u) {
        return 0u;
    }
    if (g_game.phase == (uint8_t)GAME_PHASE_PLAY) {
        game_pause();
        return 1u;
    }
    return 0u;
}

game_phase_t game_phase(void)
{
    return game_get_phase(&g_game);
}

uint32_t game_score(void)
{
    return g_game.score;
}

uint32_t game_high(void)
{
    return g_game.high;
}

uint8_t game_lives(void)
{
    return g_game.lives;
}

uint32_t game_gen(void)
{
    return g_gen;
}

const uint16_t *game_pixels(void)
{
    return dst_px();
}

uint16_t game_field_w(void)
{
    return g_game.w;
}

uint16_t game_field_h(void)
{
    return g_game.h;
}

uint16_t game_field_stride(void)
{
    return g_game.w;
}

const char *game_score_str(void)
{
    return g_score;
}

const char *game_high_str(void)
{
    return g_high;
}

const char *game_lives_str(void)
{
    return g_lives;
}

game_t *game_self(void)
{
    return &g_game;
}

const game_module_t *game_module(void)
{
    return (g_mod != NULL) ? g_mod : game_brick_module();
}
