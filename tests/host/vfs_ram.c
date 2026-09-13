#include "svc/vfs.h"

#include <string.h>

/*
 * Host-only RAM tree. Firmware links FatFs vfs.c instead.
 * Incremental readdir: one node per call, no full-directory snapshot.
 */

#define RAM_MAX 16
#define RAM_CAP 512

typedef struct {
    uint8_t used;
    uint8_t is_dir;
    int parent;
    char name[VFS_NAME_MAX];
    uint16_t len;
    uint8_t data[RAM_CAP];
} ram_node_t;

typedef struct {
    uint8_t used;
    int node;
    uint16_t pos;
} ram_file_t;

typedef struct {
    uint8_t used;
    int parent;
    int next;
} ram_dir_t;

static ram_node_t g_nodes[RAM_MAX];
static ram_file_t g_files[4];
static ram_dir_t g_dirs[2];
static uint8_t g_mounted;
static uint8_t g_inited;

static int add_node(int parent, const char *name, uint8_t is_dir, const char *payload)
{
    int i;
    for (i = 0; i < RAM_MAX; i++) {
        if (g_nodes[i].used == 0u) {
            g_nodes[i].used = 1u;
            g_nodes[i].is_dir = is_dir;
            g_nodes[i].parent = parent;
            strncpy(g_nodes[i].name, name, VFS_NAME_MAX - 1u);
            g_nodes[i].len = 0u;
            if (payload != NULL) {
                size_t n = strlen(payload);
                if (n > RAM_CAP) {
                    n = RAM_CAP;
                }
                memcpy(g_nodes[i].data, payload, n);
                g_nodes[i].len = (uint16_t)n;
            }
            return i;
        }
    }
    return -1;
}

static err_t walk(const char *path, int *out)
{
    char norm[VFS_PATH_MAX];
    char rel[VFS_PATH_MAX];
    char seg[VFS_NAME_MAX];
    size_t i;
    size_t s;
    int cur;
    err_t e;

    e = vfs_normalize(path, norm, sizeof(norm));
    if (e != ERR_OK) {
        return e;
    }
    e = vfs_jail_rel(norm, rel, sizeof(rel));
    if (e != ERR_OK) {
        return e;
    }
    cur = 0;
    if (rel[0] == '/' && rel[1] == '\0') {
        *out = cur;
        return ERR_OK;
    }
    i = 1;
    while (rel[i] != '\0') {
        s = 0u;
        while (rel[i] != '\0' && rel[i] != '/') {
            if (s + 1u >= VFS_NAME_MAX) {
                return ERR_NOSPC;
            }
            seg[s++] = rel[i++];
        }
        seg[s] = '\0';
        if (rel[i] == '/') {
            i++;
        }
        {
            int k;
            int found = -1;
            for (k = 0; k < RAM_MAX; k++) {
                if (g_nodes[k].used != 0u && g_nodes[k].parent == cur &&
                    strcmp(g_nodes[k].name, seg) == 0) {
                    found = k;
                    break;
                }
            }
            if (found < 0) {
                return ERR_NOENT;
            }
            cur = found;
        }
    }
    *out = cur;
    return ERR_OK;
}

err_t vfs_mount(void)
{
    memset(g_nodes, 0, sizeof(g_nodes));
    memset(g_files, 0, sizeof(g_files));
    memset(g_dirs, 0, sizeof(g_dirs));
    g_nodes[0].used = 1u;
    g_nodes[0].is_dir = 1u;
    g_nodes[0].parent = -1;
    strncpy(g_nodes[0].name, "/", VFS_NAME_MAX - 1u);
    (void)add_node(0, "hello.txt", 0u, "hello\n");
    (void)add_node(0, "readme.md", 0u, "# hi\n");
    (void)add_node(0, "data.bin", 0u, "BIN\n");
    (void)add_node(0, "photo.png", 0u, "PNG");
    (void)add_node(0, "song.wav", 0u, "RIFF");
    {
        int sub = add_node(0, "sub", 1u, NULL);
        if (sub >= 0) {
            (void)add_node(sub, "a.txt", 0u, "a");
        }
    }
    g_inited = 1u;
    g_mounted = 1u;
    return ERR_OK;
}

err_t vfs_unmount(void)
{
    memset(g_files, 0, sizeof(g_files));
    memset(g_dirs, 0, sizeof(g_dirs));
    g_mounted = 0u;
    return ERR_OK;
}

err_t vfs_remount(void)
{
    if (g_inited == 0u) {
        return vfs_mount();
    }
    memset(g_files, 0, sizeof(g_files));
    memset(g_dirs, 0, sizeof(g_dirs));
    g_mounted = 1u;
    return ERR_OK;
}

err_t vfs_format(void)
{
    return g_mounted ? ERR_OK : ERR_IO;
}

uint8_t vfs_formatted_on_mount(void)
{
    return 0u;
}

int vfs_mounted(void)
{
    return g_mounted ? 1 : 0;
}

static err_t create_file(const char *path, int *out)
{
    char norm[VFS_PATH_MAX];
    char rel[VFS_PATH_MAX];
    char parent_abs[VFS_PATH_MAX];
    const char *slash;
    char leaf[VFS_NAME_MAX];
    int parent;
    int i;
    err_t e;
    size_t leaf_n;

    e = vfs_normalize(path, norm, sizeof(norm));
    if (e != ERR_OK) {
        return e;
    }
    e = vfs_jail_rel(norm, rel, sizeof(rel));
    if (e != ERR_OK) {
        return e;
    }
    slash = strrchr(rel, '/');
    if (slash == NULL || slash[1] == '\0') {
        return ERR_INVAL;
    }
    leaf_n = strlen(slash + 1);
    if (leaf_n >= VFS_NAME_MAX) {
        return ERR_NOSPC;
    }
    memcpy(leaf, slash + 1, leaf_n + 1u);
    if (slash == rel) {
        parent = 0;
    } else {
        size_t pl = (size_t)(slash - rel);
        if (pl + 5u + 1u > VFS_PATH_MAX) {
            return ERR_NOSPC;
        }
        memcpy(parent_abs, "/user", 5u);
        memcpy(parent_abs + 5u, rel, pl);
        parent_abs[5u + pl] = '\0';
        e = walk(parent_abs, &parent);
        if (e != ERR_OK) {
            return e;
        }
        if (g_nodes[parent].is_dir == 0u) {
            return ERR_INVAL;
        }
    }
    i = add_node(parent, leaf, 0u, NULL);
    if (i < 0) {
        return ERR_NOSPC;
    }
    *out = i;
    return ERR_OK;
}

err_t vfs_open(const char *path, uint32_t flags, vfs_file_t *fd)
{
    int node;
    int i;
    err_t e;

    if (fd == NULL) {
        return ERR_INVAL;
    }
    *fd = -1;
    if (!g_mounted) {
        return ERR_IO;
    }
    e = walk(path, &node);
    if (e == ERR_NOENT && (flags & VFS_O_CREAT) != 0u) {
        e = create_file(path, &node);
    }
    if (e != ERR_OK) {
        return e;
    }
    if (g_nodes[node].is_dir != 0u) {
        return ERR_INVAL;
    }
    if ((flags & VFS_O_TRUNC) != 0u) {
        g_nodes[node].len = 0u;
    }
    for (i = 0; i < 4; i++) {
        if (g_files[i].used == 0u) {
            g_files[i].used = 1u;
            g_files[i].node = node;
            g_files[i].pos = 0u;
            *fd = i;
            return ERR_OK;
        }
    }
    (void)flags;
    return ERR_BUSY;
}

err_t vfs_read(vfs_file_t fd, void *buf, size_t n, size_t *got)
{
    ram_file_t *f;
    uint16_t remain;
    size_t take;

    if (got != NULL) {
        *got = 0u;
    }
    if (fd < 0 || fd >= 4 || g_files[fd].used == 0u || buf == NULL) {
        return ERR_INVAL;
    }
    f = &g_files[fd];
    if (f->pos >= g_nodes[f->node].len) {
        return ERR_OK;
    }
    remain = (uint16_t)(g_nodes[f->node].len - f->pos);
    take = n;
    if (take > remain) {
        take = remain;
    }
    memcpy(buf, g_nodes[f->node].data + f->pos, take);
    f->pos = (uint16_t)(f->pos + take);
    if (got != NULL) {
        *got = take;
    }
    return ERR_OK;
}

err_t vfs_write(vfs_file_t fd, const void *buf, size_t n, size_t *put)
{
    ram_file_t *f;
    size_t take;

    if (put != NULL) {
        *put = 0u;
    }
    if (fd < 0 || fd >= 4 || g_files[fd].used == 0u || buf == NULL) {
        return ERR_INVAL;
    }
    f = &g_files[fd];
    if ((uint32_t)f->pos + n > RAM_CAP) {
        take = (size_t)(RAM_CAP - f->pos);
    } else {
        take = n;
    }
    memcpy(g_nodes[f->node].data + f->pos, buf, take);
    f->pos = (uint16_t)(f->pos + take);
    if (f->pos > g_nodes[f->node].len) {
        g_nodes[f->node].len = f->pos;
    }
    if (put != NULL) {
        *put = take;
    }
    return (take == n) ? ERR_OK : ERR_NOSPC;
}

err_t vfs_seek(vfs_file_t fd, uint32_t off)
{
    if (fd < 0 || fd >= 4 || g_files[fd].used == 0u) {
        return ERR_INVAL;
    }
    if (off > g_nodes[g_files[fd].node].len) {
        return ERR_INVAL;
    }
    g_files[fd].pos = (uint16_t)off;
    return ERR_OK;
}

err_t vfs_close(vfs_file_t fd)
{
    if (fd < 0 || fd >= 4 || g_files[fd].used == 0u) {
        return ERR_INVAL;
    }
    g_files[fd].used = 0u;
    return ERR_OK;
}

err_t vfs_stat(const char *path, vfs_stat_t *st)
{
    int node;
    err_t e;

    if (st == NULL) {
        return ERR_INVAL;
    }
    memset(st, 0, sizeof(*st));
    e = walk(path, &node);
    if (e != ERR_OK) {
        return e;
    }
    st->is_dir = g_nodes[node].is_dir;
    st->size = g_nodes[node].len;
    return ERR_OK;
}

err_t vfs_opendir(const char *path, vfs_dir_t *dir)
{
    int node;
    int i;
    err_t e;

    if (dir == NULL) {
        return ERR_INVAL;
    }
    *dir = -1;
    if (!g_mounted) {
        return ERR_IO;
    }
    e = walk(path, &node);
    if (e != ERR_OK) {
        return e;
    }
    if (g_nodes[node].is_dir == 0u) {
        return ERR_INVAL;
    }
    for (i = 0; i < 2; i++) {
        if (g_dirs[i].used == 0u) {
            g_dirs[i].used = 1u;
            g_dirs[i].parent = node;
            g_dirs[i].next = 0;
            *dir = i;
            return ERR_OK;
        }
    }
    return ERR_BUSY;
}

err_t vfs_readdir(vfs_dir_t dir, vfs_dirent_t *ent)
{
    ram_dir_t *d;
    int k;

    if (dir < 0 || dir >= 2 || g_dirs[dir].used == 0u || ent == NULL) {
        return ERR_INVAL;
    }
    memset(ent, 0, sizeof(*ent));
    d = &g_dirs[dir];
    for (k = d->next; k < RAM_MAX; k++) {
        if (g_nodes[k].used != 0u && g_nodes[k].parent == d->parent) {
            strncpy(ent->name, g_nodes[k].name, VFS_NAME_MAX - 1u);
            ent->is_dir = g_nodes[k].is_dir;
            ent->size = g_nodes[k].len;
            d->next = k + 1;
            return ERR_OK;
        }
    }
    d->next = RAM_MAX;
    return ERR_NOENT;
}

err_t vfs_closedir(vfs_dir_t dir)
{
    if (dir < 0 || dir >= 2 || g_dirs[dir].used == 0u) {
        return ERR_INVAL;
    }
    g_dirs[dir].used = 0u;
    return ERR_OK;
}

err_t vfs_mkdir(const char *path)
{
    char norm[VFS_PATH_MAX];
    char rel[VFS_PATH_MAX];
    char parent_abs[VFS_PATH_MAX];
    const char *slash;
    char leaf[VFS_NAME_MAX];
    int parent;
    int dummy;
    err_t e;
    size_t leaf_n;

    if (!g_mounted) {
        return ERR_IO;
    }
    e = vfs_normalize(path, norm, sizeof(norm));
    if (e != ERR_OK) {
        return e;
    }
    if (walk(path, &dummy) == ERR_OK) {
        return ERR_DENIED;
    }
    e = vfs_jail_rel(norm, rel, sizeof(rel));
    if (e != ERR_OK) {
        return e;
    }
    slash = strrchr(rel, '/');
    if (slash == NULL || slash[1] == '\0') {
        return ERR_INVAL;
    }
    leaf_n = strlen(slash + 1);
    if (leaf_n >= VFS_NAME_MAX) {
        return ERR_NOSPC;
    }
    memcpy(leaf, slash + 1, leaf_n + 1u);
    if (slash == rel) {
        parent = 0;
    } else {
        size_t pl = (size_t)(slash - rel);
        if (pl + 5u + 1u > VFS_PATH_MAX) {
            return ERR_NOSPC;
        }
        memcpy(parent_abs, "/user", 5u);
        memcpy(parent_abs + 5u, rel, pl);
        parent_abs[5u + pl] = '\0';
        e = walk(parent_abs, &parent);
        if (e != ERR_OK) {
            return e;
        }
        if (g_nodes[parent].is_dir == 0u) {
            return ERR_INVAL;
        }
    }
    if (add_node(parent, leaf, 1u, NULL) < 0) {
        return ERR_NOSPC;
    }
    return ERR_OK;
}

err_t vfs_unlink(const char *path)
{
    int node;
    int i;
    err_t e;

    if (!g_mounted) {
        return ERR_IO;
    }
    e = walk(path, &node);
    if (e != ERR_OK) {
        return e;
    }
    if (node == 0) {
        return ERR_DENIED;
    }
    if (g_nodes[node].is_dir != 0u) {
        for (i = 0; i < RAM_MAX; i++) {
            if (g_nodes[i].used != 0u && g_nodes[i].parent == node) {
                return ERR_DENIED;
            }
        }
    }
    for (i = 0; i < 4; i++) {
        if (g_files[i].used != 0u && g_files[i].node == node) {
            return ERR_BUSY;
        }
    }
    memset(&g_nodes[node], 0, sizeof(g_nodes[node]));
    return ERR_OK;
}

err_t vfs_rename(const char *from, const char *to)
{
    int src;
    int dest;
    int parent;
    char norm[VFS_PATH_MAX];
    char rel[VFS_PATH_MAX];
    char parent_abs[VFS_PATH_MAX];
    const char *slash;
    char leaf[VFS_NAME_MAX];
    size_t leaf_n;
    err_t e;

    if (!g_mounted) {
        return ERR_IO;
    }
    e = walk(from, &src);
    if (e != ERR_OK) {
        return e;
    }
    if (src == 0) {
        return ERR_DENIED;
    }
    if (walk(to, &dest) == ERR_OK) {
        e = vfs_unlink(to);
        if (e != ERR_OK) {
            return e;
        }
    }
    e = vfs_normalize(to, norm, sizeof(norm));
    if (e != ERR_OK) {
        return e;
    }
    e = vfs_jail_rel(norm, rel, sizeof(rel));
    if (e != ERR_OK) {
        return e;
    }
    slash = strrchr(rel, '/');
    if (slash == NULL || slash[1] == '\0') {
        return ERR_INVAL;
    }
    leaf_n = strlen(slash + 1);
    if (leaf_n >= VFS_NAME_MAX) {
        return ERR_NOSPC;
    }
    memcpy(leaf, slash + 1, leaf_n + 1u);
    if (slash == rel) {
        parent = 0;
    } else {
        size_t pl = (size_t)(slash - rel);
        if (pl + 5u + 1u > VFS_PATH_MAX) {
            return ERR_NOSPC;
        }
        memcpy(parent_abs, "/user", 5u);
        memcpy(parent_abs + 5u, rel, pl);
        parent_abs[5u + pl] = '\0';
        e = walk(parent_abs, &parent);
        if (e != ERR_OK) {
            return e;
        }
        if (g_nodes[parent].is_dir == 0u) {
            return ERR_INVAL;
        }
    }
    g_nodes[src].parent = parent;
    memset(g_nodes[src].name, 0, sizeof(g_nodes[src].name));
    memcpy(g_nodes[src].name, leaf, leaf_n + 1u);
    return ERR_OK;
}

void vfs_ram_set_mounted(int on)
{
    g_mounted = (on != 0) ? 1u : 0u;
}

err_t vfs_ram_add_file(const char *name, const void *data, uint16_t n)
{
    int i;

    if (name == NULL || data == NULL) {
        return ERR_INVAL;
    }
    i = add_node(0, name, 0u, NULL);
    if (i < 0) {
        return ERR_NOSPC;
    }
    if (n > RAM_CAP) {
        n = RAM_CAP;
    }
    memcpy(g_nodes[i].data, data, n);
    g_nodes[i].len = n;
    return ERR_OK;
}
