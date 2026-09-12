#include "svc/vfs.h"

#include "vfs_host.h"

#include <errno.h>
#include <stdio.h>
#include <string.h>

#ifdef _WIN32
#include <direct.h>
#include <io.h>
#include <sys/stat.h>
#include <windows.h>
#define HOST_SEP '\\'
#define mkdir_one(p) _mkdir(p)
#else
#include <dirent.h>
#include <sys/stat.h>
#define HOST_SEP '/'
#define mkdir_one(p) mkdir((p), 0755)
#endif

#define VFS_FILE_MAX 4
#define VFS_DIR_MAX 2

#ifdef _WIN32
typedef struct {
    HANDLE h;
    WIN32_FIND_DATAA fd;
    uint8_t has;
} host_dir_t;
#else
typedef struct {
    DIR *d;
    char path[VFS_PATH_MAX * 2u];
} host_dir_t;
#endif

static char g_root[VFS_PATH_MAX];
static uint8_t g_mounted;
static FILE *g_fil[VFS_FILE_MAX];
static host_dir_t g_dir[VFS_DIR_MAX];
static uint8_t g_fil_used[VFS_FILE_MAX];
static uint8_t g_dir_used[VFS_DIR_MAX];

static int alloc_fil(void)
{
    int i;
    for (i = 0; i < VFS_FILE_MAX; i++) {
        if (g_fil_used[i] == 0u) {
            g_fil_used[i] = 1u;
            return i;
        }
    }
    return -1;
}

static int alloc_dir(void)
{
    int i;
    for (i = 0; i < VFS_DIR_MAX; i++) {
        if (g_dir_used[i] == 0u) {
            g_dir_used[i] = 1u;
            return i;
        }
    }
    return -1;
}

static err_t to_host(const char *path, char *out, size_t out_sz)
{
    char norm[VFS_PATH_MAX];
    char rel[VFS_PATH_MAX];
    size_t o = 0u;
    size_t i;
    err_t e;

    e = vfs_normalize(path, norm, sizeof(norm));
    if (e != ERR_OK) {
        return e;
    }
    e = vfs_jail_rel(norm, rel, sizeof(rel));
    if (e != ERR_OK) {
        return e;
    }
    if (g_root[0] == '\0') {
        return ERR_IO;
    }
    for (i = 0u; g_root[i] != '\0'; i++) {
        if (o + 2u >= out_sz) {
            return ERR_NOSPC;
        }
        out[o++] = g_root[i];
    }
    if (rel[0] == '/' && rel[1] == '\0') {
        out[o] = '\0';
        return ERR_OK;
    }
    for (i = 0u; rel[i] != '\0'; i++) {
        if (o + 1u >= out_sz) {
            return ERR_NOSPC;
        }
        out[o++] = (rel[i] == '/') ? HOST_SEP : rel[i];
    }
    out[o] = '\0';
    return ERR_OK;
}

err_t vfs_host_set_root(const char *path)
{
    size_t n;
    size_t i;

    if (path == NULL || path[0] == '\0') {
        return ERR_INVAL;
    }
    n = strlen(path);
    if (n + 1u > sizeof(g_root)) {
        return ERR_NOSPC;
    }
    memcpy(g_root, path, n + 1u);
    i = n;
    while (i > 1u && (g_root[i - 1u] == '/' || g_root[i - 1u] == '\\')) {
        g_root[--i] = '\0';
    }
    return ERR_OK;
}

const char *vfs_host_root(void)
{
    return g_root;
}

err_t vfs_mount(void)
{
#ifdef _WIN32
    DWORD a;
#else
    struct stat st;
#endif

    if (g_root[0] == '\0') {
        return ERR_INVAL;
    }
#ifdef _WIN32
    a = GetFileAttributesA(g_root);
    if (a == INVALID_FILE_ATTRIBUTES || (a & FILE_ATTRIBUTE_DIRECTORY) == 0u) {
        return ERR_NOENT;
    }
#else
    if (stat(g_root, &st) != 0 || !S_ISDIR(st.st_mode)) {
        return ERR_NOENT;
    }
#endif
    memset(g_fil, 0, sizeof(g_fil));
    memset(g_dir, 0, sizeof(g_dir));
    memset(g_fil_used, 0, sizeof(g_fil_used));
    memset(g_dir_used, 0, sizeof(g_dir_used));
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

err_t vfs_open(const char *path, uint32_t flags, vfs_file_t *fd)
{
    char host[VFS_PATH_MAX * 2u];
    const char *mode;
    int slot;
    err_t e;

    if (fd == NULL) {
        return ERR_INVAL;
    }
    *fd = -1;
    if (!g_mounted) {
        return ERR_IO;
    }
    e = to_host(path, host, sizeof(host));
    if (e != ERR_OK) {
        return e;
    }
    if ((flags & VFS_O_TRUNC) != 0u) {
        mode = "wb+";
    } else if ((flags & VFS_O_CREAT) != 0u) {
        mode = "ab+";
    } else if ((flags & VFS_O_WR) != 0u) {
        mode = "rb+";
    } else {
        mode = "rb";
    }
    slot = alloc_fil();
    if (slot < 0) {
        return ERR_BUSY;
    }
    g_fil[slot] = fopen(host, mode);
    if (g_fil[slot] == NULL) {
        g_fil_used[slot] = 0u;
        return (errno == ENOENT) ? ERR_NOENT : ERR_IO;
    }
    if ((flags & VFS_O_CREAT) != 0u && (flags & VFS_O_TRUNC) == 0u) {
        (void)fseek(g_fil[slot], 0, SEEK_SET);
    }
    *fd = slot;
    return ERR_OK;
}

err_t vfs_read(vfs_file_t fd, void *buf, size_t n, size_t *got)
{
    size_t br;

    if (got != NULL) {
        *got = 0u;
    }
    if (fd < 0 || fd >= VFS_FILE_MAX || g_fil_used[fd] == 0u || buf == NULL) {
        return ERR_INVAL;
    }
    br = fread(buf, 1, n, g_fil[fd]);
    if (got != NULL) {
        *got = br;
    }
    if (br < n && ferror(g_fil[fd]) != 0) {
        return ERR_IO;
    }
    return ERR_OK;
}

err_t vfs_write(vfs_file_t fd, const void *buf, size_t n, size_t *put)
{
    size_t bw;

    if (put != NULL) {
        *put = 0u;
    }
    if (fd < 0 || fd >= VFS_FILE_MAX || g_fil_used[fd] == 0u || buf == NULL) {
        return ERR_INVAL;
    }
    bw = fwrite(buf, 1, n, g_fil[fd]);
    if (put != NULL) {
        *put = bw;
    }
    return (bw == n) ? ERR_OK : ERR_NOSPC;
}

err_t vfs_seek(vfs_file_t fd, uint32_t off)
{
    if (fd < 0 || fd >= VFS_FILE_MAX || g_fil_used[fd] == 0u) {
        return ERR_INVAL;
    }
    return (fseek(g_fil[fd], (long)off, SEEK_SET) == 0) ? ERR_OK : ERR_INVAL;
}

err_t vfs_close(vfs_file_t fd)
{
    if (fd < 0 || fd >= VFS_FILE_MAX || g_fil_used[fd] == 0u) {
        return ERR_INVAL;
    }
    (void)fclose(g_fil[fd]);
    g_fil[fd] = NULL;
    g_fil_used[fd] = 0u;
    return ERR_OK;
}

err_t vfs_stat(const char *path, vfs_stat_t *st)
{
    char host[VFS_PATH_MAX * 2u];
    err_t e;
#ifdef _WIN32
    struct _stat s;
#else
    struct stat s;
#endif

    if (st == NULL) {
        return ERR_INVAL;
    }
    memset(st, 0, sizeof(*st));
    if (!g_mounted) {
        return ERR_IO;
    }
    e = to_host(path, host, sizeof(host));
    if (e != ERR_OK) {
        return e;
    }
#ifdef _WIN32
    if (_stat(host, &s) != 0) {
        return ERR_NOENT;
    }
    st->is_dir = ((s.st_mode & _S_IFDIR) != 0) ? 1u : 0u;
#else
    if (stat(host, &s) != 0) {
        return ERR_NOENT;
    }
    st->is_dir = S_ISDIR(s.st_mode) ? 1u : 0u;
#endif
    st->size = (uint32_t)s.st_size;
    return ERR_OK;
}

err_t vfs_opendir(const char *path, vfs_dir_t *dir)
{
    char host[VFS_PATH_MAX * 2u];
    int slot;
    err_t e;
#ifdef _WIN32
    char pat[VFS_PATH_MAX * 2u + 4u];
#endif

    if (dir == NULL) {
        return ERR_INVAL;
    }
    *dir = -1;
    if (!g_mounted) {
        return ERR_IO;
    }
    e = to_host(path, host, sizeof(host));
    if (e != ERR_OK) {
        return e;
    }
    slot = alloc_dir();
    if (slot < 0) {
        return ERR_BUSY;
    }
#ifdef _WIN32
    if (strlen(host) + 3u >= sizeof(pat)) {
        g_dir_used[slot] = 0u;
        return ERR_NOSPC;
    }
    memcpy(pat, host, strlen(host) + 1u);
    strcat(pat, "\\*");
    g_dir[slot].h = FindFirstFileA(pat, &g_dir[slot].fd);
    if (g_dir[slot].h == INVALID_HANDLE_VALUE) {
        g_dir_used[slot] = 0u;
        return ERR_NOENT;
    }
    g_dir[slot].has = 1u;
#else
    g_dir[slot].d = opendir(host);
    if (g_dir[slot].d == NULL) {
        g_dir_used[slot] = 0u;
        return ERR_NOENT;
    }
    memcpy(g_dir[slot].path, host, strlen(host) + 1u);
#endif
    *dir = slot;
    return ERR_OK;
}

err_t vfs_readdir(vfs_dir_t dir, vfs_dirent_t *ent)
{
#ifdef _WIN32
    const char *name;
#else
    struct dirent *de;
    struct stat st;
    char full[VFS_PATH_MAX * 2u];
#endif

    if (dir < 0 || dir >= VFS_DIR_MAX || g_dir_used[dir] == 0u || ent == NULL) {
        return ERR_INVAL;
    }
    memset(ent, 0, sizeof(*ent));
#ifdef _WIN32
    if (g_dir[dir].has == 0u) {
        return ERR_NOENT;
    }
    name = g_dir[dir].fd.cFileName;
    strncpy(ent->name, name, VFS_NAME_MAX - 1u);
    ent->is_dir = ((g_dir[dir].fd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) != 0u) ? 1u : 0u;
    ent->size = g_dir[dir].fd.nFileSizeLow;
    g_dir[dir].has = FindNextFileA(g_dir[dir].h, &g_dir[dir].fd) ? 1u : 0u;
    return ERR_OK;
#else
    de = readdir(g_dir[dir].d);
    if (de == NULL) {
        return ERR_NOENT;
    }
    strncpy(ent->name, de->d_name, VFS_NAME_MAX - 1u);
    if (strlen(g_dir[dir].path) + 1u + strlen(de->d_name) + 1u < sizeof(full)) {
        (void)snprintf(full, sizeof(full), "%s/%s", g_dir[dir].path, de->d_name);
        if (stat(full, &st) == 0) {
            ent->is_dir = S_ISDIR(st.st_mode) ? 1u : 0u;
            ent->size = (uint32_t)st.st_size;
        }
    }
    return ERR_OK;
#endif
}

err_t vfs_closedir(vfs_dir_t dir)
{
    if (dir < 0 || dir >= VFS_DIR_MAX || g_dir_used[dir] == 0u) {
        return ERR_INVAL;
    }
#ifdef _WIN32
    if (g_dir[dir].h != INVALID_HANDLE_VALUE) {
        (void)FindClose(g_dir[dir].h);
    }
    g_dir[dir].h = INVALID_HANDLE_VALUE;
#else
    if (g_dir[dir].d != NULL) {
        (void)closedir(g_dir[dir].d);
    }
    g_dir[dir].d = NULL;
#endif
    g_dir_used[dir] = 0u;
    return ERR_OK;
}

err_t vfs_mkdir(const char *path)
{
    char host[VFS_PATH_MAX * 2u];
    err_t e;

    if (!g_mounted) {
        return ERR_IO;
    }
    e = to_host(path, host, sizeof(host));
    if (e != ERR_OK) {
        return e;
    }
    if (mkdir_one(host) != 0) {
        return (errno == EEXIST) ? ERR_DENIED : ERR_IO;
    }
    return ERR_OK;
}
