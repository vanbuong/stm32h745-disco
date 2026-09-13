#include "app/game.h"

#include "game/gfx.h"
#include "svc/media.h"
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
static uint8_t g_lib;
static uint16_t g_w;
static uint16_t g_h;
static char g_score[12];
static char g_high[12];
static char g_lives[4];
static char g_title[32];
static game_title_t g_titles[GAME_TITLES_MAX];
static unsigned g_tn;

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

static int ascii_eq_ci(const char *a, const char *b)
{
    while (*a != '\0' && *b != '\0') {
        char ca = *a++;
        char cb = *b++;
        if (ca >= 'A' && ca <= 'Z') {
            ca = (char)(ca - 'A' + 'a');
        }
        if (cb >= 'A' && cb <= 'Z') {
            cb = (char)(cb - 'A' + 'a');
        }
        if (ca != cb) {
            return 0;
        }
    }
    return *a == '\0' && *b == '\0';
}

static int is_cart_name(const char *name)
{
    const char *dot;

    if (name == NULL) {
        return 0;
    }
    dot = strrchr(name, '.');
    if (dot == NULL || dot[1] == '\0') {
        return 0;
    }
    dot++;
    return ascii_eq_ci(dot, "ch8") || ascii_eq_ci(dot, "c8");
}

static void stem_name(char *out, size_t n, const char *name)
{
    size_t i = 0u;

    if (out == NULL || n == 0u) {
        return;
    }
    if (name == NULL) {
        out[0] = '\0';
        return;
    }
    while (name[i] != '\0' && name[i] != '.' && i + 1u < n) {
        out[i] = name[i];
        i++;
    }
    if (i == 0u) {
        strncpy(out, name, n - 1u);
        out[n - 1u] = '\0';
        return;
    }
    out[i] = '\0';
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
    if (g_mod == NULL || g_mod->id == NULL || strcmp(g_mod->id, "brick") != 0) {
        return;
    }
    e = vfs_mkdir(GAME_DIR);
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

    if (g_lib != 0u || g_mod == NULL || g_mod->draw == NULL) {
        refresh_str();
        return;
    }
    fx.fb = dst_px();
    fx.w = g_game.w;
    fx.h = g_game.h;
    fx.stride = g_game.w;
    g_mod->draw(&g_game, &fx);
    refresh_str();
}

static void clamp_size(uint16_t *w, uint16_t *h)
{
    if (w == NULL || h == NULL) {
        return;
    }
    if (*w < 160u) {
        *w = GAME_FIELD_MAX_W;
    }
    if (*h < 80u) {
        *h = 200u;
    }
    if (*w > GAME_FIELD_MAX_W) {
        *w = GAME_FIELD_MAX_W;
    }
    if (*h > GAME_FIELD_MAX_H) {
        *h = GAME_FIELD_MAX_H;
    }
}

static void apply_size(uint16_t w, uint16_t h)
{
    clamp_size(&w, &h);
    g_w = w;
    g_h = h;
    if (g_lib != 0u) {
        g_game.w = w;
        g_game.h = h;
        return;
    }
    if (g_mod == game_brick_module()) {
        if (g_open == 0u) {
            if (g_mod->reset != NULL) {
                g_mod->reset(&g_game, w, h);
            }
        } else {
            game_resize_layout(&g_game, w, h);
        }
    } else {
        g_game.w = w;
        g_game.h = h;
    }
}

static void add_title(const char *name, const char *path, const char *core, uint8_t builtin)
{
    game_title_t *t;

    if (g_tn >= GAME_TITLES_MAX || name == NULL || core == NULL) {
        return;
    }
    t = &g_titles[g_tn++];
    memset(t, 0, sizeof(*t));
    strncpy(t->name, name, sizeof(t->name) - 1u);
    if (path != NULL) {
        strncpy(t->path, path, sizeof(t->path) - 1u);
    }
    t->core = core;
    t->builtin = builtin;
}

static void reload_titles(void)
{
    vfs_dir_t d = -1;
    vfs_dirent_t ent;

    g_tn = 0u;
    add_title("Brick", "", "brick", 1u);
    add_title("CHIP-8 Demo", "", "chip8", 1u);
    if (vfs_mounted() == 0) {
        return;
    }
    (void)vfs_mkdir(GAME_DIR);
    if (vfs_opendir(GAME_DIR, &d) != ERR_OK) {
        return;
    }
    for (;;) {
        char path[VFS_PATH_MAX];
        char label[64];

        if (vfs_readdir(d, &ent) != ERR_OK) {
            break;
        }
        if (ent.is_dir != 0u || !is_cart_name(ent.name)) {
            continue;
        }
        if (vfs_path_join(GAME_DIR, ent.name, path, sizeof(path)) != ERR_OK) {
            continue;
        }
        stem_name(label, sizeof(label), ent.name);
        add_title(label, path, "chip8", 0u);
    }
    (void)vfs_closedir(d);
}

static void start_core(const char *core, const uint8_t *rom, uint32_t n)
{
    uint32_t high = g_game.high;

    g_mod = game_module_by_id(core);
    if (g_mod == NULL) {
        g_mod = game_brick_module();
    }
    memset(&g_game, 0, sizeof(g_game));
    g_game.high = high;
    g_lib = 0u;
    g_open = 0u;
    apply_size(g_w, g_h);
    if (g_mod->load != NULL) {
        if (rom != NULL && n > 0u) {
            (void)g_mod->load(&g_game, rom, n);
        }
        if (g_mod->reset != NULL) {
            g_mod->reset(&g_game, g_w, g_h);
        }
    } else if (g_mod->reset != NULL) {
        g_mod->reset(&g_game, g_w, g_h);
    }
    g_game.high = high;
    g_open = 1u;
    strncpy(g_title, (g_mod->id != NULL && strcmp(g_mod->id, "chip8") == 0) ? "CHIP-8" : "Brick",
            sizeof(g_title) - 1u);
    if (g_mod->id != NULL && strcmp(g_mod->id, "chip8") == 0 && g_mod->tick != NULL) {
        g_mod->tick(&g_game, 16u);
    }
    present();
    bump();
}

void game_open(uint16_t w, uint16_t h)
{
    g_mod = game_brick_module();
    memset(&g_game, 0, sizeof(g_game));
    g_game.high = load_high();
    g_open = 1u;
    g_lib = 1u;
    strncpy(g_title, "Games", sizeof(g_title) - 1u);
    apply_size(w, h);
    reload_titles();
    refresh_str();
    bump();
}

void game_close(void)
{
    if (g_open != 0u && g_lib == 0u) {
        if (g_game.score > g_game.high) {
            g_game.high = g_game.score;
        }
        save_high(g_game.high);
    }
    g_open = 0u;
    g_lib = 1u;
}

void game_resize(uint16_t w, uint16_t h)
{
    if (g_open == 0u) {
        game_open(w, h);
        return;
    }
    clamp_size(&w, &h);
    if (w == g_w && h == g_h && w == g_game.w && h == g_game.h) {
        return;
    }
    apply_size(w, h);
    present();
}

void game_step(uint32_t dt_ms)
{
    game_phase_t before;

    if (g_open == 0u || g_lib != 0u || g_mod == NULL || g_mod->tick == NULL) {
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

    if (g_open == 0u || g_lib != 0u || g_mod == NULL || g_mod->input == NULL) {
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
    if (g_open == 0u || g_lib != 0u) {
        return;
    }
    if (g_game.phase == (uint8_t)GAME_PHASE_PLAY) {
        game_pause_sim(&g_game);
        bump();
    }
}

void game_resume(void)
{
    if (g_open == 0u || g_lib != 0u) {
        return;
    }
    if (g_game.phase == (uint8_t)GAME_PHASE_PAUSE) {
        game_resume_sim(&g_game);
        bump();
    }
}

void game_new(void)
{
    uint32_t high;

    if (g_open == 0u || g_lib != 0u || g_mod == NULL || g_mod->reset == NULL) {
        return;
    }
    high = g_game.high;
    g_mod->reset(&g_game, g_w, g_h);
    g_game.high = high;
    if (g_mod->id != NULL && strcmp(g_mod->id, "chip8") == 0 && g_mod->tick != NULL) {
        g_mod->tick(&g_game, 16u);
    }
    present();
    bump();
}

void game_to_library(void)
{
    if (g_open == 0u) {
        return;
    }
    if (g_lib == 0u) {
        if (g_game.score > g_game.high) {
            g_game.high = g_game.score;
        }
        save_high(g_game.high);
    }
    g_lib = 1u;
    strncpy(g_title, "Games", sizeof(g_title) - 1u);
    reload_titles();
    bump();
}

uint8_t game_on_back(void)
{
    if (g_open == 0u) {
        return 0u;
    }
    if (g_lib == 0u && g_game.phase == (uint8_t)GAME_PHASE_PLAY) {
        game_pause();
        return 1u;
    }
    if (g_lib == 0u) {
        game_to_library();
        return 1u;
    }
    return 0u;
}

err_t game_load_path(const char *path)
{
    char norm[VFS_PATH_MAX];
    vfs_file_t fd = -1;
    uint8_t rom[CHIP8_ROM_MAX];
    size_t got = 0u;
    err_t e;

    if (path == NULL || path[0] == '\0') {
        return ERR_INVAL;
    }
    e = vfs_normalize(path, norm, sizeof(norm));
    if (e != ERR_OK) {
        return e;
    }
    if (vfs_in_user_jail(norm) == 0) {
        return ERR_DENIED;
    }
    if (!is_cart_name(norm)) {
        return ERR_UNSUPPORTED;
    }
    if (vfs_open(norm, VFS_O_RD, &fd) != ERR_OK) {
        return ERR_NOENT;
    }
    e = vfs_read(fd, rom, sizeof(rom), &got);
    (void)vfs_close(fd);
    if (e != ERR_OK || got == 0u) {
        return (e != ERR_OK) ? e : ERR_CORRUPT;
    }
    start_core("chip8", rom, (uint32_t)got);
    stem_name(g_title, sizeof(g_title), strrchr(norm, '/') ? strrchr(norm, '/') + 1 : norm);
    return ERR_OK;
}

void game_pick(unsigned index)
{
    const game_title_t *t = game_title_at(index);

    if (t == NULL) {
        return;
    }
    if (t->builtin != 0u && strcmp(t->core, "brick") == 0) {
        start_core("brick", NULL, 0u);
        strncpy(g_title, "Brick", sizeof(g_title) - 1u);
        return;
    }
    if (t->builtin != 0u && strcmp(t->core, "chip8") == 0) {
        uint32_t n = 0u;
        const uint8_t *rom = chip8_demo_rom(&n);
        start_core("chip8", rom, n);
        strncpy(g_title, "CHIP-8 Demo", sizeof(g_title) - 1u);
        return;
    }
    (void)game_load_path(t->path);
}

uint8_t game_in_library(void)
{
    return g_lib;
}

unsigned game_title_count(void)
{
    return g_tn;
}

const game_title_t *game_title_at(unsigned index)
{
    if (index >= g_tn) {
        return NULL;
    }
    return &g_titles[index];
}

const char *game_title(void)
{
    return g_title;
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
