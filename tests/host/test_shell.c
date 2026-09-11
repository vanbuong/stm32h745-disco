#include "test.h"

#include "app/apps.h"
#include "app/files.h"
#include "svc/vfs.h"
#include "ui/launcher.h"
#include "ui/nav.h"
#include "ui/shell.h"
#include "ui/theme.h"

#include <stdint.h>
#include <string.h>

static void test_nav_stack(void)
{
    nav_stack_t s;
    const nav_frame_t *top;

    nav_init(&s);
    CHECK(s.depth == 0u);
    CHECK(nav_top(&s) == NULL);
    CHECK(nav_pop(&s) == ERR_NOENT);
    CHECK(nav_push(NULL, "files", NULL) == ERR_INVAL);
    CHECK(nav_push(&s, NULL, NULL) == ERR_INVAL);
    CHECK(nav_push(&s, "", NULL) == ERR_INVAL);

    CHECK(nav_push(&s, "files", NULL) == ERR_OK);
    CHECK(s.depth == 1u);
    top = nav_top(&s);
    CHECK(top != NULL);
    CHECK(strcmp(top->id, "files") == 0);

    CHECK(nav_push(&s, "text", (void *)(uintptr_t)1) == ERR_OK);
    CHECK(s.depth == 2u);
    CHECK(strcmp(nav_top(&s)->id, "text") == 0);

    CHECK(nav_pop(&s) == ERR_OK);
    CHECK(strcmp(nav_top(&s)->id, "files") == 0);

    nav_home(&s);
    CHECK(s.depth == 0u);
    CHECK(nav_top(&s) == NULL);
}

static void test_nav_overflow(void)
{
    nav_stack_t s;
    unsigned i;

    nav_init(&s);
    for (i = 0u; i < NAV_STACK_MAX; i++) {
        CHECK(nav_push(&s, "files", NULL) == ERR_OK);
    }
    CHECK(nav_push(&s, "home", NULL) == ERR_NOSPC);
    CHECK(s.depth == NAV_STACK_MAX);
}

static void test_launcher_geom(void)
{
    ui_rect_t r;
    unsigned i;
    int hit;

    launcher_tile_rect(99u, &r);
    CHECK(r.w == 0u);

    for (i = 0u; i < LAUNCHER_COUNT; i++) {
        launcher_tile_rect(i, &r);
        CHECK(r.w == LAUNCHER_TILE_PX);
        CHECK(r.h == LAUNCHER_TILE_PX);
        CHECK(r.w >= THEME_HIT_MIN_PX);
        CHECK(r.h >= THEME_HIT_MIN_PX);
        CHECK(r.y >= THEME_STATUS_H);
        CHECK((uint32_t)r.x + r.w <= THEME_PANEL_W);
        CHECK((uint32_t)r.y + r.h <= THEME_PANEL_H);
        hit = launcher_hit((int16_t)(r.x + r.w / 2u), (int16_t)(r.y + r.h / 2u));
        CHECK(hit == (int)i);
    }

    CHECK(launcher_hit(0, 0) == -1);
    CHECK(launcher_hit(240, 8) == -1);
    CHECK(launcher_hit(-1, 100) == -1);
}

static void test_shell_nav(void)
{
    unsigned i;

    shell_init();
    CHECK(shell_depth() == 0);
    CHECK(shell_top_id() == NULL);
    CHECK(shell_pop() == ERR_OK);
    CHECK(shell_push("nope", NULL) == ERR_NOENT);
    CHECK(apps_count() >= LAUNCHER_COUNT);
    CHECK(apps_find(APP_ID_FILES) != NULL);
    CHECK(apps_find(APP_ID_TEXT) != NULL);
    CHECK(apps_find(APP_ID_IMAGE) != NULL);
    CHECK(apps_find(NULL) == NULL);
    CHECK(apps_at(99u) == NULL);

    CHECK(vfs_mount() == ERR_OK);
    CHECK(shell_push(APP_ID_FILES, NULL) == ERR_OK);
    CHECK(shell_depth() == 1);
    CHECK(strcmp(shell_top_id(), APP_ID_FILES) == 0);
    CHECK(strcmp(shell_top_title(), "Files") == 0);
    CHECK(files_state() == FILES_ST_OK);
    CHECK(files_count() >= 3u);
    CHECK(strcmp(files_cwd(), "/user") == 0);
    CHECK(files_row(0) != NULL);
    CHECK(files_row(99u) == NULL);

    CHECK(shell_push(APP_ID_GAME, NULL) == ERR_OK);
    CHECK(strcmp(shell_top_id(), APP_ID_GAME) == 0);
    CHECK(shell_pop() == ERR_OK);
    CHECK(strcmp(shell_top_id(), APP_ID_FILES) == 0);

    shell_home();
    CHECK(shell_depth() == 0);
    CHECK(shell_top_id() == NULL);

    for (i = 0u; i < LAUNCHER_COUNT; i++) {
        const ui_app_t *a = apps_at(i);
        CHECK(a != NULL);
        CHECK(shell_push(a->id, NULL) == ERR_OK);
        CHECK(strcmp(shell_top_id(), a->id) == 0);
        CHECK(shell_pop() == ERR_OK);
        CHECK(shell_top_id() == NULL);
    }
}

static void test_shell_status(void)
{
    const shell_status_t *st;

    shell_init();
    shell_status_set_storage(1u);
    st = shell_status();
    CHECK(st != NULL);
    CHECK(st->storage_ok == 1u);
    CHECK(st->hour == 0u);
    shell_tick(60000u);
    CHECK(shell_status()->min == 1u);
    shell_tick(60u * 60000u);
    CHECK(shell_status()->hour == 1u);
}

void test_shell_run(void)
{
    test_nav_stack();
    test_nav_overflow();
    test_launcher_geom();
    test_shell_nav();
    test_shell_status();
}
