#include "svc/text_view.h"

#include "svc/vfs.h"

#include <string.h>

#define REPL '?'

#if defined(STM32H745xx)
#include "bsp/board.h"
#define TEXT_SDRAM (BOARD_SDRAM_BASE + 0x2C0000u)
static char *g_win(void)
{
    return (char *)TEXT_SDRAM;
}
static uint8_t *g_rawbuf(void)
{
    return (uint8_t *)(TEXT_SDRAM + TEXT_WIN_MAX + 4u);
}
#else
static char g_win_store[TEXT_WIN_MAX + 1u];
static uint8_t g_raw_store[TEXT_WIN_MAX];
static char *g_win(void)
{
    return g_win_store;
}
static uint8_t *g_rawbuf(void)
{
    return g_raw_store;
}
#endif

static const uint8_t *g_mem;
static uint32_t g_size;
static uint32_t g_off;
static uint32_t g_raw;
static uint32_t g_gen;
static err_t g_st;
static char g_name[VFS_NAME_MAX];
static char g_path[VFS_PATH_MAX];
static uint8_t g_use_mem;

static void bump(void)
{
    g_gen++;
}

static void set_name_from_path(const char *path)
{
    const char *slash;
    size_t n;

    g_name[0] = '\0';
    if (path == NULL) {
        return;
    }
    slash = strrchr(path, '/');
    slash = (slash != NULL) ? (slash + 1) : path;
    n = strlen(slash);
    if (n >= VFS_NAME_MAX) {
        n = VFS_NAME_MAX - 1u;
    }
    memcpy(g_name, slash, n);
    g_name[n] = '\0';
}

static int utf8_len(uint8_t b)
{
    if (b < 0x80u) {
        return 1;
    }
    if (b >= 0xC2u && b <= 0xDFu) {
        return 2;
    }
    if (b >= 0xE0u && b <= 0xEFu) {
        return 3;
    }
    if (b >= 0xF0u && b <= 0xF4u) {
        return 4;
    }
    return 0;
}

static int utf8_cont(uint8_t b)
{
    return (b & 0xC0u) == 0x80u;
}

static int utf8_ok(const uint8_t *p, int n)
{
    int i;
    uint32_t cp;

    if (n < 1) {
        return 0;
    }
    for (i = 1; i < n; i++) {
        if (!utf8_cont(p[i])) {
            return 0;
        }
    }
    if (n == 1) {
        return 1;
    }
    if (n == 2) {
        return 1;
    }
    if (n == 3) {
        cp = ((uint32_t)(p[0] & 0x0Fu) << 12) | ((uint32_t)(p[1] & 0x3Fu) << 6) |
             (uint32_t)(p[2] & 0x3Fu);
        if (p[0] == 0xE0u && p[1] < 0xA0u) {
            return 0;
        }
        if (cp >= 0xD800u && cp <= 0xDFFFu) {
            return 0;
        }
        return 1;
    }
    cp = ((uint32_t)(p[0] & 0x07u) << 18) | ((uint32_t)(p[1] & 0x3Fu) << 12) |
         ((uint32_t)(p[2] & 0x3Fu) << 6) | (uint32_t)(p[3] & 0x3Fu);
    if (p[0] == 0xF0u && p[1] < 0x90u) {
        return 0;
    }
    if (p[0] == 0xF4u && p[1] > 0x8Fu) {
        return 0;
    }
    return (cp <= 0x10FFFFu) ? 1 : 0;
}

static uint32_t skip_partial(const uint8_t *p, uint32_t n, uint32_t file_off)
{
    uint32_t i = 0u;

    if (file_off == 0u) {
        return 0u;
    }
    while (i < n && utf8_cont(p[i])) {
        i++;
    }
    return i;
}

static err_t read_raw(uint32_t off, uint8_t *buf, uint32_t want, uint32_t *got)
{
    if (got != NULL) {
        *got = 0u;
    }
    if (off >= g_size) {
        return ERR_OK;
    }
    if (want > g_size - off) {
        want = g_size - off;
    }
    if (g_use_mem != 0u) {
        memcpy(buf, g_mem + off, want);
        if (got != NULL) {
            *got = want;
        }
        return ERR_OK;
    }
    {
        vfs_file_t fd = -1;
        size_t n = 0u;
        err_t e = vfs_open(g_path, VFS_O_RD, &fd);
        if (e != ERR_OK) {
            return e;
        }
        e = vfs_seek(fd, off);
        if (e == ERR_OK) {
            e = vfs_read(fd, buf, want, &n);
        }
        (void)vfs_close(fd);
        if (got != NULL) {
            *got = (uint32_t)n;
        }
        return e;
    }
}

static int emit_seq(char *dst, uint32_t *o, const uint8_t *raw, uint32_t *i, uint32_t got,
                    uint32_t file_end)
{
    uint8_t b = raw[*i];
    int n;
    uint32_t left = got - *i;

    if (*o >= TEXT_WIN_MAX) {
        return 0;
    }
    if (b == (uint8_t)'\r') {
        dst[(*o)++] = '\n';
        (*i)++;
        if (*i < got && raw[*i] == (uint8_t)'\n') {
            (*i)++;
        }
        return 1;
    }
    n = utf8_len(b);
    if (n == 0) {
        dst[(*o)++] = REPL;
        (*i)++;
        return 1;
    }
    if ((uint32_t)n > left) {
        if (file_end == 0u) {
            return 0;
        }
        dst[(*o)++] = REPL;
        *i = got;
        return 0;
    }
    if (!utf8_ok(raw + *i, n)) {
        dst[(*o)++] = REPL;
        (*i)++;
        return 1;
    }
    if (*o + (uint32_t)n > TEXT_WIN_MAX) {
        return 0;
    }
    memcpy(dst + *o, raw + *i, (size_t)n);
    *o += (uint32_t)n;
    *i += (uint32_t)n;
    return 1;
}

static err_t load_window(uint32_t off)
{
    uint8_t *raw = g_rawbuf();
    char *dst = g_win();
    uint32_t got = 0u;
    uint32_t i;
    uint32_t o = 0u;
    uint32_t start;
    err_t e;

    dst[0] = '\0';
    g_raw = 0u;
    if (g_size == 0u) {
        g_off = 0u;
        g_st = ERR_OK;
        bump();
        return ERR_OK;
    }
    if (off > g_size) {
        off = g_size;
    }
    e = read_raw(off, raw, TEXT_WIN_MAX, &got);
    if (e != ERR_OK) {
        g_st = e;
        bump();
        return e;
    }
    start = skip_partial(raw, got, off);
    i = start;
    while (i < got && o < TEXT_WIN_MAX) {
        if (emit_seq(dst, &o, raw, &i, got, (off + got >= g_size) ? 1u : 0u) == 0) {
            break;
        }
    }
    dst[o] = '\0';
    g_off = off + start;
    g_raw = i - start;
    g_st = ERR_OK;
    bump();
    return ERR_OK;
}

static err_t open_common(uint32_t size)
{
    g_size = size;
    g_off = 0u;
    g_raw = 0u;
    return load_window(0u);
}

err_t text_view_open(const char *path)
{
    vfs_stat_t st;
    err_t e;

    text_view_close();
    if (path == NULL || path[0] == '\0') {
        g_st = ERR_INVAL;
        return g_st;
    }
    e = vfs_stat(path, &st);
    if (e != ERR_OK) {
        g_st = e;
        return e;
    }
    if (st.is_dir != 0u) {
        g_st = ERR_INVAL;
        return g_st;
    }
    {
        size_t n = strlen(path);
        if (n >= VFS_PATH_MAX) {
            g_st = ERR_NOSPC;
            return g_st;
        }
        memcpy(g_path, path, n + 1u);
    }
    set_name_from_path(path);
    g_use_mem = 0u;
    return open_common(st.size);
}

err_t text_view_open_mem(const uint8_t *data, uint32_t size)
{
    text_view_close();
    if (data == NULL && size != 0u) {
        g_st = ERR_INVAL;
        return g_st;
    }
    memcpy(g_name, "mem", 4u);
    g_use_mem = 1u;
    g_mem = data;
    g_path[0] = '\0';
    return open_common(size);
}

void text_view_close(void)
{
    g_win()[0] = '\0';
    g_mem = NULL;
    g_size = 0u;
    g_off = 0u;
    g_raw = 0u;
    g_st = ERR_OK;
    g_name[0] = '\0';
    g_path[0] = '\0';
    g_use_mem = 0u;
    bump();
}

err_t text_view_set_window(uint32_t byte_off)
{
    if (g_st != ERR_OK && g_size == 0u && g_use_mem == 0u && g_path[0] == '\0') {
        return ERR_INVAL;
    }
    return load_window(byte_off);
}

err_t text_view_page(int dir)
{
    uint32_t next;

    if (dir > 0) {
        next = g_off + g_raw;
        if (g_raw == 0u || next >= g_size) {
            return ERR_OK;
        }
        return load_window(next);
    }
    if (dir < 0) {
        if (g_off == 0u) {
            return ERR_OK;
        }
        next = (g_off > TEXT_WIN_MAX) ? (g_off - TEXT_WIN_MAX) : 0u;
        return load_window(next);
    }
    return ERR_OK;
}

const char *text_view_text(void)
{
    return g_win();
}

uint32_t text_view_size(void)
{
    return g_size;
}

uint32_t text_view_offset(void)
{
    return g_off;
}

uint32_t text_view_win_bytes(void)
{
    return g_raw;
}

uint32_t text_view_progress(void)
{
    uint32_t end;

    if (g_size == 0u) {
        return 100u;
    }
    end = g_off + g_raw;
    if (end >= g_size) {
        return 100u;
    }
    return (uint32_t)(((uint64_t)g_off * 100u) / g_size);
}

const char *text_view_name(void)
{
    return g_name;
}

err_t text_view_status(void)
{
    return g_st;
}

uint32_t text_view_gen(void)
{
    return g_gen;
}
