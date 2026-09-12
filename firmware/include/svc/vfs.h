#ifndef VFS_H
#define VFS_H

#include "err.h"

#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

#define VFS_PATH_MAX 256
#define VFS_NAME_MAX 64
#define VFS_JAIL_PREFIX "/user"

#define VFS_O_RD 1u
#define VFS_O_WR 2u
#define VFS_O_RDWR 3u
#define VFS_O_CREAT 4u
#define VFS_O_TRUNC 8u

typedef int vfs_file_t;
typedef int vfs_dir_t;

typedef struct {
    uint8_t is_dir;
    uint32_t size;
} vfs_stat_t;

typedef struct {
    char name[VFS_NAME_MAX];
    uint8_t is_dir;
    uint32_t size;
} vfs_dirent_t;

/*
 * Rejects NUL, '\\', empty segments, "//", and any ".." component.
 * On success, out holds a path that starts with "/user".
 */
err_t vfs_normalize(const char *in, char *out, size_t out_sz);
int vfs_in_user_jail(const char *norm);

/* "/user" -> "/", "/user/a/b" -> "/a/b". in must already be normalized. */
err_t vfs_jail_rel(const char *norm, char *rel, size_t rel_sz);

/* Join jail path + single name (no slashes). Result is normalized. */
err_t vfs_path_join(const char *dir, const char *name, char *out, size_t out_sz);
/* Parent of a normalized jail path. ERR_NOENT at "/user". */
err_t vfs_path_parent(const char *path, char *out, size_t out_sz);

err_t vfs_mount(void);
int vfs_mounted(void);
err_t vfs_format(void);
uint8_t vfs_formatted_on_mount(void);

/* Create /user/vfs_probe.txt, write a known string, reopen, and compare. */
err_t vfs_selftest(void);

err_t vfs_open(const char *path, uint32_t flags, vfs_file_t *fd);
err_t vfs_read(vfs_file_t fd, void *buf, size_t n, size_t *got);
err_t vfs_write(vfs_file_t fd, const void *buf, size_t n, size_t *put);
err_t vfs_seek(vfs_file_t fd, uint32_t off);
err_t vfs_close(vfs_file_t fd);

err_t vfs_stat(const char *path, vfs_stat_t *st);

err_t vfs_opendir(const char *path, vfs_dir_t *dir);
err_t vfs_readdir(vfs_dir_t dir, vfs_dirent_t *ent);
err_t vfs_closedir(vfs_dir_t dir);

err_t vfs_mkdir(const char *path);

#ifdef __cplusplus
}
#endif

#endif /* VFS_H */
