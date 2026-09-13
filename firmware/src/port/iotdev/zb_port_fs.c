#include "zb_port_fs.h"

#include "svc/vfs.h"

#include <string.h>

#define ZB_PORT_FILE_MAX 4
#define ZB_PORT_DIR_MAX 2

struct zb_port_file {
    uint8_t used;
    vfs_file_t fd;
};

struct zb_port_dir {
    uint8_t used;
    vfs_dir_t dir;
    struct zb_port_dirent ent;
};

static struct zb_port_file g_files[ZB_PORT_FILE_MAX];
static struct zb_port_dir g_dirs[ZB_PORT_DIR_MAX];

static err_t ensure_parent(const char *path)
{
    char parent[VFS_PATH_MAX];
    err_t e;

    e = vfs_path_parent(path, parent, sizeof(parent));
    if (e != ERR_OK) {
        return e;
    }
    e = vfs_mkdir(parent);
    if (e == ERR_OK || e == ERR_DENIED) {
        return ERR_OK;
    }
    return e;
}

zb_port_file_t *zb_port_fopen(const char *path, const char *mode)
{
    uint32_t flags = 0u;
    uint32_t i;
    vfs_file_t fd = -1;

    if (path == NULL || mode == NULL || vfs_mounted() == 0) {
        return NULL;
    }
    if (strchr(mode, 'w') != NULL) {
        flags = VFS_O_WR | VFS_O_CREAT | VFS_O_TRUNC;
        (void)ensure_parent(path);
    } else {
        flags = VFS_O_RD;
    }
    if (vfs_open(path, flags, &fd) != ERR_OK) {
        return NULL;
    }
    for (i = 0u; i < ZB_PORT_FILE_MAX; i++) {
        if (g_files[i].used == 0u) {
            g_files[i].used = 1u;
            g_files[i].fd = fd;
            return &g_files[i];
        }
    }
    (void)vfs_close(fd);
    return NULL;
}

size_t zb_port_fread(void *ptr, size_t size, size_t nmemb, zb_port_file_t *f)
{
    size_t want;
    size_t got = 0u;

    if (f == NULL || f->used == 0u || ptr == NULL) {
        return 0u;
    }
    want = size * nmemb;
    if (want == 0u) {
        return 0u;
    }
    if (vfs_read(f->fd, ptr, want, &got) != ERR_OK) {
        return 0u;
    }
    if (size == 0u) {
        return 0u;
    }
    return got / size;
}

size_t zb_port_fwrite(const void *ptr, size_t size, size_t nmemb, zb_port_file_t *f)
{
    size_t want;
    size_t put = 0u;

    if (f == NULL || f->used == 0u || ptr == NULL) {
        return 0u;
    }
    want = size * nmemb;
    if (want == 0u) {
        return 0u;
    }
    if (vfs_write(f->fd, ptr, want, &put) != ERR_OK) {
        return 0u;
    }
    if (size == 0u) {
        return 0u;
    }
    return put / size;
}

int zb_port_fclose(zb_port_file_t *f)
{
    if (f == NULL || f->used == 0u) {
        return -1;
    }
    (void)vfs_close(f->fd);
    f->used = 0u;
    f->fd = -1;
    return 0;
}

zb_port_dir_t *zb_port_opendir(const char *path)
{
    uint32_t i;
    vfs_dir_t dir = -1;

    if (path == NULL || vfs_mounted() == 0) {
        return NULL;
    }
    if (vfs_opendir(path, &dir) != ERR_OK) {
        return NULL;
    }
    for (i = 0u; i < ZB_PORT_DIR_MAX; i++) {
        if (g_dirs[i].used == 0u) {
            g_dirs[i].used = 1u;
            g_dirs[i].dir = dir;
            return &g_dirs[i];
        }
    }
    (void)vfs_closedir(dir);
    return NULL;
}

struct zb_port_dirent *zb_port_readdir(zb_port_dir_t *d)
{
    vfs_dirent_t ent;

    if (d == NULL || d->used == 0u) {
        return NULL;
    }
    memset(&ent, 0, sizeof(ent));
    if (vfs_readdir(d->dir, &ent) != ERR_OK) {
        return NULL;
    }
    memset(d->ent.d_name, 0, sizeof(d->ent.d_name));
    strncpy(d->ent.d_name, ent.name, sizeof(d->ent.d_name) - 1u);
    return &d->ent;
}

int zb_port_closedir(zb_port_dir_t *d)
{
    if (d == NULL || d->used == 0u) {
        return -1;
    }
    (void)vfs_closedir(d->dir);
    d->used = 0u;
    return 0;
}

int zb_port_remove(const char *path)
{
    if (path == NULL) {
        return -1;
    }
    return (vfs_unlink(path) == ERR_OK) ? 0 : -1;
}

int zb_port_rename(const char *from, const char *to)
{
    if (from == NULL || to == NULL) {
        return -1;
    }
    (void)vfs_unlink(to);
    return (vfs_rename(from, to) == ERR_OK) ? 0 : -1;
}

int zb_port_mkdir(const char *path)
{
    err_t e;

    if (path == NULL) {
        return -1;
    }
    e = vfs_mkdir(path);
    if (e == ERR_OK || e == ERR_DENIED) {
        return 0;
    }
    return -1;
}

int zb_port_stat(const char *path, struct zb_port_stat *st)
{
    vfs_stat_t vs;

    if (path == NULL) {
        return -1;
    }
    memset(&vs, 0, sizeof(vs));
    if (vfs_stat(path, &vs) != ERR_OK) {
        return -1;
    }
    if (st != NULL) {
        st->st_size = vs.size;
        st->st_mode = vs.is_dir;
    }
    return 0;
}

int zb_port_exists(const char *path)
{
    return (zb_port_stat(path, NULL) == 0) ? 1 : 0;
}

int zb_port_file_size(const char *path)
{
    vfs_stat_t vs;

    memset(&vs, 0, sizeof(vs));
    if (path == NULL || vfs_stat(path, &vs) != ERR_OK) {
        return 0;
    }
    return (int)vs.size;
}

int zb_port_file_read(const char *path, uint8_t *data, uint16_t size, uint32_t offset)
{
    vfs_file_t fd = -1;
    size_t got = 0u;

    if (path == NULL || data == NULL) {
        return 0;
    }
    if (vfs_open(path, VFS_O_RD, &fd) != ERR_OK) {
        return 0;
    }
    if (offset != 0u && vfs_seek(fd, offset) != ERR_OK) {
        (void)vfs_close(fd);
        return 0;
    }
    (void)vfs_read(fd, data, size, &got);
    (void)vfs_close(fd);
    return (int)got;
}

int zb_port_file_write(const char *path, const uint8_t *data, uint16_t size, uint32_t offset)
{
    vfs_file_t fd = -1;
    size_t put = 0u;
    uint32_t flags = VFS_O_WR | VFS_O_CREAT;

    if (path == NULL || data == NULL) {
        return 0;
    }
    if (offset == 0u) {
        flags |= VFS_O_TRUNC;
    }
    (void)ensure_parent(path);
    if (vfs_open(path, flags, &fd) != ERR_OK) {
        return 0;
    }
    if (offset != 0u && vfs_seek(fd, offset) != ERR_OK) {
        (void)vfs_close(fd);
        return 0;
    }
    (void)vfs_write(fd, data, size, &put);
    (void)vfs_close(fd);
    return (int)put;
}

int iotdev_is_file_exists(const char *path)
{
    return zb_port_exists(path) ? 0 : -1;
}

int iotdev_create_dir(const char *path)
{
    return zb_port_mkdir(path);
}

uint32_t zb_port_crc32(uint32_t crc, const void *data, size_t size)
{
    const uint8_t *p = (const uint8_t *)data;
    size_t i;
    uint32_t j;

    crc = ~crc;
    for (i = 0u; i < size; i++) {
        crc ^= p[i];
        for (j = 0u; j < 8u; j++) {
            uint32_t mask = (uint32_t)(-(int32_t)(crc & 1u));
            crc = (crc >> 1) ^ (0xEDB88320u & mask);
        }
    }
    return ~crc;
}
