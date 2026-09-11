#include "test.h"

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

    CHECK(vfs_path_join("/user", "hello.txt", out, sizeof(out)) == ERR_OK);
    CHECK(strcmp(out, "/user/hello.txt") == 0);
    CHECK(vfs_path_join("/user/", "sub", out, sizeof(out)) == ERR_OK);
    CHECK(strcmp(out, "/user/sub") == 0);
    CHECK(vfs_path_join("/user", "..", out, sizeof(out)) == ERR_DENIED);
    CHECK(vfs_path_join("/user", ".", out, sizeof(out)) == ERR_DENIED);
    CHECK(vfs_path_join("/user", "a/b", out, sizeof(out)) == ERR_DENIED);
    CHECK(vfs_path_join("/user", "", out, sizeof(out)) == ERR_DENIED);
    CHECK(vfs_path_join(NULL, "a", out, sizeof(out)) == ERR_INVAL);
    CHECK(vfs_path_join("/user", "a", tiny, sizeof(tiny)) == ERR_NOSPC);

    CHECK(vfs_path_parent("/user/sub/a.txt", out, sizeof(out)) == ERR_OK);
    CHECK(strcmp(out, "/user/sub") == 0);
    CHECK(vfs_path_parent("/user/sub", out, sizeof(out)) == ERR_OK);
    CHECK(strcmp(out, "/user") == 0);
    CHECK(vfs_path_parent("/user", out, sizeof(out)) == ERR_NOENT);
    CHECK(vfs_path_parent(NULL, out, sizeof(out)) == ERR_INVAL);
    CHECK(vfs_path_parent("/etc", out, sizeof(out)) == ERR_DENIED);
}

static void test_format_and_tags(void)
{
    char sz[16];
    files_row_t r;

    files_format_size(0u, sz, sizeof(sz));
    CHECK(strcmp(sz, "0 B") == 0);
    files_format_size(512u, sz, sizeof(sz));
    CHECK(strcmp(sz, "512 B") == 0);
    files_format_size(2048u, sz, sizeof(sz));
    CHECK(strcmp(sz, "2 KB") == 0);
    files_format_size(2u * 1024u * 1024u, sz, sizeof(sz));
    CHECK(strcmp(sz, "2 MB") == 0);
    files_format_size(1u, NULL, 4u);
    sz[0] = 'x';
    files_format_size(1u, sz, 0u);
    CHECK(sz[0] == 'x');

    memset(&r, 0, sizeof(r));
    r.is_dir = 1u;
    CHECK(strcmp(files_kind_tag(&r), "DIR") == 0);
    r.is_dir = 0u;
    r.kind = MEDIA_KIND_TEXT;
    CHECK(strcmp(files_kind_tag(&r), "TXT") == 0);
    r.kind = MEDIA_KIND_IMAGE;
    CHECK(strcmp(files_kind_tag(&r), "IMG") == 0);
    r.kind = MEDIA_KIND_AUDIO;
    CHECK(strcmp(files_kind_tag(&r), "AUD") == 0);
    r.kind = MEDIA_KIND_NONE;
    CHECK(strcmp(files_kind_tag(&r), "BIN") == 0);
    CHECK(strcmp(files_kind_tag(NULL), "BIN") == 0);
}

static void test_list_sorted(void)
{
    const files_row_t *r;

    CHECK(vfs_mount() == ERR_OK);
    shell_init();
    files_reset();
    files_load();
    CHECK(files_state() == FILES_ST_OK);
    CHECK(strcmp(files_cwd(), "/user") == 0);
    CHECK(files_count() == 6u);
    CHECK(files_on_row(99u) == 0);
    CHECK(files_row(99u) == NULL);

    r = files_row(0);
    CHECK(r != NULL);
    CHECK(r->is_dir == 1u);
    CHECK(strcmp(r->name, "sub") == 0);
    CHECK(strcmp(files_kind_tag(r), "DIR") == 0);

    CHECK(find_row("hello.txt") > find_row("sub"));
    CHECK(find_row("data.bin") > find_row("sub"));
    CHECK(find_row("hello.txt") > find_row("data.bin"));
    CHECK(find_row("photo.png") > find_row("hello.txt"));
    CHECK(find_row("readme.md") > find_row("photo.png"));
    CHECK(find_row("song.wav") > find_row("readme.md"));

    r = files_row((unsigned)find_row("hello.txt"));
    CHECK(r != NULL);
    CHECK(r->kind == MEDIA_KIND_TEXT);
    r = files_row((unsigned)find_row("photo.png"));
    CHECK(r != NULL);
    CHECK(r->kind == MEDIA_KIND_IMAGE);
    r = files_row((unsigned)find_row("song.wav"));
    CHECK(r != NULL);
    CHECK(r->kind == MEDIA_KIND_AUDIO);
    r = files_row((unsigned)find_row("data.bin"));
    CHECK(r != NULL);
    CHECK(r->kind == MEDIA_KIND_NONE);
}

static void test_enter_and_back(void)
{
    int sub;
    const files_row_t *r;

    CHECK(vfs_mount() == ERR_OK);
    shell_init();
    files_reset();
    files_load();
    files_set_scroll(40);
    CHECK(files_scroll() == 40);
    CHECK(files_back() == 0);

    sub = find_row("sub");
    CHECK(sub >= 0);
    CHECK(files_on_row((unsigned)sub) == 1);
    CHECK(strcmp(files_cwd(), "/user/sub") == 0);
    CHECK(files_count() == 1u);
    CHECK(files_scroll() == 0);
    r = files_row(0);
    CHECK(r != NULL);
    CHECK(strcmp(r->name, "a.txt") == 0);
    CHECK(r->is_dir == 0u);

    CHECK(files_back() == 1);
    CHECK(strcmp(files_cwd(), "/user") == 0);
    CHECK(files_scroll() == 40);
    CHECK(files_count() == 6u);
    CHECK(find_row("sub") >= 0);
}

static void test_open_with(void)
{
    int i;

    CHECK(vfs_mount() == ERR_OK);
    shell_init();
    files_reset();
    files_load();
    CHECK(shell_push(APP_ID_FILES, NULL) == ERR_OK);

    i = find_row("hello.txt");
    CHECK(i >= 0);
    CHECK(files_on_row((unsigned)i) == 1);
    CHECK(strcmp(shell_top_id(), APP_ID_TEXT) == 0);
    CHECK(strcmp(files_open_path(), "/user/hello.txt") == 0);
    CHECK(app_view_path() != NULL);
    CHECK(strcmp(app_view_path(), "/user/hello.txt") == 0);
    CHECK(shell_pop() == ERR_OK);
    CHECK(strcmp(shell_top_id(), APP_ID_FILES) == 0);
    CHECK(strcmp(files_cwd(), "/user") == 0);

    i = find_row("readme.md");
    CHECK(i >= 0);
    CHECK(files_on_row((unsigned)i) == 1);
    CHECK(strcmp(shell_top_id(), APP_ID_TEXT) == 0);
    CHECK(shell_pop() == ERR_OK);

    i = find_row("photo.png");
    CHECK(i >= 0);
    CHECK(files_on_row((unsigned)i) == 1);
    CHECK(strcmp(shell_top_id(), APP_ID_IMAGE) == 0);
    CHECK(strcmp(files_open_path(), "/user/photo.png") == 0);
    CHECK(shell_pop() == ERR_OK);

    i = find_row("song.wav");
    CHECK(i >= 0);
    CHECK(files_on_row((unsigned)i) == 1);
    CHECK(strcmp(shell_top_id(), APP_ID_PLAYER) == 0);
    CHECK(strcmp(files_open_path(), "/user/song.wav") == 0);
    CHECK(shell_pop() == ERR_OK);
}

static void test_unknown_prompt(void)
{
    int i;

    CHECK(vfs_mount() == ERR_OK);
    shell_init();
    files_reset();
    files_load();
    CHECK(shell_push(APP_ID_FILES, NULL) == ERR_OK);

    i = find_row("data.bin");
    CHECK(i >= 0);
    CHECK(files_on_row((unsigned)i) == 1);
    CHECK(files_state() == FILES_ST_PROMPT);
    CHECK(strcmp(files_prompt_name(), "data.bin") == 0);
    CHECK(strcmp(files_open_path(), "/user/data.bin") == 0);
    CHECK(strcmp(shell_top_id(), APP_ID_FILES) == 0);

    files_prompt_cancel();
    CHECK(files_state() == FILES_ST_OK);
    CHECK(files_prompt_name()[0] == '\0');

    CHECK(files_on_row((unsigned)i) == 1);
    CHECK(files_state() == FILES_ST_PROMPT);
    files_prompt_open_text();
    CHECK(strcmp(shell_top_id(), APP_ID_TEXT) == 0);
    CHECK(strcmp(files_open_path(), "/user/data.bin") == 0);
    CHECK(shell_pop() == ERR_OK);
    CHECK(strcmp(shell_top_id(), APP_ID_FILES) == 0);
}

static void test_empty_and_unmounted(void)
{
    int i;

    CHECK(vfs_mount() == ERR_OK);
    shell_init();
    files_reset();
    files_load();
    CHECK(vfs_mkdir("/user/empty") == ERR_OK);
    files_reload();
    i = find_row("empty");
    CHECK(i >= 0);
    CHECK(files_on_row((unsigned)i) == 1);
    CHECK(strcmp(files_cwd(), "/user/empty") == 0);
    CHECK(files_state() == FILES_ST_EMPTY);
    CHECK(files_count() == 0u);
    CHECK(files_back() == 1);
    CHECK(files_state() == FILES_ST_OK);

    vfs_ram_set_mounted(0);
    files_reload();
    CHECK(files_state() == FILES_ST_UNMOUNTED);
    CHECK(files_count() == 0u);
    vfs_ram_set_mounted(1);
    files_reload();
    CHECK(files_state() == FILES_ST_OK);
}

void test_files_run(void)
{
    test_path_join_parent();
    test_format_and_tags();
    test_list_sorted();
    test_enter_and_back();
    test_open_with();
    test_unknown_prompt();
    test_empty_and_unmounted();
}
