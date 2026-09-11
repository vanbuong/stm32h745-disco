#include "unity.h"

#include "svc/vfs.h"

#include <string.h>

static void expect_denied(const char *p)
{
    char out[VFS_PATH_MAX];
    TEST_ASSERT_EQUAL_INT(ERR_DENIED, vfs_normalize(p, out, sizeof(out)));
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
    TEST_ASSERT_EQUAL_INT(ERR_INVAL, vfs_normalize(NULL, out, sizeof(out)));
    TEST_ASSERT_EQUAL_INT(ERR_NOSPC, vfs_normalize("/user", tiny, sizeof(tiny)));
    TEST_ASSERT_EQUAL_INT(ERR_OK, vfs_normalize("/user/a/b.txt", out, sizeof(out)));
    TEST_ASSERT_EQUAL_STRING("/user/a/b.txt", out);
    TEST_ASSERT_EQUAL_INT(1, vfs_in_user_jail(out));
    TEST_ASSERT_EQUAL_INT(ERR_OK, vfs_normalize("/user", out, sizeof(out)));
    TEST_ASSERT_EQUAL_INT(1, vfs_in_user_jail("/user"));
    TEST_ASSERT_EQUAL_INT(0, vfs_in_user_jail(NULL));

    TEST_ASSERT_EQUAL_INT(ERR_OK, vfs_jail_rel("/user", rel, sizeof(rel)));
    TEST_ASSERT_EQUAL_STRING("/", rel);
    TEST_ASSERT_EQUAL_INT(ERR_OK, vfs_jail_rel("/user/a/b", rel, sizeof(rel)));
    TEST_ASSERT_EQUAL_STRING("/a/b", rel);
    TEST_ASSERT_EQUAL_INT(ERR_DENIED, vfs_jail_rel("/etc", rel, sizeof(rel)));
    TEST_ASSERT_EQUAL_INT(ERR_INVAL, vfs_jail_rel(NULL, rel, sizeof(rel)));
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

    TEST_ASSERT_EQUAL_INT(ERR_OK, vfs_mount());
    TEST_ASSERT_EQUAL_INT(1, vfs_mounted());

    TEST_ASSERT_EQUAL_INT(ERR_DENIED, vfs_open("/user/../secret", VFS_O_RD, &fd));
    TEST_ASSERT_EQUAL_INT(ERR_DENIED, vfs_open("/qspi/x", VFS_O_RD, &fd));
    TEST_ASSERT_EQUAL_INT(ERR_DENIED, vfs_open("/etc/passwd", VFS_O_RD, &fd));

    TEST_ASSERT_EQUAL_INT(ERR_OK, vfs_stat("/user/hello.txt", &st));
    TEST_ASSERT_EQUAL_UINT8(0u, st.is_dir);
    TEST_ASSERT_EQUAL_UINT32(6u, st.size);
    TEST_ASSERT_EQUAL_INT(ERR_OK, vfs_stat("/user/sub", &st));
    TEST_ASSERT_EQUAL_UINT8(1u, st.is_dir);

    TEST_ASSERT_EQUAL_INT(ERR_OK, vfs_open("/user/hello.txt", VFS_O_RD, &fd));
    TEST_ASSERT_EQUAL_INT(ERR_OK, vfs_read(fd, buf, sizeof(buf), &n));
    TEST_ASSERT_EQUAL_UINT(6u, n);
    TEST_ASSERT_EQUAL_MEMORY("hello\n", buf, 6);
    TEST_ASSERT_EQUAL_INT(ERR_OK, vfs_read(fd, buf, sizeof(buf), &n));
    TEST_ASSERT_EQUAL_UINT(0u, n);
    TEST_ASSERT_EQUAL_INT(ERR_OK, vfs_seek(fd, 0u));
    TEST_ASSERT_EQUAL_INT(ERR_OK, vfs_close(fd));

    TEST_ASSERT_EQUAL_INT(ERR_OK, vfs_opendir("/user", &dir));
    for (;;) {
        e = vfs_readdir(dir, &ent);
        if (e == ERR_NOENT) {
            break;
        }
        TEST_ASSERT_EQUAL_INT(ERR_OK, e);
        seen++;
        if (strcmp(ent.name, "hello.txt") == 0) {
            got_hello = 1;
            TEST_ASSERT_EQUAL_UINT8(0u, ent.is_dir);
        }
        if (strcmp(ent.name, "readme.md") == 0) {
            got_md = 1;
        }
        if (strcmp(ent.name, "sub") == 0) {
            got_sub = 1;
            TEST_ASSERT_EQUAL_UINT8(1u, ent.is_dir);
        }
        TEST_ASSERT_LESS_OR_EQUAL_UINT(8u, seen);
    }
    TEST_ASSERT_EQUAL_UINT(6u, seen);
    TEST_ASSERT_EQUAL_INT(1, got_hello);
    TEST_ASSERT_EQUAL_INT(1, got_md);
    TEST_ASSERT_EQUAL_INT(1, got_sub);
    TEST_ASSERT_EQUAL_INT(ERR_OK, vfs_closedir(dir));

    TEST_ASSERT_EQUAL_INT(ERR_OK, vfs_opendir("/user/sub", &dir));
    TEST_ASSERT_EQUAL_INT(ERR_OK, vfs_readdir(dir, &ent));
    TEST_ASSERT_EQUAL_STRING("a.txt", ent.name);
    TEST_ASSERT_EQUAL_INT(ERR_NOENT, vfs_readdir(dir, &ent));
    TEST_ASSERT_EQUAL_INT(ERR_OK, vfs_closedir(dir));

    TEST_ASSERT_EQUAL_INT(ERR_OK, vfs_mkdir("/user/newd"));
    TEST_ASSERT_EQUAL_INT(ERR_OK, vfs_stat("/user/newd", &st));
    TEST_ASSERT_EQUAL_UINT8(1u, st.is_dir);
    TEST_ASSERT_EQUAL_INT(ERR_DENIED, vfs_mkdir("/user/../x"));
}

void test_vfs_run(void)
{
    UnitySetTestFile(__FILE__);
    RUN_TEST(test_jail);
    RUN_TEST(test_ram_ops);
}
