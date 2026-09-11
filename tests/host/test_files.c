#include "unity.h"

#include "app/apps.h"
#include "app/files.h"
#include "svc/vfs.h"
#include "ui/shell.h"

#include <string.h>

void vfs_ram_set_mounted(int on);

static int find_row(const char *name)
{
    unsigned i;

    for (i = 0u; i < files_count(); i++) {
        const files_row_t *r = files_row(i);
        if (r != NULL && strcmp(r->name, name) == 0) {
            return (int)i;
        }
    }
    return -1;
}

static void test_path_join_parent(void)
{
    char out[VFS_PATH_MAX];
    char tiny[4];

    TEST_ASSERT_EQUAL_INT(ERR_OK, vfs_path_join("/user", "hello.txt", out, sizeof(out)));
    TEST_ASSERT_EQUAL_STRING("/user/hello.txt", out);
    TEST_ASSERT_EQUAL_INT(ERR_OK, vfs_path_join("/user/", "sub", out, sizeof(out)));
    TEST_ASSERT_EQUAL_STRING("/user/sub", out);
    TEST_ASSERT_EQUAL_INT(ERR_DENIED, vfs_path_join("/user", "..", out, sizeof(out)));
    TEST_ASSERT_EQUAL_INT(ERR_DENIED, vfs_path_join("/user", ".", out, sizeof(out)));
    TEST_ASSERT_EQUAL_INT(ERR_DENIED, vfs_path_join("/user", "a/b", out, sizeof(out)));
    TEST_ASSERT_EQUAL_INT(ERR_DENIED, vfs_path_join("/user", "", out, sizeof(out)));
    TEST_ASSERT_EQUAL_INT(ERR_INVAL, vfs_path_join(NULL, "a", out, sizeof(out)));
    TEST_ASSERT_EQUAL_INT(ERR_NOSPC, vfs_path_join("/user", "a", tiny, sizeof(tiny)));

    TEST_ASSERT_EQUAL_INT(ERR_OK, vfs_path_parent("/user/sub/a.txt", out, sizeof(out)));
    TEST_ASSERT_EQUAL_STRING("/user/sub", out);
    TEST_ASSERT_EQUAL_INT(ERR_OK, vfs_path_parent("/user/sub", out, sizeof(out)));
    TEST_ASSERT_EQUAL_STRING("/user", out);
    TEST_ASSERT_EQUAL_INT(ERR_NOENT, vfs_path_parent("/user", out, sizeof(out)));
    TEST_ASSERT_EQUAL_INT(ERR_INVAL, vfs_path_parent(NULL, out, sizeof(out)));
    TEST_ASSERT_EQUAL_INT(ERR_DENIED, vfs_path_parent("/etc", out, sizeof(out)));
}

static void test_format_and_tags(void)
{
    char sz[16];
    files_row_t r;

    files_format_size(0u, sz, sizeof(sz));
    TEST_ASSERT_EQUAL_STRING("0 B", sz);
    files_format_size(512u, sz, sizeof(sz));
    TEST_ASSERT_EQUAL_STRING("512 B", sz);
    files_format_size(2048u, sz, sizeof(sz));
    TEST_ASSERT_EQUAL_STRING("2 KB", sz);
    files_format_size(2u * 1024u * 1024u, sz, sizeof(sz));
    TEST_ASSERT_EQUAL_STRING("2 MB", sz);
    files_format_size(1u, NULL, 4u);
    sz[0] = 'x';
    files_format_size(1u, sz, 0u);
    TEST_ASSERT_EQUAL_CHAR('x', sz[0]);

    memset(&r, 0, sizeof(r));
    r.is_dir = 1u;
    TEST_ASSERT_EQUAL_STRING("DIR", files_kind_tag(&r));
    r.is_dir = 0u;
    r.kind = MEDIA_KIND_TEXT;
    TEST_ASSERT_EQUAL_STRING("TXT", files_kind_tag(&r));
    r.kind = MEDIA_KIND_IMAGE;
    TEST_ASSERT_EQUAL_STRING("IMG", files_kind_tag(&r));
    r.kind = MEDIA_KIND_AUDIO;
    TEST_ASSERT_EQUAL_STRING("AUD", files_kind_tag(&r));
    r.kind = MEDIA_KIND_NONE;
    TEST_ASSERT_EQUAL_STRING("BIN", files_kind_tag(&r));
    TEST_ASSERT_EQUAL_STRING("BIN", files_kind_tag(NULL));
}

static void test_list_sorted(void)
{
    const files_row_t *r;

    TEST_ASSERT_EQUAL_INT(ERR_OK, vfs_mount());
    shell_init();
    files_reset();
    files_load();
    TEST_ASSERT_EQUAL_INT(FILES_ST_OK, files_state());
    TEST_ASSERT_EQUAL_STRING("/user", files_cwd());
    TEST_ASSERT_EQUAL_UINT(6u, files_count());
    TEST_ASSERT_EQUAL_INT(0, files_on_row(99u));
    TEST_ASSERT_NULL(files_row(99u));

    r = files_row(0);
    TEST_ASSERT_NOT_NULL(r);
    TEST_ASSERT_EQUAL_UINT8(1u, r->is_dir);
    TEST_ASSERT_EQUAL_STRING("sub", r->name);
    TEST_ASSERT_EQUAL_STRING("DIR", files_kind_tag(r));

    TEST_ASSERT_GREATER_THAN_INT(find_row("sub"), find_row("hello.txt"));
    TEST_ASSERT_GREATER_THAN_INT(find_row("sub"), find_row("data.bin"));
    TEST_ASSERT_GREATER_THAN_INT(find_row("data.bin"), find_row("hello.txt"));
    TEST_ASSERT_GREATER_THAN_INT(find_row("hello.txt"), find_row("photo.png"));
    TEST_ASSERT_GREATER_THAN_INT(find_row("photo.png"), find_row("readme.md"));
    TEST_ASSERT_GREATER_THAN_INT(find_row("readme.md"), find_row("song.wav"));

    r = files_row((unsigned)find_row("hello.txt"));
    TEST_ASSERT_NOT_NULL(r);
    TEST_ASSERT_EQUAL_INT(MEDIA_KIND_TEXT, r->kind);
    r = files_row((unsigned)find_row("photo.png"));
    TEST_ASSERT_NOT_NULL(r);
    TEST_ASSERT_EQUAL_INT(MEDIA_KIND_IMAGE, r->kind);
    r = files_row((unsigned)find_row("song.wav"));
    TEST_ASSERT_NOT_NULL(r);
    TEST_ASSERT_EQUAL_INT(MEDIA_KIND_AUDIO, r->kind);
    r = files_row((unsigned)find_row("data.bin"));
    TEST_ASSERT_NOT_NULL(r);
    TEST_ASSERT_EQUAL_INT(MEDIA_KIND_NONE, r->kind);
}

static void test_enter_and_back(void)
{
    int sub;
    const files_row_t *r;

    TEST_ASSERT_EQUAL_INT(ERR_OK, vfs_mount());
    shell_init();
    files_reset();
    files_load();
    files_set_scroll(40);
    TEST_ASSERT_EQUAL_INT32(40, files_scroll());
    TEST_ASSERT_EQUAL_INT(0, files_back());

    sub = find_row("sub");
    TEST_ASSERT_GREATER_OR_EQUAL_INT(0, sub);
    TEST_ASSERT_EQUAL_INT(1, files_on_row((unsigned)sub));
    TEST_ASSERT_EQUAL_STRING("/user/sub", files_cwd());
    TEST_ASSERT_EQUAL_UINT(1u, files_count());
    TEST_ASSERT_EQUAL_INT32(0, files_scroll());
    r = files_row(0);
    TEST_ASSERT_NOT_NULL(r);
    TEST_ASSERT_EQUAL_STRING("a.txt", r->name);
    TEST_ASSERT_EQUAL_UINT8(0u, r->is_dir);

    TEST_ASSERT_EQUAL_INT(1, files_back());
    TEST_ASSERT_EQUAL_STRING("/user", files_cwd());
    TEST_ASSERT_EQUAL_INT32(40, files_scroll());
    TEST_ASSERT_EQUAL_UINT(6u, files_count());
    TEST_ASSERT_GREATER_OR_EQUAL_INT(0, find_row("sub"));
}

static void test_open_with(void)
{
    int i;

    TEST_ASSERT_EQUAL_INT(ERR_OK, vfs_mount());
    shell_init();
    files_reset();
    files_load();
    TEST_ASSERT_EQUAL_INT(ERR_OK, shell_push(APP_ID_FILES, NULL));

    i = find_row("hello.txt");
    TEST_ASSERT_GREATER_OR_EQUAL_INT(0, i);
    TEST_ASSERT_EQUAL_INT(1, files_on_row((unsigned)i));
    TEST_ASSERT_EQUAL_STRING(APP_ID_TEXT, shell_top_id());
    TEST_ASSERT_EQUAL_STRING("/user/hello.txt", files_open_path());
    TEST_ASSERT_NOT_NULL(app_view_path());
    TEST_ASSERT_EQUAL_STRING("/user/hello.txt", app_view_path());
    TEST_ASSERT_EQUAL_INT(ERR_OK, shell_pop());
    TEST_ASSERT_EQUAL_STRING(APP_ID_FILES, shell_top_id());
    TEST_ASSERT_EQUAL_STRING("/user", files_cwd());

    i = find_row("readme.md");
    TEST_ASSERT_GREATER_OR_EQUAL_INT(0, i);
    TEST_ASSERT_EQUAL_INT(1, files_on_row((unsigned)i));
    TEST_ASSERT_EQUAL_STRING(APP_ID_TEXT, shell_top_id());
    TEST_ASSERT_EQUAL_INT(ERR_OK, shell_pop());

    i = find_row("photo.png");
    TEST_ASSERT_GREATER_OR_EQUAL_INT(0, i);
    TEST_ASSERT_EQUAL_INT(1, files_on_row((unsigned)i));
    TEST_ASSERT_EQUAL_STRING(APP_ID_IMAGE, shell_top_id());
    TEST_ASSERT_EQUAL_STRING("/user/photo.png", files_open_path());
    TEST_ASSERT_EQUAL_INT(ERR_OK, shell_pop());

    i = find_row("song.wav");
    TEST_ASSERT_GREATER_OR_EQUAL_INT(0, i);
    TEST_ASSERT_EQUAL_INT(1, files_on_row((unsigned)i));
    TEST_ASSERT_EQUAL_STRING(APP_ID_PLAYER, shell_top_id());
    TEST_ASSERT_EQUAL_STRING("/user/song.wav", files_open_path());
    TEST_ASSERT_EQUAL_INT(ERR_OK, shell_pop());
}

static void test_unknown_prompt(void)
{
    int i;

    TEST_ASSERT_EQUAL_INT(ERR_OK, vfs_mount());
    shell_init();
    files_reset();
    files_load();
    TEST_ASSERT_EQUAL_INT(ERR_OK, shell_push(APP_ID_FILES, NULL));

    i = find_row("data.bin");
    TEST_ASSERT_GREATER_OR_EQUAL_INT(0, i);
    TEST_ASSERT_EQUAL_INT(1, files_on_row((unsigned)i));
    TEST_ASSERT_EQUAL_INT(FILES_ST_PROMPT, files_state());
    TEST_ASSERT_EQUAL_STRING("data.bin", files_prompt_name());
    TEST_ASSERT_EQUAL_STRING("/user/data.bin", files_open_path());
    TEST_ASSERT_EQUAL_STRING(APP_ID_FILES, shell_top_id());

    files_prompt_cancel();
    TEST_ASSERT_EQUAL_INT(FILES_ST_OK, files_state());
    TEST_ASSERT_EQUAL_CHAR('\0', files_prompt_name()[0]);

    TEST_ASSERT_EQUAL_INT(1, files_on_row((unsigned)i));
    TEST_ASSERT_EQUAL_INT(FILES_ST_PROMPT, files_state());
    files_prompt_open_text();
    TEST_ASSERT_EQUAL_STRING(APP_ID_TEXT, shell_top_id());
    TEST_ASSERT_EQUAL_STRING("/user/data.bin", files_open_path());
    TEST_ASSERT_EQUAL_INT(ERR_OK, shell_pop());
    TEST_ASSERT_EQUAL_STRING(APP_ID_FILES, shell_top_id());
}

static void test_empty_and_unmounted(void)
{
    int i;

    TEST_ASSERT_EQUAL_INT(ERR_OK, vfs_mount());
    shell_init();
    files_reset();
    files_load();
    TEST_ASSERT_EQUAL_INT(ERR_OK, vfs_mkdir("/user/empty"));
    files_reload();
    i = find_row("empty");
    TEST_ASSERT_GREATER_OR_EQUAL_INT(0, i);
    TEST_ASSERT_EQUAL_INT(1, files_on_row((unsigned)i));
    TEST_ASSERT_EQUAL_STRING("/user/empty", files_cwd());
    TEST_ASSERT_EQUAL_INT(FILES_ST_EMPTY, files_state());
    TEST_ASSERT_EQUAL_UINT(0u, files_count());
    TEST_ASSERT_EQUAL_INT(1, files_back());
    TEST_ASSERT_EQUAL_INT(FILES_ST_OK, files_state());

    vfs_ram_set_mounted(0);
    files_reload();
    TEST_ASSERT_EQUAL_INT(FILES_ST_UNMOUNTED, files_state());
    TEST_ASSERT_EQUAL_UINT(0u, files_count());
    vfs_ram_set_mounted(1);
    files_reload();
    TEST_ASSERT_EQUAL_INT(FILES_ST_OK, files_state());
}

void test_files_run(void)
{
    UnitySetTestFile(__FILE__);
    RUN_TEST(test_path_join_parent);
    RUN_TEST(test_format_and_tags);
    RUN_TEST(test_list_sorted);
    RUN_TEST(test_enter_and_back);
    RUN_TEST(test_open_with);
    RUN_TEST(test_unknown_prompt);
    RUN_TEST(test_empty_and_unmounted);
}
