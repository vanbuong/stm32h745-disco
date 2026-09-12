#include "ui/shell.h"

#include "app/apps.h"
#include "svc/audio.h"
#include "svc/cfg.h"
#include "svc/health.h"
#include "svc/home.h"
#include "svc/net.h"
#include "svc/time.h"
#include "ui/nav.h"

#include <stddef.h>

static nav_stack_t g_nav;
static shell_status_t g_status;
static uint32_t g_now_ms;
static const ui_app_t *g_top_app;

static void stop_top(void)
{
    if (g_top_app != NULL && g_top_app->on_stop != NULL) {
        g_top_app->on_stop();
    }
    g_top_app = NULL;
}

static void start_top(void)
{
    const nav_frame_t *f = nav_top(&g_nav);

    g_top_app = NULL;
    if (f == NULL) {
        return;
    }
    g_top_app = apps_find(f->id);
    if (g_top_app != NULL && g_top_app->on_start != NULL) {
        g_top_app->on_start(f->args);
    }
}

void shell_init(void)
{
    nav_init(&g_nav);
    apps_init();
    g_status.hour = 0u;
    g_status.min = 0u;
    g_status.storage_ok = 0u;
    g_status.m4 = 0u;
    g_status.net = 0u;
    g_status.zb = 1u;
    g_now_ms = 0u;
    g_top_app = NULL;
}

err_t shell_push(const char *app_id, void *args)
{
    const ui_app_t *app;
    err_t e;

    app = apps_find(app_id);
    if (app == NULL) {
        return ERR_NOENT;
    }
    stop_top();
    e = nav_push(&g_nav, app->id, args);
    if (e != ERR_OK) {
        start_top();
        return e;
    }
    start_top();
    return ERR_OK;
}

err_t shell_pop(void)
{
    if (g_nav.depth == 0u) {
        return ERR_OK;
    }
    stop_top();
    (void)nav_pop(&g_nav);
    start_top();
    return ERR_OK;
}

void shell_home(void)
{
    if (g_nav.depth == 0u) {
        return;
    }
    stop_top();
    nav_home(&g_nav);
    g_top_app = NULL;
}

void shell_tick(uint32_t dt_ms)
{
    uint8_t hh;
    uint8_t mm;
    uint8_t ss;

    g_now_ms += dt_ms;
    time_poll(dt_ms);
    if (time_rtc_get(&hh, &mm, &ss) == ERR_OK) {
        g_status.min = mm;
        g_status.hour = hh;
    }
    net_service_poll(g_now_ms);
    g_status.net = net_bar_level();
    home_poll(dt_ms);
    g_status.zb = home_bar_level();
    health_kick();
    health_note_peer(g_status.m4);
    health_poll(dt_ms);
    cfg_poll();
    if (g_top_app != NULL && g_top_app->on_tick != NULL) {
        g_top_app->on_tick(dt_ms);
    }
    audio_poll(g_now_ms);
}

const char *shell_top_id(void)
{
    const nav_frame_t *f = nav_top(&g_nav);

    return (f != NULL) ? f->id : NULL;
}

const char *shell_top_title(void)
{
    return (g_top_app != NULL) ? g_top_app->title : NULL;
}

void *shell_top_args(void)
{
    const nav_frame_t *f = nav_top(&g_nav);

    return (f != NULL) ? f->args : NULL;
}

int shell_depth(void)
{
    return (int)g_nav.depth;
}

uint32_t shell_nav_gen(void)
{
    return g_nav.gen;
}

const shell_status_t *shell_status(void)
{
    return &g_status;
}

void shell_status_set_storage(uint8_t ok)
{
    g_status.storage_ok = (ok != 0u) ? 1u : 0u;
}

void shell_status_set_m4(uint8_t ok)
{
    g_status.m4 = (ok != 0u) ? 1u : 0u;
}
