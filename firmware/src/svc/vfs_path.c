#include "svc/vfs.h"

#include <string.h>

static int is_user_root(const char *p)
{
    size_t n = strlen(VFS_JAIL_PREFIX);
    if (strncmp(p, VFS_JAIL_PREFIX, n) != 0) {
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

    out[0] = '\0';
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
