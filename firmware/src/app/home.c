#include "app/home.h"

#include <stddef.h>

static home_page_t g_page;
static unsigned g_sel;
static uint32_t g_gen;
static char g_banner[48];
static char g_title[16];

static void bump(void)
{
    g_gen++;
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

static void put_u16(char *out, size_t n, uint16_t v, size_t *o)
{
    char tmp[6];
    int i = 6;

    if (out == NULL || o == NULL || n == 0u) {
        return;
    }
    tmp[5] = '\0';
    if (v == 0u) {
        tmp[--i] = '0';
    }
    while (v > 0u && i > 0) {
        tmp[--i] = (char)('0' + (v % 10u));
        v = (uint16_t)(v / 10u);
    }
    while (tmp[i] != '\0' && *o + 1u < n) {
        out[(*o)++] = tmp[i++];
    }
}

static void append(char *out, size_t n, const char *s, size_t *o)
{
    if (out == NULL || s == NULL || o == NULL) {
        return;
    }
    while (*s != '\0' && *o + 1u < n) {
        out[(*o)++] = *s++;
    }
}

static void refresh_banner(void)
{
    home_net_t n;
    size_t o = 0u;

    home_net(&n);
    if (n.permit_left > 0u) {
        append(g_banner, sizeof(g_banner), "Pairing... ", &o);
        put_u16(g_banner, sizeof(g_banner), n.permit_left, &o);
        append(g_banner, sizeof(g_banner), " s", &o);
        g_banner[o] = '\0';
        return;
    }
    if (n.radio_ok == 0u) {
        copy_str(g_banner, sizeof(g_banner), "Radio not ready");
        return;
    }
    append(g_banner, sizeof(g_banner), "Zigbee  ch ", &o);
    put_u16(g_banner, sizeof(g_banner), n.channel, &o);
    append(g_banner, sizeof(g_banner), "   ", &o);
    put_u16(g_banner, sizeof(g_banner), (uint16_t)home_device_count(), &o);
    append(g_banner, sizeof(g_banner), " devices", &o);
    g_banner[o] = '\0';
}

void home_app_open(void)
{
    (void)home_init();
    g_page = HOME_PAGE_LIST;
    g_sel = 0u;
    refresh_banner();
    copy_str(g_title, sizeof(g_title), "Home");
    bump();
}

void home_app_close(void)
{
    g_page = HOME_PAGE_LIST;
    g_sel = 0u;
}

void home_app_tick(uint32_t dt_ms)
{
    (void)dt_ms;
    refresh_banner();
}

uint8_t home_app_on_back(void)
{
    if (g_page != HOME_PAGE_LIST) {
        g_page = HOME_PAGE_LIST;
        copy_str(g_title, sizeof(g_title), "Home");
        bump();
        return 1u;
    }
    return 0u;
}

void home_app_pair(void)
{
    (void)home_permit_join(60u);
    refresh_banner();
}

void home_app_open_device(unsigned index)
{
    if (index >= home_device_count()) {
        return;
    }
    g_sel = index;
    g_page = HOME_PAGE_DEVICE;
    copy_str(g_title, sizeof(g_title), "Device");
    bump();
}

void home_app_open_network(void)
{
    g_page = HOME_PAGE_NETWORK;
    copy_str(g_title, sizeof(g_title), "Network");
    bump();
}

void home_app_toggle(unsigned index)
{
    home_device_t d;
    home_cmd_t cmd;

    if (home_device_at((size_t)index, &d) != ERR_OK) {
        return;
    }
    if (d.kind != HOME_LIGHT && d.kind != HOME_SWITCH) {
        return;
    }
    cmd.on = (d.on != 0u) ? 0u : 1u;
    cmd.has_level = 0u;
    cmd.level = 0u;
    (void)home_cmd(d.id, &cmd);
    bump();
}

home_page_t home_app_page(void)
{
    return g_page;
}

unsigned home_app_sel(void)
{
    return g_sel;
}

uint32_t home_app_gen(void)
{
    return g_gen + home_gen();
}

const char *home_app_banner(void)
{
    refresh_banner();
    return g_banner;
}

const char *home_app_title(void)
{
    return g_title;
}

const char *home_app_state_text(const home_device_t *d)
{
    if (d == NULL) {
        return "";
    }
    if (d->kind == HOME_BINARY_SENSOR) {
        return (d->on != 0u) ? "active" : "clear";
    }
    if (d->kind == HOME_CLIMATE) {
        return "climate";
    }
    return (d->on != 0u) ? "ON" : "OFF";
}

const char *home_app_kind_text(home_kind_t kind)
{
    if (kind == HOME_LIGHT) {
        return "Light";
    }
    if (kind == HOME_SWITCH) {
        return "Switch";
    }
    if (kind == HOME_BINARY_SENSOR) {
        return "Sensor";
    }
    return "Climate";
}
