#include "unity.h"

#include "svc/vfs.h"

#include <string.h>

static void expect_denied(const char *p)
{
    char out[VFS_PATH_MAX];
    TEST_ASSERT_TRUE(vfs_normalize(p, out, sizeof(out)) == ERR_DENIED);
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
    TEST_ASSERT_TRUE(vfs_normalize(NULL, out, sizeof(out)) == ERR_INVAL);
    TEST_ASSERT_TRUE(vfs_normalize("/user", tiny, sizeof(tiny)) == ERR_NOSPC);
    TEST_ASSERT_TRUE(vfs_normalize("/user/a/b.txt", out, sizeof(out)) == ERR_OK);
    TEST_ASSERT_TRUE(strcmp(out, "/user/a/b.txt") == 0);
    TEST_ASSERT_TRUE(vfs_in_user_jail(out) == 1);
    TEST_ASSERT_TRUE(vfs_normalize("/user", out, sizeof(out)) == ERR_OK);
    TEST_ASSERT_TRUE(vfs_in_user_jail("/user") == 1);
    TEST_ASSERT_TRUE(vfs_in_user_jail(NULL) == 0);

    TEST_ASSERT_TRUE(vfs_jail_rel("/user", rel, sizeof(rel)) == ERR_OK);
    TEST_ASSERT_TRUE(strcmp(rel, "/") == 0);
    TEST_ASSERT_TRUE(vfs_jail_rel("/user/a/b", rel, sizeof(rel)) == ERR_OK);
    TEST_ASSERT_TRUE(strcmp(rel, "/a/b") == 0);
    TEST_ASSERT_TRUE(vfs_jail_rel("/etc", rel, sizeof(rel)) == ERR_DENIED);
    TEST_ASSERT_TRUE(vfs_jail_rel(NULL, rel, sizeof(rel)) == ERR_INVAL);
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

    TEST_ASSERT_TRUE(vfs_mount() == ERR_OK);
    TEST_ASSERT_TRUE(vfs_mounted() == 1);

    TEST_ASSERT_TRUE(vfs_open("/user/../secret", VFS_O_RD, &fd) == ERR_DENIED);
    TEST_ASSERT_TRUE(vfs_open("/qspi/x", VFS_O_RD, &fd) == ERR_DENIED);
    TEST_ASSERT_TRUE(vfs_open("/etc/passwd", VFS_O_RD, &fd) == ERR_DENIED);

    TEST_ASSERT_TRUE(vfs_stat("/user/hello.txt", &st) == ERR_OK);
    TEST_ASSERT_TRUE(st.is_dir == 0u);
    TEST_ASSERT_TRUE(st.size == 6u);
    TEST_ASSERT_TRUE(vfs_stat("/user/sub", &st) == ERR_OK);
    TEST_ASSERT_TRUE(st.is_dir == 1u);

    TEST_ASSERT_TRUE(vfs_open("/user/hello.txt", VFS_O_RD, &fd) == ERR_OK);
    TEST_ASSERT_TRUE(vfs_read(fd, buf, sizeof(buf), &n) == ERR_OK);
    TEST_ASSERT_TRUE(n == 6u);
    TEST_ASSERT_TRUE(memcmp(buf, "hello\n", 6) == 0);
    TEST_ASSERT_TRUE(vfs_read(fd, buf, sizeof(buf), &n) == ERR_OK);
    TEST_ASSERT_TRUE(n == 0u);
    TEST_ASSERT_TRUE(vfs_seek(fd, 0u) == ERR_OK);
    TEST_ASSERT_TRUE(vfs_close(fd) == ERR_OK);

    TEST_ASSERT_TRUE(vfs_opendir("/user", &dir) == ERR_OK);
    for (;;) {
        e = vfs_readdir(dir, &ent);
        if (e == ERR_NOENT) {
            break;
        }
        TEST_ASSERT_TRUE(e == ERR_OK);
        seen++;
        if (strcmp(ent.name, "hello.txt") == 0) {
            got_hello = 1;
            TEST_ASSERT_TRUE(ent.is_dir == 0u);
        }
        if (strcmp(ent.name, "readme.md") == 0) {
            got_md = 1;
        }
        if (strcmp(ent.name, "sub") == 0) {
            got_sub = 1;
            TEST_ASSERT_TRUE(ent.is_dir == 1u);
        }
        TEST_ASSERT_TRUE(seen <= 8u);
    }
    TEST_ASSERT_TRUE(seen == 6u);
    TEST_ASSERT_TRUE(got_hello == 1);
    TEST_ASSERT_TRUE(got_md == 1);
    TEST_ASSERT_TRUE(got_sub == 1);
    TEST_ASSERT_TRUE(vfs_closedir(dir) == ERR_OK);

    TEST_ASSERT_TRUE(vfs_opendir("/user/sub", &dir) == ERR_OK);
    TEST_ASSERT_TRUE(vfs_readdir(dir, &ent) == ERR_OK);
    TEST_ASSERT_TRUE(strcmp(ent.name, "a.txt") == 0);
    TEST_ASSERT_TRUE(vfs_readdir(dir, &ent) == ERR_NOENT);
    TEST_ASSERT_TRUE(vfs_closedir(dir) == ERR_OK);

    TEST_ASSERT_TRUE(vfs_mkdir("/user/newd") == ERR_OK);
    TEST_ASSERT_TRUE(vfs_stat("/user/newd", &st) == ERR_OK);
    TEST_ASSERT_TRUE(st.is_dir == 1u);
    TEST_ASSERT_TRUE(vfs_mkdir("/user/../x") == ERR_DENIED);
}

void test_vfs_run(void)
{
    UnitySetTestFile(__FILE__);
    RUN_TEST(test_jail);
    RUN_TEST(test_ram_ops);
}
