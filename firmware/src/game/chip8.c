#include "game/game_sim.h"

#include "game/gfx.h"

#include <string.h>

#define MEM_N 4096u
#define ROM_BASE 0x200u
#define FB_W 64u
#define FB_H 32u
#define FONT_BASE 0x50u

static uint8_t s_mem[MEM_N];
static uint8_t s_fb[FB_W * FB_H];
static uint8_t s_v[16];
static uint8_t s_key[16];
static uint16_t s_stack[16];
static uint16_t s_pc;
static uint16_t s_i;
static uint8_t s_sp;
static uint8_t s_dt;
static uint8_t s_st;
static uint8_t s_wait;
static uint32_t s_seed = 1u;
static uint32_t s_acc;
static uint8_t s_rom[CHIP8_ROM_MAX];
static uint32_t s_rom_n;
static int16_t s_pad_x;
static int16_t s_pad_y;
static uint16_t s_pad_s;

static const uint8_t k_font[80] = {
    0xF0u, 0x90u, 0x90u, 0x90u, 0xF0u, 0x20u, 0x60u, 0x20u, 0x20u, 0x70u, 0xF0u, 0x10u,
    0xF0u, 0x80u, 0xF0u, 0xF0u, 0x10u, 0xF0u, 0x10u, 0xF0u, 0x90u, 0x90u, 0xF0u, 0x10u,
    0x10u, 0xF0u, 0x80u, 0xF0u, 0x10u, 0xF0u, 0xF0u, 0x80u, 0xF0u, 0x90u, 0xF0u, 0xF0u,
    0x10u, 0x20u, 0x40u, 0x40u, 0xF0u, 0x90u, 0xF0u, 0x90u, 0xF0u, 0xF0u, 0x90u, 0xF0u,
    0x10u, 0xF0u, 0xF0u, 0x90u, 0xF0u, 0x90u, 0x90u, 0xE0u, 0x90u, 0xE0u, 0x90u, 0xE0u,
    0xF0u, 0x80u, 0x80u, 0x80u, 0xF0u, 0xE0u, 0x90u, 0x90u, 0x90u, 0xE0u, 0xF0u, 0x80u,
    0xF0u, 0x80u, 0xF0u, 0xF0u, 0x80u, 0xF0u, 0x80u, 0x80u,
};

/* Original 17-byte demo: CLS, place a 5-row glyph, sit on DRW. */
static const uint8_t k_demo[] = {0x00u, 0xE0u, 0x60u, 0x0Au, 0x61u, 0x08u, 0xA2u, 0x0Cu, 0xD0u,
                                 0x15u, 0x12u, 0x0Au, 0xF8u, 0x88u, 0x88u, 0x88u, 0xF8u};

static const uint8_t k_hex[16] = {0x1u, 0x2u, 0x3u, 0xCu, 0x4u, 0x5u, 0x6u, 0xDu,
                                  0x7u, 0x8u, 0x9u, 0xEu, 0xAu, 0x0u, 0xBu, 0xFu};

static uint16_t rgb565(uint32_t rgb)
{
    return (uint16_t)(((rgb >> 8) & 0xF800u) | ((rgb >> 5) & 0x07E0u) | ((rgb >> 3) & 0x001Fu));
}

const uint8_t *chip8_demo_rom(uint32_t *n)
{
    if (n != NULL) {
        *n = (uint32_t)sizeof(k_demo);
    }
    return k_demo;
}

uint8_t chip8_pixel(unsigned x, unsigned y)
{
    if (x >= FB_W || y >= FB_H) {
        return 0u;
    }
    return s_fb[y * FB_W + x];
}

static void hard_reset(void)
{
    memset(s_mem, 0, sizeof(s_mem));
    memset(s_fb, 0, sizeof(s_fb));
    memset(s_v, 0, sizeof(s_v));
    memset(s_key, 0, sizeof(s_key));
    memset(s_stack, 0, sizeof(s_stack));
    memcpy(s_mem + FONT_BASE, k_font, sizeof(k_font));
    s_pc = ROM_BASE;
    s_i = 0u;
    s_sp = 0u;
    s_dt = 0u;
    s_st = 0u;
    s_wait = 0u;
    s_acc = 0u;
}

static err_t chip8_load(game_t *g, const uint8_t *rom, uint32_t n)
{
    if (rom == NULL || n == 0u || n > CHIP8_ROM_MAX) {
        return ERR_INVAL;
    }
    hard_reset();
    memcpy(s_rom, rom, n);
    s_rom_n = n;
    memcpy(s_mem + ROM_BASE, rom, n);
    if (g != NULL) {
        g->phase = (uint8_t)GAME_PHASE_PLAY;
        g->score = 0u;
        g->lives = 0u;
    }
    return ERR_OK;
}

static void chip8_reset(game_t *g, uint16_t w, uint16_t h)
{
    if (g != NULL) {
        g->w = w;
        g->h = h;
        g->phase = (uint8_t)GAME_PHASE_PLAY;
    }
    if (s_rom_n == 0u) {
        (void)chip8_load(g, k_demo, (uint32_t)sizeof(k_demo));
        if (g != NULL) {
            g->w = w;
            g->h = h;
        }
        return;
    }
    (void)chip8_load(g, s_rom, s_rom_n);
    if (g != NULL) {
        g->w = w;
        g->h = h;
    }
}

static void op_step(void)
{
    uint16_t op;
    uint8_t x;
    uint8_t y;
    uint8_t n;
    uint8_t kk;
    uint16_t nnn;
    unsigned i;

    if (s_pc + 1u >= MEM_N) {
        return;
    }
    op = (uint16_t)(((uint16_t)s_mem[s_pc] << 8) | s_mem[s_pc + 1u]);
    s_pc = (uint16_t)(s_pc + 2u);
    x = (uint8_t)((op >> 8) & 0x0Fu);
    y = (uint8_t)((op >> 4) & 0x0Fu);
    n = (uint8_t)(op & 0x0Fu);
    kk = (uint8_t)(op & 0xFFu);
    nnn = (uint16_t)(op & 0x0FFFu);

    switch (op & 0xF000u) {
    case 0x0000u:
        if (op == 0x00E0u) {
            memset(s_fb, 0, sizeof(s_fb));
        } else if (op == 0x00EEu && s_sp > 0u) {
            s_sp--;
            s_pc = s_stack[s_sp];
        }
        break;
    case 0x1000u:
        s_pc = nnn;
        break;
    case 0x2000u:
        if (s_sp < 16u) {
            s_stack[s_sp++] = s_pc;
        }
        s_pc = nnn;
        break;
    case 0x3000u:
        if (s_v[x] == kk) {
            s_pc = (uint16_t)(s_pc + 2u);
        }
        break;
    case 0x4000u:
        if (s_v[x] != kk) {
            s_pc = (uint16_t)(s_pc + 2u);
        }
        break;
    case 0x5000u:
        if (n == 0u && s_v[x] == s_v[y]) {
            s_pc = (uint16_t)(s_pc + 2u);
        }
        break;
    case 0x6000u:
        s_v[x] = kk;
        break;
    case 0x7000u:
        s_v[x] = (uint8_t)(s_v[x] + kk);
        break;
    case 0x8000u:
        switch (n) {
        case 0x0u:
            s_v[x] = s_v[y];
            break;
        case 0x1u:
            s_v[x] = (uint8_t)(s_v[x] | s_v[y]);
            break;
        case 0x2u:
            s_v[x] = (uint8_t)(s_v[x] & s_v[y]);
            break;
        case 0x3u:
            s_v[x] = (uint8_t)(s_v[x] ^ s_v[y]);
            break;
        case 0x4u: {
            uint16_t sum = (uint16_t)s_v[x] + s_v[y];
            s_v[0xF] = (sum > 255u) ? 1u : 0u;
            s_v[x] = (uint8_t)sum;
            break;
        }
        case 0x5u:
            s_v[0xF] = (s_v[x] >= s_v[y]) ? 1u : 0u;
            s_v[x] = (uint8_t)(s_v[x] - s_v[y]);
            break;
        case 0x6u:
            s_v[0xF] = (uint8_t)(s_v[x] & 1u);
            s_v[x] = (uint8_t)(s_v[x] >> 1);
            break;
        case 0x7u:
            s_v[0xF] = (s_v[y] >= s_v[x]) ? 1u : 0u;
            s_v[x] = (uint8_t)(s_v[y] - s_v[x]);
            break;
        case 0xEu:
            s_v[0xF] = (uint8_t)((s_v[x] >> 7) & 1u);
            s_v[x] = (uint8_t)(s_v[x] << 1);
            break;
        default:
            break;
        }
        break;
    case 0x9000u:
        if (n == 0u && s_v[x] != s_v[y]) {
            s_pc = (uint16_t)(s_pc + 2u);
        }
        break;
    case 0xA000u:
        s_i = nnn;
        break;
    case 0xB000u:
        s_pc = (uint16_t)(nnn + s_v[0]);
        break;
    case 0xC000u:
        s_seed = s_seed * 1664525u + 1013904223u;
        s_v[x] = (uint8_t)((s_seed >> 16) & kk);
        break;
    case 0xD000u: {
        uint8_t px = (uint8_t)(s_v[x] % FB_W);
        uint8_t py = (uint8_t)(s_v[y] % FB_H);
        uint8_t row;
        s_v[0xF] = 0u;
        for (row = 0u; row < n; row++) {
            uint8_t bits;
            uint8_t col;
            if ((uint32_t)s_i + row >= MEM_N) {
                break;
            }
            bits = s_mem[s_i + row];
            for (col = 0u; col < 8u; col++) {
                uint8_t xx;
                uint8_t yy;
                if ((bits & (uint8_t)(0x80u >> col)) == 0u) {
                    continue;
                }
                xx = (uint8_t)((px + col) % FB_W);
                yy = (uint8_t)((py + row) % FB_H);
                if (s_fb[yy * FB_W + xx] != 0u) {
                    s_v[0xF] = 1u;
                }
                s_fb[yy * FB_W + xx] ^= 1u;
            }
        }
        break;
    }
    case 0xE000u:
        if (kk == 0x9Eu && s_key[s_v[x] & 0x0Fu] != 0u) {
            s_pc = (uint16_t)(s_pc + 2u);
        } else if (kk == 0xA1u && s_key[s_v[x] & 0x0Fu] == 0u) {
            s_pc = (uint16_t)(s_pc + 2u);
        }
        break;
    case 0xF000u:
        switch (kk) {
        case 0x07u:
            s_v[x] = s_dt;
            break;
        case 0x0Au: {
            uint8_t found = 0u;
            for (i = 0u; i < 16u; i++) {
                if (s_key[i] != 0u) {
                    s_v[x] = (uint8_t)i;
                    found = 1u;
                    break;
                }
            }
            if (found == 0u) {
                s_pc = (uint16_t)(s_pc - 2u);
                s_wait = 1u;
            }
            break;
        }
        case 0x15u:
            s_dt = s_v[x];
            break;
        case 0x18u:
            s_st = s_v[x];
            break;
        case 0x1Eu:
            s_i = (uint16_t)(s_i + s_v[x]);
            break;
        case 0x29u:
            s_i = (uint16_t)(FONT_BASE + (uint16_t)(s_v[x] & 0x0Fu) * 5u);
            break;
        case 0x33u:
            if (s_i + 2u < MEM_N) {
                s_mem[s_i] = (uint8_t)(s_v[x] / 100u);
                s_mem[s_i + 1u] = (uint8_t)((s_v[x] / 10u) % 10u);
                s_mem[s_i + 2u] = (uint8_t)(s_v[x] % 10u);
            }
            break;
        case 0x55u:
            for (i = 0u; i <= x && (uint32_t)s_i + i < MEM_N; i++) {
                s_mem[s_i + i] = s_v[i];
            }
            s_i = (uint16_t)(s_i + x + 1u);
            break;
        case 0x65u:
            for (i = 0u; i <= x && (uint32_t)s_i + i < MEM_N; i++) {
                s_v[i] = s_mem[s_i + i];
            }
            s_i = (uint16_t)(s_i + x + 1u);
            break;
        default:
            break;
        }
        break;
    default:
        break;
    }
}

static void chip8_tick(game_t *g, uint32_t dt_ms)
{
    unsigned steps;
    unsigned i;

    if (g != NULL && g->phase != (uint8_t)GAME_PHASE_PLAY) {
        return;
    }
    if (dt_ms == 0u) {
        return;
    }
    if (dt_ms > 32u) {
        dt_ms = 32u;
    }
    s_acc += dt_ms;
    while (s_acc >= 16u) {
        s_acc -= 16u;
        if (s_dt > 0u) {
            s_dt--;
        }
        if (s_st > 0u) {
            s_st--;
        }
        steps = 12u;
        for (i = 0u; i < steps; i++) {
            op_step();
        }
    }
}

static int key_at(int16_t x, int16_t y)
{
    int c;
    int r;

    if (s_pad_s == 0u) {
        return -1;
    }
    if (x < s_pad_x || y < s_pad_y) {
        return -1;
    }
    c = (x - s_pad_x) / (int)s_pad_s;
    r = (y - s_pad_y) / (int)s_pad_s;
    if (c < 0 || c > 3 || r < 0 || r > 3) {
        return -1;
    }
    return (int)k_hex[r * 4 + c];
}

static void chip8_input(game_t *g, const input_event_t *e)
{
    int k;

    if (g != NULL && g->phase != (uint8_t)GAME_PHASE_PLAY) {
        return;
    }
    if (e == NULL) {
        return;
    }
    if (e->kind == INPUT_PTR_UP) {
        memset(s_key, 0, sizeof(s_key));
        return;
    }
    if (e->kind != INPUT_PTR_DOWN && e->kind != INPUT_PTR_MOVE) {
        return;
    }
    k = key_at(e->x, e->y);
    memset(s_key, 0, sizeof(s_key));
    if (k >= 0) {
        s_key[k] = 1u;
        s_wait = 0u;
    }
}

static void chip8_draw(const game_t *g, struct gfx *fx)
{
    int scale;
    int ox;
    int oy;
    int pad;
    unsigned x;
    unsigned y;
    gfx_rect_t r;

    if (g == NULL || fx == NULL) {
        return;
    }
    gfx_clear(fx, rgb565(0xF4F6FAu));
    pad = 0;
    scale = (int)g->h / (int)FB_H;
    if (g->w >= 480u && g->h >= 140u) {
        pad = 1;
        if (scale > ((int)g->w - 176) / (int)FB_W) {
            scale = ((int)g->w - 176) / (int)FB_W;
        }
    } else if (scale > (int)g->w / (int)FB_W) {
        scale = (int)g->w / (int)FB_W;
    }
    if (scale < 1) {
        scale = 1;
    }
    ox = 8;
    oy = ((int)g->h - (int)FB_H * scale) / 2;
    if (oy < 0) {
        oy = 0;
    }
    for (y = 0u; y < FB_H; y++) {
        for (x = 0u; x < FB_W; x++) {
            if (s_fb[y * FB_W + x] == 0u) {
                continue;
            }
            r.x = (int16_t)(ox + (int)x * scale);
            r.y = (int16_t)(oy + (int)y * scale);
            r.w = (uint16_t)scale;
            r.h = (uint16_t)scale;
            gfx_fill(fx, r, rgb565(0x1A2030u));
        }
    }
    if (pad != 0) {
        uint16_t cell = 40u;
        if ((uint16_t)(4u * cell) > g->h) {
            cell = (uint16_t)(g->h / 4u);
        }
        if (cell < 28u) {
            cell = 28u;
        }
        s_pad_s = cell;
        s_pad_x = (int16_t)(g->w - (int16_t)(4u * cell) - 8);
        s_pad_y = (int16_t)(((int)g->h - (int)(4u * cell)) / 2);
        if (s_pad_y < 0) {
            s_pad_y = 0;
        }
        for (y = 0u; y < 4u; y++) {
            for (x = 0u; x < 4u; x++) {
                uint8_t hex = k_hex[y * 4u + x];
                r.x = (int16_t)(s_pad_x + (int)x * (int)cell + 2);
                r.y = (int16_t)(s_pad_y + (int)y * (int)cell + 2);
                r.w = (uint16_t)(cell - 4u);
                r.h = (uint16_t)(cell - 4u);
                gfx_fill(fx, r, (s_key[hex] != 0u) ? rgb565(0xFF9F0Au) : rgb565(0xEEF1F6u));
            }
        }
    } else {
        s_pad_s = 0u;
    }
}

static const game_module_t g_chip8 = {
    "chip8", chip8_reset, chip8_input, chip8_tick, chip8_draw, chip8_load,
};

const game_module_t *game_chip8_module(void)
{
    return &g_chip8;
}
