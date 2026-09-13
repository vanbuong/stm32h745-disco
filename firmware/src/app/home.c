#include "app/home.h"

#include "svc/cfg.h"

#include "svc/auto.h"

#include <stddef.h>
#include <string.h>

static home_page_t g_page;
static unsigned g_sel;
static uint32_t g_gen;
static char g_banner[48];
static char g_title[16];
static char g_seen[20];
static char g_rname[HOME_NAME_MAX];
static char g_rsum[48];

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
    char tmp[7];
    int i = 6;

    if (out == NULL || o == NULL || n == 0u) {
        return;
    }
    tmp[6] = '\0';
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

static void set_title(const char *s)
{
    copy_str(g_title, sizeof(g_title), s);
}

static const char *dev_name(const uint8_t ieee[8])
{
    home_device_t d;
    size_t i;
    size_t n = home_device_count();

    for (i = 0u; i < n; i++) {
        if (home_device_at(i, &d) != ERR_OK) {
            continue;
        }
        if (d.ieee[0] == ieee[0] && d.ieee[1] == ieee[1] && d.ieee[2] == ieee[2] &&
            d.ieee[3] == ieee[3] && d.ieee[4] == ieee[4] && d.ieee[5] == ieee[5] &&
            d.ieee[6] == ieee[6] && d.ieee[7] == ieee[7]) {
            copy_str(g_rname, sizeof(g_rname), d.name);
            return g_rname;
        }
    }
    copy_str(g_rname, sizeof(g_rname), "device");
    return g_rname;
}

static void fill_summary(const auto_rule_t *r)
{
    const char *trig;
    const char *act;
    size_t o = 0u;
    char trig_name[HOME_NAME_MAX];
    char act_name[HOME_NAME_MAX];

    copy_str(trig_name, sizeof(trig_name), dev_name(r->trig_ieee));
    copy_str(act_name, sizeof(act_name), dev_name(r->action_ieee));
    if (r->trig == AUTO_TRIG_OCCUPIED) {
        trig = "occupied";
    } else if (r->trig == AUTO_TRIG_ON) {
        trig = "on";
    } else if (r->trig == AUTO_TRIG_TEMP_GT) {
        trig = "temp";
    } else {
        trig = "time";
    }
    if (r->action == AUTO_ACT_OFF) {
        act = "Off";
    } else if (r->action == AUTO_ACT_TOGGLE) {
        act = "Toggle";
    } else {
        act = "On";
    }
    append(g_rsum, sizeof(g_rsum), trig_name, &o);
    append(g_rsum, sizeof(g_rsum), " ", &o);
    append(g_rsum, sizeof(g_rsum), trig, &o);
    append(g_rsum, sizeof(g_rsum), " -> ", &o);
    append(g_rsum, sizeof(g_rsum), act, &o);
    append(g_rsum, sizeof(g_rsum), " ", &o);
    append(g_rsum, sizeof(g_rsum), act_name, &o);
    g_rsum[o] = '\0';
}

static int find_kind(home_kind_t kind, home_device_t *out)
{
    size_t i;
    size_t n = home_device_count();

    for (i = 0u; i < n; i++) {
        if (home_device_at(i, out) == ERR_OK && out->kind == kind) {
            return 1;
        }
    }
    return 0;
}

void home_app_open(void)
{
    (void)home_init();
    g_page = HOME_PAGE_LIST;
    g_sel = 0u;
    refresh_banner();
    set_title("Home");
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
    if (g_page == HOME_PAGE_RULE) {
        g_page = HOME_PAGE_AUTOS;
        set_title("Rules");
        bump();
        return 1u;
    }
    if (g_page != HOME_PAGE_LIST) {
        g_page = HOME_PAGE_LIST;
        set_title("Home");
        bump();
        return 1u;
    }
    return 0u;
}

void home_app_pair(void)
{
    (void)home_permit_join(cfg_join_s());
    refresh_banner();
}

void home_app_open_device(unsigned index)
{
    if (index >= home_device_count()) {
        return;
    }
    g_sel = index;
    g_page = HOME_PAGE_DEVICE;
    set_title("Device");
    bump();
}

void home_app_open_network(void)
{
    g_page = HOME_PAGE_NETWORK;
    set_title("Network");
    bump();
}

void home_app_open_autos(void)
{
    g_page = HOME_PAGE_AUTOS;
    set_title("Rules");
    bump();
}

void home_app_open_rule(unsigned index)
{
    if (index >= auto_count()) {
        return;
    }
    g_sel = index;
    g_page = HOME_PAGE_RULE;
    set_title("Rule");
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

void home_app_toggle_rule(unsigned index)
{
    auto_rule_t r;

    if (auto_at((size_t)index, &r) != ERR_OK) {
        return;
    }
    (void)auto_set_enabled(r.id, (r.enabled != 0u) ? 0u : 1u);
    bump();
}

void home_app_add_rule(void)
{
    auto_rule_t r;
    home_device_t trig;
    home_device_t act;

    memset(&r, 0, sizeof(r));
    if (find_kind(HOME_BINARY_SENSOR, &trig) == 0 || find_kind(HOME_LIGHT, &act) == 0) {
        return;
    }
    copy_str(r.name, HOME_NAME_MAX, "Motion light");
    memcpy(r.trig_ieee, trig.ieee, 8u);
    memcpy(r.action_ieee, act.ieee, 8u);
    r.trig = AUTO_TRIG_OCCUPIED;
    r.action = AUTO_ACT_ON;
    r.delay_ms = 3000u;
    r.enabled = 1u;
    (void)auto_add(&r);
    bump();
}

void home_app_delete_rule(void)
{
    auto_rule_t r;

    if (auto_at((size_t)g_sel, &r) != ERR_OK) {
        return;
    }
    (void)auto_remove(r.id);
    g_page = HOME_PAGE_AUTOS;
    set_title("Rules");
    bump();
}

void home_app_remove_device(void)
{
    home_device_t d;

    if (home_device_at((size_t)g_sel, &d) != ERR_OK) {
        return;
    }
    (void)home_remove(d.id);
    g_page = HOME_PAGE_LIST;
    set_title("Home");
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

const char *home_app_last_seen(const home_device_t *d)
{
    uint32_t dt;
    size_t o = 0u;

    if (d == NULL || d->last_seen_ms == 0u) {
        copy_str(g_seen, sizeof(g_seen), "never");
        return g_seen;
    }
    dt = home_now_ms() - d->last_seen_ms;
    if (dt < 5000u) {
        copy_str(g_seen, sizeof(g_seen), "just now");
        return g_seen;
    }
    put_u16(g_seen, sizeof(g_seen), (uint16_t)(dt / 1000u), &o);
    append(g_seen, sizeof(g_seen), " s ago", &o);
    g_seen[o] = '\0';
    return g_seen;
}

const char *home_app_clusters(home_kind_t kind)
{
    return home_cluster_text(kind);
}

unsigned home_app_rule_count(void)
{
    return (unsigned)auto_count();
}

uint16_t home_app_rule_id(unsigned index)
{
    auto_rule_t r;

    if (auto_at((size_t)index, &r) != ERR_OK) {
        return 0u;
    }
    return r.id;
}

uint8_t home_app_rule_enabled(unsigned index)
{
    auto_rule_t r;

    if (auto_at((size_t)index, &r) != ERR_OK) {
        return 0u;
    }
    return r.enabled;
}

const char *home_app_rule_name(unsigned index)
{
    auto_rule_t r;

    if (auto_at((size_t)index, &r) != ERR_OK) {
        copy_str(g_rname, sizeof(g_rname), "");
        return g_rname;
    }
    copy_str(g_rname, sizeof(g_rname), r.name);
    return g_rname;
}

const char *home_app_rule_summary(unsigned index)
{
    auto_rule_t r;

    if (auto_at((size_t)index, &r) != ERR_OK) {
        g_rsum[0] = '\0';
        return g_rsum;
    }
    fill_summary(&r);
    return g_rsum;
}

const char *home_app_rule_delay(unsigned index)
{
    auto_rule_t r;
    size_t o = 0u;

    if (auto_at((size_t)index, &r) != ERR_OK) {
        copy_str(g_seen, sizeof(g_seen), "");
        return g_seen;
    }
    append(g_seen, sizeof(g_seen), "Delay ", &o);
    put_u16(g_seen, sizeof(g_seen), (uint16_t)(r.delay_ms / 1000u), &o);
    append(g_seen, sizeof(g_seen), " s", &o);
    g_seen[o] = '\0';
    return g_seen;
}
