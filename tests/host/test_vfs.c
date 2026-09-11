#include "test.h"

#include "svc/vfs.h"

#include <string.h>

static void expect_denied(const char *p)
{
    char out[VFS_PATH_MAX];
    CHECK(vfs_normalize(p, out, sizeof(out)) == ERR_DENIED);
}

static void test_jail(void)
{
    char out[VFS_PATH_MAX];
    char rel[VFS_PATH_MAX];
    char tiny[4];
    expect_denied("/user/../user/x");
    expect_denied("/user/foo/../../etc");
    expect_denied("//user");
    expect_denied("/etc/passwd");
    expect_denied("user/a.txt");
    expect_denied("/user/a\\b");
    expect_denied("/user/\x01x");
    CHECK(vfs_normalize(NULL, out, sizeof(out)) == ERR_INVAL);
    CHECK(vfs_normalize("/user", tiny, sizeof(tiny)) == ERR_NOSPC);
    CHECK(vfs_normalize("/user/a/b.txt", out, sizeof(out)) == ERR_OK);
    CHECK(strcmp(out, "/user/a/b.txt") == 0);
    CHECK(vfs_in_user_jail(out) == 1);
    CHECK(vfs_normalize("/user", out, sizeof(out)) == ERR_OK);
    CHECK(vfs_in_user_jail("/user") == 1);
    CHECK(vfs_in_user_jail(NULL) == 0);

    CHECK(vfs_jail_rel("/user", rel, sizeof(rel)) == ERR_OK);
    CHECK(strcmp(rel, "/") == 0);
    CHECK(vfs_jail_rel("/user/a/b", rel, sizeof(rel)) == ERR_OK);
    CHECK(strcmp(rel, "/a/b") == 0);
    CHECK(vfs_jail_rel("/etc", rel, sizeof(rel)) == ERR_DENIED);
    CHECK(vfs_jail_rel(NULL, rel, sizeof(rel)) == ERR_INVAL);
}

static void test_ram_ops(void)
{
    vfs_file_t fd = -1;
    vfs_dir_t dir = -1;
    vfs_dirent_t ent;
    vfs_stat_t st;
    char buf[16];
    size_t n = 0;
    unsigned seen = 0;
    int got_hello = 0;
    int got_sub = 0;
    int got_md = 0;
    err_t e;

    CHECK(vfs_mount() == ERR_OK);
    CHECK(vfs_mounted() == 1);

    CHECK(vfs_open("/user/../secret", VFS_O_RD, &fd) == ERR_DENIED);
    CHECK(vfs_open("/qspi/x", VFS_O_RD, &fd) == ERR_DENIED);
    CHECK(vfs_open("/etc/passwd", VFS_O_RD, &fd) == ERR_DENIED);

    CHECK(vfs_stat("/user/hello.txt", &st) == ERR_OK);
    CHECK(st.is_dir == 0u);
    CHECK(st.size == 6u);
    CHECK(vfs_stat("/user/sub", &st) == ERR_OK);
    CHECK(st.is_dir == 1u);

    CHECK(vfs_open("/user/hello.txt", VFS_O_RD, &fd) == ERR_OK);
    CHECK(vfs_read(fd, buf, sizeof(buf), &n) == ERR_OK);
    CHECK(n == 6u);
    CHECK(memcmp(buf, "hello\n", 6) == 0);
    CHECK(vfs_read(fd, buf, sizeof(buf), &n) == ERR_OK);
    CHECK(n == 0u);
    CHECK(vfs_seek(fd, 0u) == ERR_OK);
    CHECK(vfs_close(fd) == ERR_OK);

    CHECK(vfs_opendir("/user", &dir) == ERR_OK);
    for (;;) {
        e = vfs_readdir(dir, &ent);
        if (e == ERR_NOENT) {
            break;
        }
        CHECK(e == ERR_OK);
        seen++;
        if (strcmp(ent.name, "hello.txt") == 0) {
            got_hello = 1;
            CHECK(ent.is_dir == 0u);
        }
        if (strcmp(ent.name, "readme.md") == 0) {
            got_md = 1;
        }
        if (strcmp(ent.name, "sub") == 0) {
            got_sub = 1;
            CHECK(ent.is_dir == 1u);
        }
        CHECK(seen <= 8u);
    }
    CHECK(seen == 6u);
    CHECK(got_hello == 1);
    CHECK(got_md == 1);
    CHECK(got_sub == 1);
    CHECK(vfs_closedir(dir) == ERR_OK);

    CHECK(vfs_opendir("/user/sub", &dir) == ERR_OK);
    CHECK(vfs_readdir(dir, &ent) == ERR_OK);
    CHECK(strcmp(ent.name, "a.txt") == 0);
    CHECK(vfs_readdir(dir, &ent) == ERR_NOENT);
    CHECK(vfs_closedir(dir) == ERR_OK);

    CHECK(vfs_mkdir("/user/newd") == ERR_OK);
    CHECK(vfs_stat("/user/newd", &st) == ERR_OK);
    CHECK(st.is_dir == 1u);
    CHECK(vfs_mkdir("/user/../x") == ERR_DENIED);
}

void test_vfs_run(void)
{
    test_jail();
    test_ram_ops();
}
