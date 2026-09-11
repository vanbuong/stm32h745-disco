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
    TEST_ASSERT_EQUAL_UINT8(0u, s.depth);
    TEST_ASSERT_NULL(nav_top(&s));
    TEST_ASSERT_EQUAL_INT(ERR_NOENT, nav_pop(&s));
    TEST_ASSERT_EQUAL_INT(ERR_INVAL, nav_push(NULL, "files", NULL));
    TEST_ASSERT_EQUAL_INT(ERR_INVAL, nav_push(&s, NULL, NULL));
    TEST_ASSERT_EQUAL_INT(ERR_INVAL, nav_push(&s, "", NULL));

    TEST_ASSERT_EQUAL_INT(ERR_OK, nav_push(&s, "files", NULL));
    TEST_ASSERT_EQUAL_UINT8(1u, s.depth);
    top = nav_top(&s);
    TEST_ASSERT_NOT_NULL(top);
    TEST_ASSERT_EQUAL_STRING("files", top->id);

    TEST_ASSERT_EQUAL_INT(ERR_OK, nav_push(&s, "text", (void *)(uintptr_t)1));
    TEST_ASSERT_EQUAL_UINT8(2u, s.depth);
    TEST_ASSERT_EQUAL_STRING("text", nav_top(&s)->id);

    TEST_ASSERT_EQUAL_INT(ERR_OK, nav_pop(&s));
    TEST_ASSERT_EQUAL_STRING("files", nav_top(&s)->id);

    nav_home(&s);
    TEST_ASSERT_EQUAL_UINT8(0u, s.depth);
    TEST_ASSERT_NULL(nav_top(&s));
}

static void test_nav_overflow(void)
{
    nav_stack_t s;
    unsigned i;

    nav_init(&s);
    for (i = 0u; i < NAV_STACK_MAX; i++) {
        TEST_ASSERT_EQUAL_INT(ERR_OK, nav_push(&s, "files", NULL));
    }
    TEST_ASSERT_EQUAL_INT(ERR_NOSPC, nav_push(&s, "home", NULL));
    TEST_ASSERT_EQUAL_UINT8(NAV_STACK_MAX, s.depth);
}

static void test_launcher_geom(void)
{
    ui_rect_t r;
    unsigned i;
    int hit;

    launcher_tile_rect(99u, &r);
    TEST_ASSERT_EQUAL_UINT16(0u, r.w);

    for (i = 0u; i < LAUNCHER_COUNT; i++) {
        launcher_tile_rect(i, &r);
        TEST_ASSERT_EQUAL_UINT16(LAUNCHER_TILE_PX, r.w);
        TEST_ASSERT_EQUAL_UINT16(LAUNCHER_TILE_PX, r.h);
        TEST_ASSERT_GREATER_OR_EQUAL_UINT16(THEME_HIT_MIN_PX, r.w);
        TEST_ASSERT_GREATER_OR_EQUAL_UINT16(THEME_HIT_MIN_PX, r.h);
        TEST_ASSERT_GREATER_OR_EQUAL_UINT16(THEME_STATUS_H, r.y);
        TEST_ASSERT_LESS_OR_EQUAL_UINT32(THEME_PANEL_W, (uint32_t)r.x + r.w);
        TEST_ASSERT_LESS_OR_EQUAL_UINT32(THEME_PANEL_H, (uint32_t)r.y + r.h);
        hit = launcher_hit((int16_t)(r.x + r.w / 2u), (int16_t)(r.y + r.h / 2u));
        TEST_ASSERT_EQUAL_INT((int)i, hit);
    }

    TEST_ASSERT_EQUAL_INT(-1, launcher_hit(0, 0));
    TEST_ASSERT_EQUAL_INT(-1, launcher_hit(240, 8));
    TEST_ASSERT_EQUAL_INT(-1, launcher_hit(-1, 100));
}

static void test_shell_nav(void)
{
    unsigned i;

    shell_init();
    TEST_ASSERT_EQUAL_INT(0, shell_depth());
    TEST_ASSERT_NULL(shell_top_id());
    TEST_ASSERT_EQUAL_INT(ERR_OK, shell_pop());
    TEST_ASSERT_EQUAL_INT(ERR_NOENT, shell_push("nope", NULL));
    TEST_ASSERT_GREATER_OR_EQUAL_UINT(LAUNCHER_COUNT, apps_count());
    TEST_ASSERT_NOT_NULL(apps_find(APP_ID_FILES));
    TEST_ASSERT_NOT_NULL(apps_find(APP_ID_TEXT));
    TEST_ASSERT_NOT_NULL(apps_find(APP_ID_IMAGE));
    TEST_ASSERT_NULL(apps_find(NULL));
    TEST_ASSERT_NULL(apps_at(99u));

    TEST_ASSERT_EQUAL_INT(ERR_OK, vfs_mount());
    TEST_ASSERT_EQUAL_INT(ERR_OK, shell_push(APP_ID_FILES, NULL));
    TEST_ASSERT_EQUAL_INT(1, shell_depth());
    TEST_ASSERT_EQUAL_STRING(APP_ID_FILES, shell_top_id());
    TEST_ASSERT_EQUAL_STRING("Files", shell_top_title());
    TEST_ASSERT_EQUAL_INT(FILES_ST_OK, files_state());
    TEST_ASSERT_GREATER_OR_EQUAL_UINT(3u, files_count());
    TEST_ASSERT_EQUAL_STRING("/user", files_cwd());
    TEST_ASSERT_NOT_NULL(files_row(0));
    TEST_ASSERT_NULL(files_row(99u));

    TEST_ASSERT_EQUAL_INT(ERR_OK, shell_push(APP_ID_GAME, NULL));
    TEST_ASSERT_EQUAL_STRING(APP_ID_GAME, shell_top_id());
    TEST_ASSERT_EQUAL_INT(ERR_OK, shell_pop());
    TEST_ASSERT_EQUAL_STRING(APP_ID_FILES, shell_top_id());

    shell_home();
    TEST_ASSERT_EQUAL_INT(0, shell_depth());
    TEST_ASSERT_NULL(shell_top_id());

    for (i = 0u; i < LAUNCHER_COUNT; i++) {
        const ui_app_t *a = apps_at(i);
        TEST_ASSERT_NOT_NULL(a);
        TEST_ASSERT_EQUAL_INT(ERR_OK, shell_push(a->id, NULL));
        TEST_ASSERT_EQUAL_STRING(a->id, shell_top_id());
        TEST_ASSERT_EQUAL_INT(ERR_OK, shell_pop());
        TEST_ASSERT_NULL(shell_top_id());
    }
}

static void test_shell_status(void)
{
    const shell_status_t *st;

    shell_init();
    shell_status_set_storage(1u);
    st = shell_status();
    TEST_ASSERT_NOT_NULL(st);
    TEST_ASSERT_EQUAL_UINT8(1u, st->storage_ok);
    TEST_ASSERT_EQUAL_UINT8(0u, st->m4);
    TEST_ASSERT_EQUAL_UINT8(0u, st->hour);
    shell_status_set_m4(1u);
    TEST_ASSERT_EQUAL_UINT8(1u, shell_status()->m4);
    shell_tick(60000u);
    TEST_ASSERT_EQUAL_UINT8(1u, shell_status()->min);
    shell_tick(60u * 60000u);
    TEST_ASSERT_EQUAL_UINT8(1u, shell_status()->hour);
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
