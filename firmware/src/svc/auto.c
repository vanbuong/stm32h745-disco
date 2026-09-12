#include "svc/auto.h"

#include "svc/time.h"
#include "svc/vfs.h"

#include <string.h>

#define RULE_HDR 6u
#define RULE_REC 51u
#define RULE_STORE_MAX 4u

typedef struct {
    uint8_t used;
    uint8_t ieee[8];
    home_cmd_t cmd;
    uint16_t rule_id;
    uint32_t due_ms;
    uint8_t toggle;
} pend_t;

static auto_rule_t g_rules[AUTO_RULE_MAX];
static uint8_t g_latched[AUTO_RULE_MAX];
static uint8_t g_time_fired[AUTO_RULE_MAX];
static pend_t g_pend[AUTO_PEND_MAX];
static uint16_t g_n;
static uint16_t g_last_id;
static uint16_t g_next_id;
static uint32_t g_now_ms;
static uint8_t g_dirty;
static uint8_t g_ready;
static int g_test_min = -1;

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

static int ieee_eq(const uint8_t a[8], const uint8_t b[8])
{
    return memcmp(a, b, 8u) == 0;
}

static void put_le16(uint8_t *p, uint16_t v)
{
    p[0] = (uint8_t)v;
    p[1] = (uint8_t)(v >> 8);
}

static uint16_t get_le16(const uint8_t *p)
{
    return (uint16_t)((uint16_t)p[0] | ((uint16_t)p[1] << 8));
}

static void put_le32(uint8_t *p, uint32_t v)
{
    p[0] = (uint8_t)v;
    p[1] = (uint8_t)(v >> 8);
    p[2] = (uint8_t)(v >> 16);
    p[3] = (uint8_t)(v >> 24);
}

static uint32_t get_le32(const uint8_t *p)
{
    return (uint32_t)p[0] | ((uint32_t)p[1] << 8) | ((uint32_t)p[2] << 16) | ((uint32_t)p[3] << 24);
}

static void mark_dirty(void)
{
    g_dirty = 1u;
}

static err_t write_file(const char *path, const uint8_t *buf, size_t n)
{
    vfs_file_t fd = -1;
    size_t put = 0u;
    err_t e;

    if (vfs_mounted() == 0) {
        return ERR_IO;
    }
    e = vfs_mkdir("/user/home");
    if (e != ERR_OK && e != ERR_DENIED) {
        return e;
    }
    e = vfs_open(path, VFS_O_WR | VFS_O_CREAT | VFS_O_TRUNC, &fd);
    if (e != ERR_OK) {
        return e;
    }
    e = vfs_write(fd, buf, n, &put);
    (void)vfs_close(fd);
    if (e != ERR_OK || put != n) {
        return (e != ERR_OK) ? e : ERR_NOSPC;
    }
    return ERR_OK;
}

static void pack_rule(uint8_t *p, const auto_rule_t *r)
{
    put_le16(p, r->id);
    p[2] = r->enabled;
    memcpy(p + 3u, r->name, HOME_NAME_MAX);
    memcpy(p + 27u, r->trig_ieee, 8u);
    p[35] = (uint8_t)r->trig;
    put_le16(p + 36u, (uint16_t)r->thresh);
    memcpy(p + 38u, r->action_ieee, 8u);
    p[46] = (uint8_t)r->action;
    put_le32(p + 47u, r->delay_ms);
}

static void unpack_rule(auto_rule_t *r, const uint8_t *p)
{
    memset(r, 0, sizeof(*r));
    r->id = get_le16(p);
    r->enabled = p[2];
    memcpy(r->name, p + 3u, HOME_NAME_MAX);
    r->name[HOME_NAME_MAX - 1u] = '\0';
    memcpy(r->trig_ieee, p + 27u, 8u);
    r->trig = (auto_trig_t)p[35];
    r->thresh = (int16_t)get_le16(p + 36u);
    memcpy(r->action_ieee, p + 38u, 8u);
    r->action = (auto_act_t)p[46];
    r->delay_ms = get_le32(p + 47u);
}

static void persist_save(void)
{
    uint8_t buf[RULE_HDR + (RULE_REC * RULE_STORE_MAX)];
    size_t count = g_n;
    size_t i;
    size_t nwrite;

    if (count > RULE_STORE_MAX) {
        count = RULE_STORE_MAX;
    }
    nwrite = RULE_HDR + (count * RULE_REC);
    memset(buf, 0, sizeof(buf));
    buf[0] = (uint8_t)'A';
    buf[1] = (uint8_t)'R';
    buf[2] = (uint8_t)'U';
    buf[3] = (uint8_t)'L';
    buf[4] = 1u;
    buf[5] = (uint8_t)count;
    for (i = 0u; i < count; i++) {
        pack_rule(buf + RULE_HDR + (i * RULE_REC), &g_rules[i]);
    }
    if (write_file(AUTO_RULES_PATH, buf, nwrite) == ERR_OK) {
        g_dirty = 0u;
    }
}

static uint8_t persist_load(void)
{
    uint8_t buf[RULE_HDR + (RULE_REC * RULE_STORE_MAX)];
    vfs_file_t fd = -1;
    size_t got = 0u;
    size_t i;
    uint8_t count;

    if (vfs_mounted() == 0) {
        return 0u;
    }
    if (vfs_open(AUTO_RULES_PATH, VFS_O_RD, &fd) != ERR_OK) {
        return 0u;
    }
    if (vfs_read(fd, buf, sizeof(buf), &got) != ERR_OK) {
        (void)vfs_close(fd);
        return 0u;
    }
    (void)vfs_close(fd);
    if (got < RULE_HDR || buf[0] != (uint8_t)'A' || buf[1] != (uint8_t)'R' ||
        buf[2] != (uint8_t)'U' || buf[3] != (uint8_t)'L' || buf[4] != 1u) {
        return 0u;
    }
    count = buf[5];
    if (count > RULE_STORE_MAX || RULE_HDR + ((size_t)count * RULE_REC) > got) {
        return 0u;
    }
    g_n = 0u;
    g_next_id = 1u;
    for (i = 0u; i < (size_t)count; i++) {
        unpack_rule(&g_rules[i], buf + RULE_HDR + (i * RULE_REC));
        if (g_rules[i].id >= g_next_id) {
            g_next_id = (uint16_t)(g_rules[i].id + 1u);
        }
        g_n++;
    }
    return (g_n > 0u) ? 1u : 0u;
}

static void seed_motion(void)
{
    auto_rule_t r;

    memset(&r, 0, sizeof(r));
    r.id = 1u;
    r.enabled = 1u;
    copy_str(r.name, HOME_NAME_MAX, "Motion light");
    r.trig_ieee[0] = 0x01u;
    r.trig_ieee[7] = 0x04u;
    r.trig = AUTO_TRIG_OCCUPIED;
    r.action_ieee[0] = 0x01u;
    r.action_ieee[7] = 0x01u;
    r.action = AUTO_ACT_ON;
    r.delay_ms = 3000u;
    g_rules[0] = r;
    g_n = 1u;
    g_next_id = 2u;
    mark_dirty();
}

static err_t find_id(uint16_t id, size_t *idx)
{
    size_t i;

    for (i = 0u; i < g_n; i++) {
        if (g_rules[i].id == id) {
            if (idx != NULL) {
                *idx = i;
            }
            return ERR_OK;
        }
    }
    return ERR_NOENT;
}

static void cmd_from_act(auto_act_t act, home_cmd_t *cmd)
{
    memset(cmd, 0, sizeof(*cmd));
    cmd->has_level = 0u;
    if (act == AUTO_ACT_OFF) {
        cmd->on = 0u;
    } else {
        cmd->on = 1u;
    }
}

static void cancel_pending(const uint8_t ieee[8])
{
    size_t i;

    for (i = 0u; i < AUTO_PEND_MAX; i++) {
        if (g_pend[i].used != 0u && ieee_eq(g_pend[i].ieee, ieee) != 0) {
            g_pend[i].used = 0u;
        }
    }
}

static err_t queue_act(const uint8_t ieee[8], auto_act_t act, uint32_t due, uint16_t rule_id)
{
    size_t i;

    for (i = 0u; i < AUTO_PEND_MAX; i++) {
        if (g_pend[i].used == 0u) {
            g_pend[i].used = 1u;
            memcpy(g_pend[i].ieee, ieee, 8u);
            cmd_from_act(act, &g_pend[i].cmd);
            g_pend[i].rule_id = rule_id;
            g_pend[i].due_ms = due;
            g_pend[i].toggle = (act == AUTO_ACT_TOGGLE) ? 1u : 0u;
            return ERR_OK;
        }
    }
    return ERR_NOSPC;
}

static void fire_rule(const auto_rule_t *r)
{
    uint32_t due = g_now_ms;

    cancel_pending(r->action_ieee);
    if (r->action == AUTO_ACT_ON) {
        (void)queue_act(r->action_ieee, AUTO_ACT_ON, g_now_ms, r->id);
        if (r->delay_ms > 0u) {
            (void)queue_act(r->action_ieee, AUTO_ACT_OFF, g_now_ms + r->delay_ms, r->id);
        }
        return;
    }
    if (r->delay_ms > 0u) {
        due = g_now_ms + r->delay_ms;
    }
    (void)queue_act(r->action_ieee, r->action, due, r->id);
}

static int match_attr(const auto_rule_t *r, const home_device_t *changed, uint8_t *latched)
{
    if (r->trig == AUTO_TRIG_TIME) {
        return 0;
    }
    if (ieee_eq(r->trig_ieee, changed->ieee) == 0) {
        return 0;
    }
    if (r->trig == AUTO_TRIG_TEMP_GT) {
        if ((int16_t)changed->level > r->thresh) {
            if (*latched == 0u) {
                *latched = 1u;
                return 1;
            }
        } else {
            *latched = 0u;
        }
        return 0;
    }
    if (r->trig == AUTO_TRIG_ON || r->trig == AUTO_TRIG_OCCUPIED) {
        if (changed->on != 0u) {
            if (*latched == 0u) {
                *latched = 1u;
                return 1;
            }
        } else {
            *latched = 0u;
        }
    }
    return 0;
}

static uint16_t clock_minutes(void)
{
    uint8_t hh = 0u;
    uint8_t mm = 0u;
    uint8_t ss = 0u;

    if (g_test_min >= 0) {
        return (uint16_t)g_test_min;
    }
    if (time_rtc_get(&hh, &mm, &ss) == ERR_OK) {
        return (uint16_t)((uint16_t)hh * 60u + mm);
    }
    return 0xFFFFu;
}

void auto_reset(void)
{
    memset(g_rules, 0, sizeof(g_rules));
    memset(g_latched, 0, sizeof(g_latched));
    memset(g_time_fired, 0, sizeof(g_time_fired));
    memset(g_pend, 0, sizeof(g_pend));
    g_n = 0u;
    g_last_id = 0u;
    g_next_id = 1u;
    g_now_ms = 0u;
    g_dirty = 0u;
    g_ready = 0u;
    g_test_min = -1;
}

err_t auto_init(void)
{
    if (g_ready != 0u) {
        return ERR_OK;
    }
    memset(g_rules, 0, sizeof(g_rules));
    memset(g_latched, 0, sizeof(g_latched));
    memset(g_time_fired, 0, sizeof(g_time_fired));
    memset(g_pend, 0, sizeof(g_pend));
    g_n = 0u;
    g_last_id = 0u;
    g_next_id = 1u;
    g_now_ms = 0u;
    g_dirty = 0u;
    if (persist_load() == 0u) {
        seed_motion();
    }
    g_ready = 1u;
    if (g_dirty != 0u) {
        persist_save();
    }
    return ERR_OK;
}

void auto_poll(uint32_t dt_ms)
{
    uint16_t minutes;
    uint16_t i;

    g_now_ms += dt_ms;
    if (g_dirty != 0u) {
        persist_save();
    }
    minutes = clock_minutes();
    if (minutes == 0xFFFFu) {
        return;
    }
    for (i = 0u; i < g_n; i++) {
        auto_rule_t *r = &g_rules[i];
        if (r->enabled == 0u || r->trig != AUTO_TRIG_TIME) {
            continue;
        }
        if (r->thresh == (int16_t)minutes) {
            if (g_time_fired[i] == 0u) {
                g_time_fired[i] = 1u;
                g_last_id = r->id;
                fire_rule(r);
            }
        } else {
            g_time_fired[i] = 0u;
        }
    }
}

err_t auto_add(const auto_rule_t *r)
{
    auto_rule_t *dst;

    if (r == NULL) {
        return ERR_INVAL;
    }
    if (g_n >= AUTO_RULE_MAX) {
        return ERR_NOSPC;
    }
    dst = &g_rules[g_n];
    *dst = *r;
    if (dst->id == 0u) {
        dst->id = g_next_id++;
    } else if (dst->id >= g_next_id) {
        g_next_id = (uint16_t)(dst->id + 1u);
    }
    g_latched[g_n] = 0u;
    g_time_fired[g_n] = 0u;
    g_n++;
    mark_dirty();
    return ERR_OK;
}

err_t auto_set_enabled(uint16_t id, uint8_t on)
{
    size_t idx;

    if (find_id(id, &idx) != ERR_OK) {
        return ERR_NOENT;
    }
    g_rules[idx].enabled = (on != 0u) ? 1u : 0u;
    if (on == 0u) {
        cancel_pending(g_rules[idx].action_ieee);
    }
    mark_dirty();
    return ERR_OK;
}

err_t auto_remove(uint16_t id)
{
    size_t idx;
    size_t i;

    if (find_id(id, &idx) != ERR_OK) {
        return ERR_NOENT;
    }
    cancel_pending(g_rules[idx].action_ieee);
    for (i = idx; i + 1u < g_n; i++) {
        g_rules[i] = g_rules[i + 1u];
        g_latched[i] = g_latched[i + 1u];
        g_time_fired[i] = g_time_fired[i + 1u];
    }
    g_n--;
    memset(&g_rules[g_n], 0, sizeof(g_rules[0]));
    g_latched[g_n] = 0u;
    g_time_fired[g_n] = 0u;
    mark_dirty();
    return ERR_OK;
}

size_t auto_count(void)
{
    return g_n;
}

err_t auto_at(size_t i, auto_rule_t *out)
{
    if (out == NULL) {
        return ERR_INVAL;
    }
    if (i >= g_n) {
        return ERR_NOENT;
    }
    *out = g_rules[i];
    return ERR_OK;
}

err_t auto_get(uint16_t id, auto_rule_t *out)
{
    size_t idx;

    if (out == NULL) {
        return ERR_INVAL;
    }
    if (find_id(id, &idx) != ERR_OK) {
        return ERR_NOENT;
    }
    *out = g_rules[idx];
    return ERR_OK;
}

err_t auto_eval(const home_device_t *changed)
{
    uint16_t i;
    int hit = 0;

    if (changed == NULL) {
        return ERR_INVAL;
    }
    for (i = 0u; i < g_n; i++) {
        if (g_rules[i].enabled == 0u) {
            continue;
        }
        if (match_attr(&g_rules[i], changed, &g_latched[i]) != 0) {
            g_last_id = g_rules[i].id;
            fire_rule(&g_rules[i]);
            hit = 1;
        }
    }
    return (hit != 0) ? ERR_OK : ERR_NOENT;
}

err_t auto_take_due(auto_act_req_t *out)
{
    size_t i;
    size_t best = AUTO_PEND_MAX;
    uint32_t soon = 0xFFFFFFFFu;

    if (out == NULL) {
        return ERR_INVAL;
    }
    for (i = 0u; i < AUTO_PEND_MAX; i++) {
        if (g_pend[i].used != 0u && g_pend[i].due_ms <= g_now_ms && g_pend[i].due_ms <= soon) {
            soon = g_pend[i].due_ms;
            best = i;
        }
    }
    if (best >= AUTO_PEND_MAX) {
        return ERR_NOENT;
    }
    memcpy(out->ieee, g_pend[best].ieee, 8u);
    out->cmd = g_pend[best].cmd;
    out->rule_id = g_pend[best].rule_id;
    out->toggle = g_pend[best].toggle;
    g_pend[best].used = 0u;
    return ERR_OK;
}

uint16_t auto_last_id(void)
{
    return g_last_id;
}

void auto_test_set_minutes(int minutes)
{
    g_test_min = minutes;
}
