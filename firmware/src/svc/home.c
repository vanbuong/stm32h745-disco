#include "svc/home.h"

#include "svc/auto.h"
#include "svc/zb_host.h"

#include <string.h>

static void (*g_cb)(const home_device_t *);
static uint32_t g_gen;
static uint8_t g_auto_apply;

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

static void ieee_hex(const uint8_t ieee[8], char *out, size_t n)
{
    static const char hex[] = "0123456789abcdef";
    size_t i;

    if (out == NULL || n == 0u) {
        return;
    }
    if (n < 17u) {
        out[0] = '\0';
        return;
    }
    for (i = 0u; i < 8u; i++) {
        out[i * 2u] = hex[(ieee[i] >> 4) & 0x0Fu];
        out[i * 2u + 1u] = hex[ieee[i] & 0x0Fu];
    }
    out[16] = '\0';
}

static void fill_dev(home_device_t *out, const zb_dev_t *d)
{
    memset(out, 0, sizeof(*out));
    ieee_hex(d->ieee, out->id, sizeof(out->id));
    memcpy(out->ieee, d->ieee, 8u);
    out->nwk = d->nwk;
    copy_str(out->name, HOME_NAME_MAX, d->name);
    copy_str(out->room_id, HOME_ROOM_MAX, d->room_id);
    out->kind = d->kind;
    out->on = d->on;
    out->level = d->level;
    out->lqi = d->lqi;
    out->last_seen_ms = d->last_seen_ms;
}

static int id_match(const zb_dev_t *d, const char *device_id)
{
    char hex[HOME_NAME_MAX];

    if (d == NULL || device_id == NULL) {
        return 0;
    }
    if (strcmp(d->name, device_id) == 0) {
        return 1;
    }
    ieee_hex(d->ieee, hex, sizeof(hex));
    return (strcmp(hex, device_id) == 0) ? 1 : 0;
}

static err_t lookup(const char *device_id, size_t *idx)
{
    size_t i;
    size_t n;

    if (device_id == NULL || device_id[0] == '\0') {
        return ERR_INVAL;
    }
    n = zb_host_device_count();
    for (i = 0u; i < n; i++) {
        const zb_dev_t *d = zb_host_device_at(i);
        if (d != NULL && id_match(d, device_id) != 0) {
            if (idx != NULL) {
                *idx = i;
            }
            return ERR_OK;
        }
    }
    return ERR_NOENT;
}

static void notify_at(size_t i)
{
    home_device_t snap;
    const zb_dev_t *d = zb_host_device_at(i);

    if (d == NULL) {
        return;
    }
    fill_dev(&snap, d);
    if (g_cb != NULL) {
        g_cb(&snap);
    }
    if (g_auto_apply == 0u) {
        (void)auto_eval(&snap);
    }
}

static err_t apply_ieee_cmd(const uint8_t ieee[8], const home_cmd_t *cmd)
{
    char hex[HOME_NAME_MAX];

    ieee_hex(ieee, hex, sizeof(hex));
    return home_cmd(hex, cmd);
}

static void apply_due(void)
{
    auto_act_req_t act;
    home_cmd_t cmd;

    while (auto_take_due(&act) == ERR_OK) {
        cmd = act.cmd;
        if (act.toggle != 0u) {
            size_t idx;
            const zb_dev_t *d;
            if (zb_host_find(act.ieee, &idx) != ERR_OK) {
                continue;
            }
            d = zb_host_device_at(idx);
            if (d == NULL) {
                continue;
            }
            cmd.on = (d->on != 0u) ? 0u : 1u;
            cmd.has_level = 0u;
        }
        g_auto_apply = 1u;
        (void)apply_ieee_cmd(act.ieee, &cmd);
        g_auto_apply = 0u;
    }
}

err_t home_init(void)
{
    err_t e = zb_host_init();
    if (e != ERR_OK) {
        return e;
    }
    e = auto_init();
    if (e == ERR_OK) {
        bump();
    }
    return e;
}

void home_reset(void)
{
    zb_host_reset();
    auto_reset();
    g_cb = NULL;
    g_gen = 0u;
    g_auto_apply = 0u;
}

void home_poll(uint32_t dt_ms)
{
    zb_host_poll(dt_ms);
    auto_poll(dt_ms);
    apply_due();
}

size_t home_devices(const char *room_id, home_device_t *out, size_t max)
{
    size_t i;
    size_t n;
    size_t w = 0u;

    if (out == NULL || max == 0u) {
        return 0u;
    }
    n = zb_host_device_count();
    for (i = 0u; i < n && w < max; i++) {
        const zb_dev_t *d = zb_host_device_at(i);
        if (d == NULL) {
            continue;
        }
        if (room_id != NULL && room_id[0] != '\0' && strcmp(d->room_id, room_id) != 0) {
            continue;
        }
        fill_dev(&out[w], d);
        w++;
    }
    return w;
}

err_t home_device(const char *device_id, home_device_t *out)
{
    size_t idx;
    const zb_dev_t *d;

    if (out == NULL) {
        return ERR_INVAL;
    }
    if (lookup(device_id, &idx) != ERR_OK) {
        return ERR_NOENT;
    }
    d = zb_host_device_at(idx);
    if (d == NULL) {
        return ERR_NOENT;
    }
    fill_dev(out, d);
    return ERR_OK;
}

err_t home_device_at(size_t i, home_device_t *out)
{
    const zb_dev_t *d = zb_host_device_at(i);

    if (out == NULL) {
        return ERR_INVAL;
    }
    if (d == NULL) {
        return ERR_NOENT;
    }
    fill_dev(out, d);
    return ERR_OK;
}

size_t home_device_count(void)
{
    return zb_host_device_count();
}

size_t home_rooms(home_room_t *out, size_t max)
{
    size_t i;
    size_t n;
    size_t w = 0u;

    if (out == NULL || max == 0u) {
        return 0u;
    }
    n = zb_host_device_count();
    for (i = 0u; i < n; i++) {
        const zb_dev_t *d = zb_host_device_at(i);
        size_t k;
        uint8_t dup = 0u;
        if (d == NULL || d->room_id[0] == '\0') {
            continue;
        }
        for (k = 0u; k < w; k++) {
            if (strcmp(out[k].id, d->room_id) == 0) {
                dup = 1u;
                break;
            }
        }
        if (dup != 0u) {
            continue;
        }
        if (w >= max) {
            break;
        }
        copy_str(out[w].id, HOME_NAME_MAX, d->room_id);
        copy_str(out[w].name, HOME_NAME_MAX, d->room_id);
        w++;
    }
    return w;
}

err_t home_cmd(const char *device_id, const home_cmd_t *cmd)
{
    size_t idx;
    zb_dev_t *d;
    uint8_t old_on;
    uint8_t old_level;
    err_t e;

    if (cmd == NULL) {
        return ERR_INVAL;
    }
    if (lookup(device_id, &idx) != ERR_OK) {
        return ERR_NOENT;
    }
    d = zb_host_device_mut(idx);
    if (d == NULL) {
        return ERR_NOENT;
    }
    old_on = d->on;
    old_level = d->level;
    e = zb_host_apply_cmd(d->ieee, cmd);
    if (e != ERR_OK) {
        return e;
    }
    if (zb_host_cmd_allowed() == 0u) {
        d->on = old_on;
        d->level = old_level;
        return ERR_IO;
    }
    bump();
    notify_at(idx);
    return ERR_OK;
}

err_t home_set_meta(const char *device_id, const char *name, const char *room_id)
{
    size_t idx;
    const zb_dev_t *d;
    err_t e;

    if (lookup(device_id, &idx) != ERR_OK) {
        return ERR_NOENT;
    }
    d = zb_host_device_at(idx);
    if (d == NULL) {
        return ERR_NOENT;
    }
    e = zb_host_set_meta(d->ieee, name, room_id);
    if (e == ERR_OK) {
        bump();
        notify_at(idx);
    }
    return e;
}

void home_on_change(void (*cb)(const home_device_t *))
{
    g_cb = cb;
}

err_t home_form(uint8_t channel, uint16_t pan)
{
    zb_net_cfg_t cfg;

    cfg.channel = channel;
    cfg.pan = pan;
    if (zb_form(&cfg) != ERR_OK) {
        return ERR_INVAL;
    }
    bump();
    return ERR_OK;
}

err_t home_permit_join(uint8_t seconds)
{
    return zb_permit_join(seconds);
}

err_t home_remove(const char *device_id)
{
    size_t idx;
    const zb_dev_t *d;
    err_t e;

    if (lookup(device_id, &idx) != ERR_OK) {
        return ERR_NOENT;
    }
    d = zb_host_device_at(idx);
    if (d == NULL) {
        return ERR_NOENT;
    }
    e = zb_leave(d->ieee);
    if (e == ERR_OK) {
        bump();
    }
    return e;
}

void home_net(home_net_t *out)
{
    zb_net_info_t n;

    if (out == NULL) {
        return;
    }
    memset(out, 0, sizeof(*out));
    zb_host_net(&n);
    out->formed = n.formed;
    out->channel = n.channel;
    out->pan = n.pan;
    out->radio_ok = n.radio_ok;
    out->persist_ok = n.persist_ok;
    out->mock = n.mock;
    out->permit_left = n.permit_left;
    copy_str(out->znp_ver, sizeof(out->znp_ver), n.znp_ver);
}

uint32_t home_gen(void)
{
    return g_gen;
}

const char *home_cluster_text(home_kind_t kind)
{
    if (kind == HOME_LIGHT) {
        return "OnOff, Level";
    }
    if (kind == HOME_SWITCH) {
        return "OnOff";
    }
    if (kind == HOME_BINARY_SENSOR) {
        return "Occupancy";
    }
    return "Temperature";
}

uint32_t home_now_ms(void)
{
    return zb_host_now_ms();
}

uint8_t home_bar_level(void)
{
    home_net_t n;

    home_net(&n);
    if (n.radio_ok != 0u && n.formed != 0u) {
        return 3u;
    }
    if (zb_host_device_count() > 0u) {
        return 2u;
    }
    return 1u;
}

err_t home_test_announce(uint16_t nwk, const uint8_t ieee[8])
{
    err_t e = zb_host_apply_announce(nwk, ieee);
    if (e == ERR_OK) {
        bump();
    }
    return e;
}

err_t home_test_clusters(const uint8_t ieee[8], const uint16_t *in, uint8_t n)
{
    err_t e = zb_host_apply_clusters(ieee, in, n);
    if (e == ERR_OK) {
        bump();
    }
    return e;
}

err_t home_test_report(const uint8_t ieee[8], uint16_t cluster, uint8_t on, uint8_t level)
{
    size_t idx;
    err_t e = zb_host_apply_report(ieee, cluster, on, level);
    if (e == ERR_OK && zb_host_find(ieee, &idx) == ERR_OK) {
        bump();
        notify_at(idx);
    }
    return e;
}

void home_test_force_radio(uint8_t radio_ok, uint8_t mock)
{
    zb_host_test_set_flags(radio_ok, mock);
}
