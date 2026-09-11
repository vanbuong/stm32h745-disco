#include "app/apps.h"

#include <stddef.h>
#include <string.h>

static char g_files_row[APP_FILES_STUB_ROWS][12];
static unsigned g_files_n;

static void stub_start(void *args)
{
    (void)args;
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
    unsigned i;

    (void)args;
    g_files_n = APP_FILES_STUB_ROWS;
    for (i = 0u; i < APP_FILES_STUB_ROWS; i++) {
        g_files_row[i][0] = 'I';
        g_files_row[i][1] = 't';
        g_files_row[i][2] = 'e';
        g_files_row[i][3] = 'm';
        g_files_row[i][4] = ' ';
        g_files_row[i][5] = (char)('0' + (i / 10u));
        g_files_row[i][6] = (char)('0' + (i % 10u));
        g_files_row[i][7] = '\0';
    }
}

static void files_stop(void)
{
    g_files_n = 0u;
}

static const ui_app_t g_apps[] = {
    {APP_ID_FILES, "Files", "files", files_start, files_stop, stub_tick, stub_event},
    {APP_ID_HOME, "Home", "home", stub_start, stub_stop, stub_tick, stub_event},
    {APP_ID_GAME, "Game", "game", stub_start, stub_stop, stub_tick, stub_event},
    {APP_ID_PLAYER, "Music", "player", stub_start, stub_stop, stub_tick, stub_event},
    {APP_ID_NETWORK, "Network", "network", stub_start, stub_stop, stub_tick, stub_event},
    {APP_ID_SETTINGS, "Settings", "settings", stub_start, stub_stop, stub_tick, stub_event},
};

void apps_init(void)
{
    g_files_n = 0u;
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

unsigned app_files_stub_count(void)
{
    return g_files_n;
}

const char *app_files_stub_row(unsigned i)
{
    if (i >= g_files_n) {
        return NULL;
    }
    return g_files_row[i];
}
