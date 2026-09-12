#include "app/apps.h"

#include "app/calendar.h"
#include "app/files.h"
#include "app/game.h"
#include "app/image_view.h"
#include "app/network.h"
#include "app/player.h"
#include "svc/text_view.h"

#include <stddef.h>
#include <string.h>

static const char *g_view_path;

static void stub_start(void *args)
{
    (void)args;
    g_view_path = NULL;
}

static void stub_stop(void)
{
}

static void stub_tick(uint32_t dt_ms)
{
    (void)dt_ms;
}

static void stub_event(const ui_event_t *e)
{
    (void)e;
}

static void files_start(void *args)
{
    (void)args;
    files_load();
}

static void files_stop(void)
{
}

static void player_start(void *args)
{
    g_view_path = (const char *)args;
    if (g_view_path != NULL && g_view_path[0] != '\0') {
        (void)player_open(g_view_path);
    } else {
        (void)player_open("/user/demo.mp3");
    }
}

static void player_stop(void)
{
    player_close();
}

static void text_start(void *args)
{
    g_view_path = (const char *)args;
    if (g_view_path != NULL) {
        (void)text_view_open(g_view_path);
    }
}

static void text_stop(void)
{
    text_view_close();
}

static void image_start(void *args)
{
    g_view_path = (const char *)args;
    if (g_view_path != NULL) {
        (void)image_view_open(g_view_path);
    }
}

static void image_stop(void)
{
    image_view_close();
}

static void network_start(void *args)
{
    (void)args;
    network_refresh();
}

static void network_tick(uint32_t dt_ms)
{
    (void)dt_ms;
    network_refresh();
}

static void settings_start(void *args)
{
    (void)args;
    network_refresh();
}

static void calendar_start(void *args)
{
    (void)args;
    calendar_open();
}

static void calendar_stop(void)
{
    calendar_close();
}

static void game_start(void *args)
{
    (void)args;
    game_open(0, 0);
}

static void game_stop(void)
{
    game_close();
}

static void game_on_tick(uint32_t dt_ms)
{
    game_step(dt_ms);
}

static const ui_app_t g_apps[] = {
    {APP_ID_FILES, "Files", "files", files_start, files_stop, stub_tick, stub_event},
    {APP_ID_HOME, "Home", "home", stub_start, stub_stop, stub_tick, stub_event},
    {APP_ID_GAME, "Game", "game", game_start, game_stop, game_on_tick, stub_event},
    {APP_ID_PLAYER, "Music", "player", player_start, player_stop, stub_tick, stub_event},
    {APP_ID_CALENDAR, "Calendar", "calendar", calendar_start, calendar_stop, stub_tick, stub_event},
    {APP_ID_SETTINGS, "Settings", "settings", settings_start, stub_stop, network_tick, stub_event},
    {APP_ID_TEXT, "Text", "text", text_start, text_stop, stub_tick, stub_event},
    {APP_ID_IMAGE, "Image", "image", image_start, image_stop, stub_tick, stub_event},
    {APP_ID_NETWORK, "Network", "network", network_start, stub_stop, network_tick, stub_event},
};

void apps_init(void)
{
    g_view_path = NULL;
    files_reset();
}

const ui_app_t *apps_find(const char *id)
{
    unsigned i;

    if (id == NULL) {
        return NULL;
    }
    for (i = 0u; i < (unsigned)(sizeof(g_apps) / sizeof(g_apps[0])); i++) {
        if (strcmp(g_apps[i].id, id) == 0) {
            return &g_apps[i];
        }
    }
    return NULL;
}

const ui_app_t *apps_at(unsigned index)
{
    if (index >= (unsigned)(sizeof(g_apps) / sizeof(g_apps[0]))) {
        return NULL;
    }
    return &g_apps[index];
}

unsigned apps_count(void)
{
    return (unsigned)(sizeof(g_apps) / sizeof(g_apps[0]));
}

const char *app_view_path(void)
{
    return g_view_path;
}
