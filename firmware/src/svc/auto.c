#include "svc/auto.h"

#include <string.h>

static auto_rule_t g_rules[AUTO_RULE_MAX];
static uint16_t g_n;
static uint16_t g_last_id;

void auto_reset(void)
{
    g_n = 0;
    g_last_id = 0;
    memset(g_rules, 0, sizeof(g_rules));
}

err_t auto_add(const auto_rule_t *r)
{
    if (r == NULL) {
        return ERR_INVAL;
    }
    if (g_n >= AUTO_RULE_MAX) {
        return ERR_NOSPC;
    }
    g_rules[g_n++] = *r;
    return ERR_OK;
}

static int ieee_eq(const uint8_t a[8], const uint8_t b[8])
{
    return memcmp(a, b, 8) == 0;
}

err_t auto_eval(const home_device_t *changed)
{
    uint16_t i;
    if (changed == NULL) {
        return ERR_INVAL;
    }
    for (i = 0; i < g_n; i++) {
        auto_rule_t *r = &g_rules[i];
        int match = 0;
        if (!r->enabled) {
            continue;
        }
        if (!ieee_eq(r->trig_ieee, changed->ieee)) {
            continue;
        }
        if ((r->trig == AUTO_TRIG_ON || r->trig == AUTO_TRIG_OCCUPIED) && changed->on) {
            match = 1;
        }
        if (match) {
            g_last_id = r->id;
            return ERR_OK;
        }
    }
    return ERR_NOENT;
}

uint16_t auto_last_id(void)
{
    return g_last_id;
}
