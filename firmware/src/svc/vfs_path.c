#include "svc/vfs.h"

#include <string.h>

static int is_user_root(const char *p)
{
    size_t n = strlen(VFS_JAIL_PREFIX);
    size_t pl;

    if (p == NULL) {
        return 0;
    }
    pl = strlen(p);
    if (pl < n || strncmp(p, VFS_JAIL_PREFIX, n) != 0) {
        return 0;
    }
    return p[n] == '\0' || p[n] == '/';
}

err_t vfs_normalize(const char *in, char *out, size_t out_sz)
{
    size_t i;
    size_t o = 0;
    int start_seg = 1;

    if (in == NULL || out == NULL || out_sz < 2u) {
        return ERR_INVAL;
    }
    if (in[0] != '/') {
        return ERR_DENIED;
    }

    memset(out, 0, out_sz);
    for (i = 0; in[i] != '\0'; i++) {
        unsigned char c = (unsigned char)in[i];
        if (c == '\\' || c == '\0') {
            return ERR_DENIED;
        }
        if (c < 0x20u) {
            return ERR_DENIED;
        }
        if (o + 1u >= out_sz) {
            return ERR_NOSPC;
        }
        if (in[i] == '/') {
            if (start_seg) {
                /* "//" or leading extra slash after first */
                if (i != 0u) {
                    return ERR_DENIED;
                }
            }
            start_seg = 1;
            out[o++] = '/';
            continue;
        }
        if (start_seg && in[i] == '.' && in[i + 1u] == '.' &&
            (in[i + 2u] == '/' || in[i + 2u] == '\0')) {
            return ERR_DENIED;
        }
        start_seg = 0;
        out[o++] = in[i];
    }
    out[o] = '\0';
    if (!is_user_root(out)) {
        return ERR_DENIED;
    }
    return ERR_OK;
}

int vfs_in_user_jail(const char *norm)
{
    if (norm == NULL) {
        return 0;
    }
    return is_user_root(norm);
}

err_t vfs_jail_rel(const char *norm, char *rel, size_t rel_sz)
{
    size_t n = strlen(VFS_JAIL_PREFIX);
    const char *rest;

    if (norm == NULL || rel == NULL || rel_sz < 2u) {
        return ERR_INVAL;
    }
    if (!is_user_root(norm)) {
        return ERR_DENIED;
    }
    rest = norm + n;
    if (rest[0] == '\0' || (rest[0] == '/' && rest[1] == '\0')) {
        rel[0] = '/';
        rel[1] = '\0';
        return ERR_OK;
    }
    if (rest[0] != '/') {
        return ERR_DENIED;
    }
    if (strlen(rest) + 1u > rel_sz) {
        return ERR_NOSPC;
    }
    memcpy(rel, rest, strlen(rest) + 1u);
    return ERR_OK;
}

err_t vfs_path_join(const char *dir, const char *name, char *out, size_t out_sz)
{
    char tmp[VFS_PATH_MAX];
    size_t o = 0u;
    size_t i;

    if (dir == NULL || name == NULL || out == NULL) {
        return ERR_INVAL;
    }
    if (name[0] == '\0') {
        return ERR_DENIED;
    }
    if (strcmp(name, ".") == 0 || strcmp(name, "..") == 0) {
        return ERR_DENIED;
    }
    for (i = 0u; name[i] != '\0'; i++) {
        if (name[i] == '/' || name[i] == '\\') {
            return ERR_DENIED;
        }
    }
    for (i = 0u; dir[i] != '\0'; i++) {
        if (o + 2u >= sizeof(tmp)) {
            return ERR_NOSPC;
        }
        tmp[o++] = dir[i];
    }
    if (o == 0u || tmp[o - 1u] != '/') {
        if (o + 2u >= sizeof(tmp)) {
            return ERR_NOSPC;
        }
        tmp[o++] = '/';
    }
    for (i = 0u; name[i] != '\0'; i++) {
        if (o + 1u >= sizeof(tmp)) {
            return ERR_NOSPC;
        }
        tmp[o++] = name[i];
    }
    tmp[o] = '\0';
    return vfs_normalize(tmp, out, out_sz);
}

err_t vfs_path_parent(const char *path, char *out, size_t out_sz)
{
    char norm[VFS_PATH_MAX];
    size_t len;
    size_t end;
    err_t e;

    if (path == NULL || out == NULL) {
        return ERR_INVAL;
    }
    e = vfs_normalize(path, norm, sizeof(norm));
    if (e != ERR_OK) {
        return e;
    }
    if (strcmp(norm, VFS_JAIL_PREFIX) == 0) {
        return ERR_NOENT;
    }
    len = strlen(norm);
    while (len > 0u && norm[len - 1u] != '/') {
        len--;
    }
    if (len == 0u) {
        return ERR_NOENT;
    }
    end = len - 1u;
    if (end == 0u) {
        return ERR_NOENT;
    }
    if (end + 1u > out_sz) {
        return ERR_NOSPC;
    }
    memcpy(out, norm, end);
    out[end] = '\0';
    return vfs_in_user_jail(out) ? ERR_OK : ERR_DENIED;
}
