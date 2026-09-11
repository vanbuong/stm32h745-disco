#ifndef APPS_H
#define APPS_H

#include "ui/event.h"

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

#define APP_ID_FILES "files"
#define APP_ID_IMAGE "image"
#define APP_ID_TEXT "text"
#define APP_ID_HOME "home"
#define APP_ID_GAME "game"
#define APP_ID_PLAYER "player"
#define APP_ID_SETTINGS "settings"
#define APP_ID_NETWORK "network"

#define APP_FILES_STUB_ROWS 40u

typedef struct {
    const char *id;
    const char *title;
    const char *icon;
    void (*on_start)(void *args);
    void (*on_stop)(void);
    void (*on_tick)(uint32_t dt_ms);
    void (*on_event)(const ui_event_t *e);
} ui_app_t;

void apps_init(void);
const ui_app_t *apps_find(const char *id);
const ui_app_t *apps_at(unsigned index);
unsigned apps_count(void);

unsigned app_files_stub_count(void);
const char *app_files_stub_row(unsigned i);

#ifdef __cplusplus
}
#endif

#endif /* APPS_H */
