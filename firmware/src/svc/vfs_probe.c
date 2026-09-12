#include "svc/vfs.h"

#include <string.h>

#define VFS_PROBE_PATH "/user/vfs_probe.txt"
#define VFS_PROBE_MSG "h745 vfs\n"

err_t vfs_selftest(void)
{
    vfs_file_t fd = -1;
    char buf[16];
    size_t n = 0;
    err_t e;

    e = vfs_open(VFS_PROBE_PATH, VFS_O_WR | VFS_O_CREAT | VFS_O_TRUNC, &fd);
    if (e != ERR_OK) {
        return e;
    }
    e = vfs_write(fd, VFS_PROBE_MSG, sizeof(VFS_PROBE_MSG) - 1u, &n);
    if (e != ERR_OK) {
        (void)vfs_close(fd);
        return e;
    }
    if (n != sizeof(VFS_PROBE_MSG) - 1u) {
        (void)vfs_close(fd);
        return ERR_IO;
    }
    e = vfs_close(fd);
    if (e != ERR_OK) {
        return e;
    }

    fd = -1;
    memset(buf, 0, sizeof(buf));
    e = vfs_open(VFS_PROBE_PATH, VFS_O_RD, &fd);
    if (e != ERR_OK) {
        return e;
    }
    e = vfs_read(fd, buf, sizeof(buf), &n);
    (void)vfs_close(fd);
    if (e != ERR_OK) {
        return e;
    }
    if (n != sizeof(VFS_PROBE_MSG) - 1u) {
        return ERR_CORRUPT;
    }
    if (memcmp(buf, VFS_PROBE_MSG, n) != 0) {
        return ERR_CORRUPT;
    }
    return ERR_OK;
}
