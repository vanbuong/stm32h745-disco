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
}

void test_vfs_run(void)
{
    test_jail();
}
