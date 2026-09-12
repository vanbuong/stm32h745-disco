#include "app/settings.h"

#include "app/network.h"
#include "fw_version.h"
#include "svc/audio.h"
#include "svc/cfg.h"
#include "svc/health.h"
#include "svc/home.h"

#include <stddef.h>
#include <stdint.h>

static uint32_t g_gen;
static char g_banner[40];
static char g_bright[28];
static char g_vol[24];
static char g_zb[36];
static char g_about[56];

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

static void append_str(char *dst, size_t n, const char *s)
{
    size_t i = 0u;

    if (dst == NULL || n == 0u || s == NULL) {
        return;
    }
    while (dst[i] != '\0' && i + 1u < n) {
        i++;
    }
    while (*s != '\0' && i + 1u < n) {
        dst[i++] = *s++;
    }
    dst[i] = '\0';
}

static void append_u8(char *dst, size_t n, uint8_t v)
{
    char tmp[4];
    size_t i = 0u;

    if (v >= 100u) {
        tmp[i++] = (char)('0' + ((v / 100u) % 10u));
        tmp[i++] = (char)('0' + ((v / 10u) % 10u));
        tmp[i++] = (char)('0' + (v % 10u));
    } else if (v >= 10u) {
        tmp[i++] = (char)('0' + ((v / 10u) % 10u));
        tmp[i++] = (char)('0' + (v % 10u));
    } else {
        tmp[i++] = (char)('0' + v);
    }
    tmp[i] = '\0';
    append_str(dst, n, tmp);
}

static int clamp_i(int v, int lo, int hi)
{
    if (v < lo) {
        return lo;
    }
    if (v > hi) {
        return hi;
    }
    return v;
}

static void bump(void)
{
    g_gen++;
}

static void refresh_lines(void)
{
    home_net_t net;
    const char *zbver;
    health_reason_t why;

    g_bright[0] = '\0';
    append_str(g_bright, sizeof(g_bright), "Brightness  ");
    append_u8(g_bright, sizeof(g_bright), cfg_brightness());
    append_str(g_bright, sizeof(g_bright), "%");

    g_vol[0] = '\0';
    append_str(g_vol, sizeof(g_vol), "Volume  ");
    append_u8(g_vol, sizeof(g_vol), audio_volume());
    append_str(g_vol, sizeof(g_vol), "%");

    g_zb[0] = '\0';
    append_str(g_zb, sizeof(g_zb), "Zigbee  ch ");
    append_u8(g_zb, sizeof(g_zb), cfg_zb_channel());
    append_str(g_zb, sizeof(g_zb), "  join ");
    append_u8(g_zb, sizeof(g_zb), cfg_join_s());
    append_str(g_zb, sizeof(g_zb), "s");

    home_net(&net);
    zbver = net.znp_ver;
    if (zbver == NULL || zbver[0] == '\0') {
        zbver = (net.radio_ok != 0u) ? "ok" : "—";
    }
    why = health_reason();
    if (health_oom() != 0u) {
        why = HEALTH_REASON_OOM;
    } else if (health_expired() != 0u) {
        why = HEALTH_REASON_WDOG;
    }
    g_about[0] = '\0';
    append_str(g_about, sizeof(g_about), "M7 " FW_VERSION_M7 "  M4 " FW_VERSION_M4 "  ZB ");
    append_str(g_about, sizeof(g_about), zbver);
    append_str(g_about, sizeof(g_about), "  ");
    append_str(g_about, sizeof(g_about), health_reason_text(why));

    if (health_oom() != 0u) {
        copy_str(g_banner, sizeof(g_banner), "Out of memory");
    } else if (health_expired() != 0u) {
        copy_str(g_banner, sizeof(g_banner), "Watchdog expired");
    } else if (health_boot_reason() == HEALTH_REASON_WDOG) {
        copy_str(g_banner, sizeof(g_banner), "Restarted by watchdog");
    } else if (health_boot_reason() == HEALTH_REASON_BOR) {
        copy_str(g_banner, sizeof(g_banner), "Restarted after brown-out");
    } else {
        g_banner[0] = '\0';
    }
}

void settings_open(void)
{
    network_refresh();
    refresh_lines();
    bump();
}

void settings_close(void)
{
}

void settings_tick(uint32_t dt_ms)
{
    (void)dt_ms;
    network_refresh();
    refresh_lines();
}

void settings_nudge_brightness(int delta)
{
    int v = clamp_i((int)cfg_brightness() + delta, (int)CFG_BRIGHT_MIN, 100);
    (void)cfg_set_brightness((uint8_t)v);
    refresh_lines();
    bump();
}

void settings_nudge_volume(int delta)
{
    int v = clamp_i((int)audio_volume() + delta, 0, 100);
    (void)audio_set_volume((uint8_t)v);
    (void)cfg_set_volume((uint8_t)v);
    refresh_lines();
    bump();
}

void settings_nudge_zb_channel(int delta)
{
    int v = clamp_i((int)cfg_zb_channel() + delta, (int)CFG_ZB_CH_MIN, (int)CFG_ZB_CH_MAX);
    (void)cfg_set_zb_channel((uint8_t)v);
    refresh_lines();
    bump();
}

void settings_nudge_join_s(int delta)
{
    int v = clamp_i((int)cfg_join_s() + delta, 1, 254);
    (void)cfg_set_join_s((uint8_t)v);
    refresh_lines();
    bump();
}

uint8_t settings_brightness(void)
{
    return cfg_brightness();
}

uint8_t settings_volume(void)
{
    return audio_volume();
}

uint8_t settings_zb_channel(void)
{
    return cfg_zb_channel();
}

uint8_t settings_join_s(void)
{
    return cfg_join_s();
}

const char *settings_banner(void)
{
    return g_banner;
}

const char *settings_bright_line(void)
{
    return g_bright;
}

const char *settings_vol_line(void)
{
    return g_vol;
}

const char *settings_zb_line(void)
{
    return g_zb;
}

const char *settings_about(void)
{
    return g_about;
}

uint32_t settings_gen(void)
{
    return g_gen;
}
