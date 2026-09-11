#include "unity.h"

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
    TEST_ASSERT_TRUE(s.depth == 0u);
    TEST_ASSERT_TRUE(nav_top(&s) == NULL);
    TEST_ASSERT_TRUE(nav_pop(&s) == ERR_NOENT);
    TEST_ASSERT_TRUE(nav_push(NULL, "files", NULL) == ERR_INVAL);
    TEST_ASSERT_TRUE(nav_push(&s, NULL, NULL) == ERR_INVAL);
    TEST_ASSERT_TRUE(nav_push(&s, "", NULL) == ERR_INVAL);

    TEST_ASSERT_TRUE(nav_push(&s, "files", NULL) == ERR_OK);
    TEST_ASSERT_TRUE(s.depth == 1u);
    top = nav_top(&s);
    TEST_ASSERT_TRUE(top != NULL);
    TEST_ASSERT_TRUE(strcmp(top->id, "files") == 0);

    TEST_ASSERT_TRUE(nav_push(&s, "text", (void *)(uintptr_t)1) == ERR_OK);
    TEST_ASSERT_TRUE(s.depth == 2u);
    TEST_ASSERT_TRUE(strcmp(nav_top(&s)->id, "text") == 0);

    TEST_ASSERT_TRUE(nav_pop(&s) == ERR_OK);
    TEST_ASSERT_TRUE(strcmp(nav_top(&s)->id, "files") == 0);

    nav_home(&s);
    TEST_ASSERT_TRUE(s.depth == 0u);
    TEST_ASSERT_TRUE(nav_top(&s) == NULL);
}

static void test_nav_overflow(void)
{
    nav_stack_t s;
    unsigned i;

    nav_init(&s);
    for (i = 0u; i < NAV_STACK_MAX; i++) {
        TEST_ASSERT_TRUE(nav_push(&s, "files", NULL) == ERR_OK);
    }
    TEST_ASSERT_TRUE(nav_push(&s, "home", NULL) == ERR_NOSPC);
    TEST_ASSERT_TRUE(s.depth == NAV_STACK_MAX);
}

static void test_launcher_geom(void)
{
    ui_rect_t r;
    unsigned i;
    int hit;

    launcher_tile_rect(99u, &r);
    TEST_ASSERT_TRUE(r.w == 0u);

    for (i = 0u; i < LAUNCHER_COUNT; i++) {
        launcher_tile_rect(i, &r);
        TEST_ASSERT_TRUE(r.w == LAUNCHER_TILE_PX);
        TEST_ASSERT_TRUE(r.h == LAUNCHER_TILE_PX);
        TEST_ASSERT_TRUE(r.w >= THEME_HIT_MIN_PX);
        TEST_ASSERT_TRUE(r.h >= THEME_HIT_MIN_PX);
        TEST_ASSERT_TRUE(r.y >= THEME_STATUS_H);
        TEST_ASSERT_TRUE((uint32_t)r.x + r.w <= THEME_PANEL_W);
        TEST_ASSERT_TRUE((uint32_t)r.y + r.h <= THEME_PANEL_H);
        hit = launcher_hit((int16_t)(r.x + r.w / 2u), (int16_t)(r.y + r.h / 2u));
        TEST_ASSERT_TRUE(hit == (int)i);
    }

    TEST_ASSERT_TRUE(launcher_hit(0, 0) == -1);
    TEST_ASSERT_TRUE(launcher_hit(240, 8) == -1);
    TEST_ASSERT_TRUE(launcher_hit(-1, 100) == -1);
}

static void test_shell_nav(void)
{
    unsigned i;

    shell_init();
    TEST_ASSERT_TRUE(shell_depth() == 0);
    TEST_ASSERT_TRUE(shell_top_id() == NULL);
    TEST_ASSERT_TRUE(shell_pop() == ERR_OK);
    TEST_ASSERT_TRUE(shell_push("nope", NULL) == ERR_NOENT);
    TEST_ASSERT_TRUE(apps_count() >= LAUNCHER_COUNT);
    TEST_ASSERT_TRUE(apps_find(APP_ID_FILES) != NULL);
    TEST_ASSERT_TRUE(apps_find(APP_ID_TEXT) != NULL);
    TEST_ASSERT_TRUE(apps_find(APP_ID_IMAGE) != NULL);
    TEST_ASSERT_TRUE(apps_find(NULL) == NULL);
    TEST_ASSERT_TRUE(apps_at(99u) == NULL);

    TEST_ASSERT_TRUE(vfs_mount() == ERR_OK);
    TEST_ASSERT_TRUE(shell_push(APP_ID_FILES, NULL) == ERR_OK);
    TEST_ASSERT_TRUE(shell_depth() == 1);
    TEST_ASSERT_TRUE(strcmp(shell_top_id(), APP_ID_FILES) == 0);
    TEST_ASSERT_TRUE(strcmp(shell_top_title(), "Files") == 0);
    TEST_ASSERT_TRUE(files_state() == FILES_ST_OK);
    TEST_ASSERT_TRUE(files_count() >= 3u);
    TEST_ASSERT_TRUE(strcmp(files_cwd(), "/user") == 0);
    TEST_ASSERT_TRUE(files_row(0) != NULL);
    TEST_ASSERT_TRUE(files_row(99u) == NULL);

    TEST_ASSERT_TRUE(shell_push(APP_ID_GAME, NULL) == ERR_OK);
    TEST_ASSERT_TRUE(strcmp(shell_top_id(), APP_ID_GAME) == 0);
    TEST_ASSERT_TRUE(shell_pop() == ERR_OK);
    TEST_ASSERT_TRUE(strcmp(shell_top_id(), APP_ID_FILES) == 0);

    shell_home();
    TEST_ASSERT_TRUE(shell_depth() == 0);
    TEST_ASSERT_TRUE(shell_top_id() == NULL);

    for (i = 0u; i < LAUNCHER_COUNT; i++) {
        const ui_app_t *a = apps_at(i);
        TEST_ASSERT_TRUE(a != NULL);
        TEST_ASSERT_TRUE(shell_push(a->id, NULL) == ERR_OK);
        TEST_ASSERT_TRUE(strcmp(shell_top_id(), a->id) == 0);
        TEST_ASSERT_TRUE(shell_pop() == ERR_OK);
        TEST_ASSERT_TRUE(shell_top_id() == NULL);
    }
}

static void test_shell_status(void)
{
    const shell_status_t *st;

    shell_init();
    shell_status_set_storage(1u);
    st = shell_status();
    TEST_ASSERT_TRUE(st != NULL);
    TEST_ASSERT_TRUE(st->storage_ok == 1u);
    TEST_ASSERT_TRUE(st->m4 == 0u);
    TEST_ASSERT_TRUE(st->hour == 0u);
    shell_status_set_m4(1u);
    TEST_ASSERT_TRUE(shell_status()->m4 == 1u);
    shell_tick(60000u);
    TEST_ASSERT_TRUE(shell_status()->min == 1u);
    shell_tick(60u * 60000u);
    TEST_ASSERT_TRUE(shell_status()->hour == 1u);
}

void test_shell_run(void)
{
    UnitySetTestFile(__FILE__);
    RUN_TEST(test_nav_stack);
    RUN_TEST(test_nav_overflow);
    RUN_TEST(test_launcher_geom);
    RUN_TEST(test_shell_nav);
    RUN_TEST(test_shell_status);
}
