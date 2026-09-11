#ifndef VFS_H
#define VFS_H

#include "err.h"

#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

#define VFS_PATH_MAX 256
#define VFS_JAIL_PREFIX "/user"

/*
 * Rejects NUL, '\\', empty segments, "//", and any ".." component.
 * On success, out holds a path that starts with "/user".
 */
err_t vfs_normalize(const char *in, char *out, size_t out_sz);
int vfs_in_user_jail(const char *norm);

#ifdef __cplusplus
}
#endif

#endif /* VFS_H */
