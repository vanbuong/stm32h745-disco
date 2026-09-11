#ifndef VFS_HOST_H
#define VFS_HOST_H

#include "err.h"

#ifdef __cplusplus
extern "C" {
#endif

err_t vfs_host_set_root(const char *path);
const char *vfs_host_root(void);

#ifdef __cplusplus
}
#endif

#endif /* VFS_HOST_H */
