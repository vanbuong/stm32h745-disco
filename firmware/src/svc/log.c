#include "svc/log.h"

#include <stdarg.h>
#include <stdint.h>
#include <string.h>

#if defined(CORE_CM7)
#include "bsp/board.h"
#endif

#define CORE_MAX 8u
#define MOD_MAX 16u

static uint8_t g_ready;
static log_lvl_t g_level;
static char g_core[CORE_MAX];
static char g_last[LOG_LINE_MAX];
static uint32_t g_count;
static void (*g_sink)(const char *line, size_t n);
static uint32_t (*g_clock)(void);

#if defined(CORE_CM7)
static uint32_t default_clock(void)
{
    return board_millis();
}
#else
static uint32_t default_clock(void)
{
    return 0u;
}
#endif

static uint32_t now_ms(void)
{
    if (g_clock != NULL) {
        return g_clock();
    }
    return default_clock();
}

static void copy_str(char *dst, size_t n, const char *s)
{
    size_t i = 0u;

    if (dst == NULL || n == 0u) {
        return;
    }
    if (s == NULL) {
        dst[0] = '\0';
        return;
    }
    while (s[i] != '\0' && i + 1u < n) {
        dst[i] = s[i];
        i++;
    }
    dst[i] = '\0';
}

static const char *default_core(void)
{
#if defined(CORE_CM7)
    return "m7";
#elif defined(CORE_CM4)
    return "m4";
#else
    return "host";
#endif
}

static char lvl_ch(log_lvl_t lvl)
{
    if (lvl == LOG_ERROR) {
        return 'E';
    }
    if (lvl == LOG_WARN) {
        return 'W';
    }
    if (lvl == LOG_DEBUG) {
        return 'D';
    }
    return 'I';
}

#if defined(CORE_CM7)
static void board_sink(const char *line, size_t n)
{
    (void)n;
    board_console_puts(line);
    board_console_puts("\r\n");
}
#endif

static void emit(void)
{
    g_count++;
    if (g_sink != NULL) {
        g_sink(g_last, strlen(g_last));
    }
}

static void put_ch(char *out, size_t n, size_t *o, char c)
{
    if (out == NULL || o == NULL || n == 0u) {
        return;
    }
    if (*o + 1u < n) {
        out[(*o)++] = c;
    }
}

static void put_str(char *out, size_t n, size_t *o, const char *s)
{
    if (s == NULL) {
        s = "(null)";
    }
    while (*s != '\0') {
        put_ch(out, n, o, *s++);
    }
}

static void put_u64(char *out, size_t n, size_t *o, uint64_t v, unsigned base, int upper, int width,
                    int zpad)
{
    char tmp[24];
    const char *hex = upper != 0 ? "0123456789ABCDEF" : "0123456789abcdef";
    int i = 0;
    int pad;

    if (base < 2u || base > 16u) {
        return;
    }
    do {
        tmp[i++] = hex[v % (uint64_t)base];
        v /= (uint64_t)base;
    } while (v > 0u && i < (int)sizeof(tmp));
    pad = width - i;
    while (pad > 0) {
        put_ch(out, n, o, (zpad != 0) ? '0' : ' ');
        pad--;
    }
    while (i > 0) {
        put_ch(out, n, o, tmp[--i]);
    }
}

static uint64_t uabs64(int64_t v)
{
    uint64_t u;

    if (v >= 0) {
        return (uint64_t)v;
    }
    u = (uint64_t)(-(v + 1));
    return u + 1u;
}

static void format_msg(char *out, size_t n, size_t *o, const char *fmt, va_list ap)
{
    const char *p;

    if (fmt == NULL) {
        return;
    }
    for (p = fmt; *p != '\0'; p++) {
        int width;
        int zpad;
        int llen;
        int upper;
        unsigned base;
        int is_signed;
        uint64_t uv;
        int64_t sv;

        if (*p != '%') {
            if (*p == '\r' || *p == '\n') {
                continue;
            }
            put_ch(out, n, o, *p);
            continue;
        }
        p++;
        if (*p == '\0') {
            put_ch(out, n, o, '%');
            break;
        }
        if (*p == '%') {
            put_ch(out, n, o, '%');
            continue;
        }
        zpad = 0;
        width = 0;
        if (*p == '0') {
            zpad = 1;
            p++;
        }
        while (*p >= '0' && *p <= '9') {
            width = (width * 10) + (*p - '0');
            p++;
        }
        if (width > 16) {
            width = 16;
        }
        llen = 0;
        if (*p == 'z') {
            llen = 1;
            p++;
        } else if (*p == 'l') {
            p++;
            if (*p == 'l') {
                llen = 2;
                p++;
            } else {
                llen = 1;
            }
        } else if (*p == 'h') {
            p++;
            if (*p == 'h') {
                p++;
            }
        }
        if (*p == '\0') {
            break;
        }
        upper = 0;
        base = 10u;
        is_signed = 0;
        if (*p == 'c') {
            put_ch(out, n, o, (char)va_arg(ap, int));
            continue;
        }
        if (*p == 's') {
            put_str(out, n, o, va_arg(ap, const char *));
            continue;
        }
        if (*p == 'p') {
            put_str(out, n, o, "0x");
            put_u64(out, n, o, (uint64_t)(uintptr_t)va_arg(ap, void *), 16u, 0, 0, 0);
            continue;
        }
        if (*p == 'd' || *p == 'i') {
            is_signed = 1;
        } else if (*p == 'u') {
            is_signed = 0;
        } else if (*p == 'x') {
            base = 16u;
        } else if (*p == 'X') {
            base = 16u;
            upper = 1;
        } else {
            put_ch(out, n, o, *p);
            continue;
        }
        if (is_signed != 0) {
            if (llen >= 2) {
                sv = (int64_t)va_arg(ap, long long);
            } else if (llen == 1) {
                sv = (int64_t)va_arg(ap, long);
            } else {
                sv = (int64_t)va_arg(ap, int);
            }
            if (sv < 0) {
                put_ch(out, n, o, '-');
                if (width > 0) {
                    width--;
                }
            }
            uv = uabs64(sv);
        } else if (llen >= 2) {
            uv = (uint64_t)va_arg(ap, unsigned long long);
        } else if (llen == 1) {
            uv = (uint64_t)va_arg(ap, unsigned long);
        } else {
            uv = (uint64_t)va_arg(ap, unsigned int);
        }
        put_u64(out, n, o, uv, base, upper, width, zpad);
    }
}

void log_init(void)
{
    if (g_ready != 0u) {
        return;
    }
    g_level = LOG_INFO;
    if (g_core[0] == '\0') {
        copy_str(g_core, sizeof(g_core), default_core());
    }
#if defined(CORE_CM7)
    if (g_sink == NULL) {
        g_sink = board_sink;
    }
#endif
    g_last[0] = '\0';
    g_ready = 1u;
}

void log_reset(void)
{
    g_ready = 0u;
    g_level = LOG_INFO;
    g_core[0] = '\0';
    g_last[0] = '\0';
    g_count = 0u;
    g_sink = NULL;
    g_clock = NULL;
}

void log_set_level(log_lvl_t max)
{
    if ((unsigned)max > (unsigned)LOG_DEBUG) {
        max = LOG_DEBUG;
    }
    g_level = max;
}

log_lvl_t log_level(void)
{
    return g_level;
}

void log_set_core(const char *core)
{
    copy_str(g_core, sizeof(g_core), (core != NULL && core[0] != '\0') ? core : default_core());
}

void log_set_clock(uint32_t (*millis)(void))
{
    g_clock = millis;
}

void log_set_sink(void (*fn)(const char *line, size_t n))
{
    g_sink = fn;
}

void log_write(log_lvl_t lvl, const char *mod, const char *fmt, ...)
{
    va_list ap;
    size_t o = 0u;
    char tag[MOD_MAX];

    if (g_ready == 0u) {
        log_init();
    }
    if ((unsigned)lvl > (unsigned)g_level) {
        return;
    }
    copy_str(tag, sizeof(tag), (mod != NULL && mod[0] != '\0') ? mod : "app");
    put_str(g_last, sizeof(g_last), &o, g_core[0] != '\0' ? g_core : default_core());
    put_ch(g_last, sizeof(g_last), &o, ',');
    put_u64(g_last, sizeof(g_last), &o, (uint64_t)now_ms(), 10u, 0, 0, 0);
    put_ch(g_last, sizeof(g_last), &o, ',');
    put_ch(g_last, sizeof(g_last), &o, lvl_ch(lvl));
    put_ch(g_last, sizeof(g_last), &o, ',');
    put_str(g_last, sizeof(g_last), &o, tag);
    put_ch(g_last, sizeof(g_last), &o, ',');
    va_start(ap, fmt);
    format_msg(g_last, sizeof(g_last), &o, fmt, ap);
    va_end(ap);
    g_last[o] = '\0';
    emit();
}

void log_hex(log_lvl_t lvl, const char *mod, const void *data, size_t n)
{
    const uint8_t *p = (const uint8_t *)data;
    char hex[48];
    size_t o = 0u;
    size_t i;
    size_t lim;

    if ((unsigned)lvl > (unsigned)g_level && g_ready != 0u) {
        return;
    }
    if (p == NULL) {
        log_write(lvl, mod, "hex (null)");
        return;
    }
    lim = (n > 16u) ? 16u : n;
    for (i = 0u; i < lim; i++) {
        if (i > 0u) {
            put_ch(hex, sizeof(hex), &o, ' ');
        }
        put_u64(hex, sizeof(hex), &o, (uint64_t)p[i], 16u, 1, 2, 1);
    }
    hex[o] = '\0';
    log_write(lvl, mod, "%s", hex);
}

const char *log_last(void)
{
    return g_last;
}

uint32_t log_count(void)
{
    return g_count;
}
