#include "svc/vfs.h"

#include "ff.h"

#include <string.h>

#define VFS_FILE_MAX 4
#define VFS_DIR_MAX 2
#define VFS_MKFS_WORK 16384u

static FATFS g_fs;
static uint8_t g_mounted;
static uint8_t g_did_format;
static uint8_t g_mkfs_work[VFS_MKFS_WORK] __attribute__((section(".dma_buf"), aligned(32)));

static FIL g_fil[VFS_FILE_MAX];
static uint8_t g_fil_used[VFS_FILE_MAX];

static DIR g_dir[VFS_DIR_MAX];
static uint8_t g_dir_used[VFS_DIR_MAX];

static err_t map_fr(FRESULT r)
{
    switch (r) {
    case FR_OK:
        return ERR_OK;
    case FR_NO_FILE:
    case FR_NO_PATH:
        return ERR_NOENT;
    case FR_DENIED:
    case FR_EXIST:
    case FR_WRITE_PROTECTED:
        return ERR_DENIED;
    case FR_INVALID_NAME:
    case FR_INVALID_OBJECT:
    case FR_INVALID_PARAMETER:
        return ERR_INVAL;
    case FR_NOT_ENOUGH_CORE:
        return ERR_NOMEM;
    case FR_TOO_MANY_OPEN_FILES:
    case FR_LOCKED:
        return ERR_BUSY;
    case FR_TIMEOUT:
        return ERR_TIMEOUT;
    case FR_NO_FILESYSTEM:
        return ERR_CORRUPT;
    case FR_MKFS_ABORTED:
        return ERR_IO;
    default:
        return ERR_IO;
    }
}

static err_t to_fat(const char *path, char *fat, size_t fat_sz)
{
    char norm[VFS_PATH_MAX];
    char rel[VFS_PATH_MAX];
    err_t e;
    size_t n;

    e = vfs_normalize(path, norm, sizeof(norm));
    if (e != ERR_OK) {
        return e;
    }
    e = vfs_jail_rel(norm, rel, sizeof(rel));
    if (e != ERR_OK) {
        return e;
    }
    n = strlen(rel);
    if (fat_sz < n + 3u) {
        return ERR_NOSPC;
    }
    fat[0] = '0';
    fat[1] = ':';
    memcpy(fat + 2, rel, n + 1u);
    return ERR_OK;
}

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

err_t vfs_format(void)
{
    MKFS_PARM opt;
    FRESULT r;

    (void)f_mount(0, "0:", 0);
    g_mounted = 0u;
    memset(&opt, 0, sizeof(opt));
    opt.fmt = (BYTE)(FM_FAT | FM_FAT32);
    opt.n_fat = 1;
    opt.align = 8;
    opt.n_root = 0;
    opt.au_size = 0;
    r = f_mkfs("0:", &opt, g_mkfs_work, sizeof(g_mkfs_work));
    if (r != FR_OK) {
        return map_fr(r);
    }
    r = f_mount(&g_fs, "0:", 1);
    if (r != FR_OK) {
        return map_fr(r);
    }
    g_mounted = 1u;
    g_did_format = 1u;
    return ERR_OK;
}

uint8_t vfs_formatted_on_mount(void)
{
    return g_did_format;
}

err_t vfs_mount(void)
{
    FRESULT r;

    if (g_mounted) {
        return ERR_OK;
    }
    g_did_format = 0u;
    r = f_mount(&g_fs, "0:", 1);
    if (r != FR_OK) {
        return map_fr(r);
    }
    g_mounted = 1u;
    return ERR_OK;
}

int vfs_mounted(void)
{
    return g_mounted ? 1 : 0;
}

err_t vfs_open(const char *path, uint32_t flags, vfs_file_t *fd)
{
    char fat[VFS_PATH_MAX + 4];
    BYTE mode = 0;
    int slot;
    err_t e;

    if (fd == NULL) {
        return ERR_INVAL;
    }
    *fd = -1;
    if (!g_mounted) {
        return ERR_IO;
    }
    e = to_fat(path, fat, sizeof(fat));
    if (e != ERR_OK) {
        return e;
    }
    if ((flags & VFS_O_RD) != 0u) {
        mode |= FA_READ;
    }
    if ((flags & VFS_O_WR) != 0u) {
        mode |= FA_WRITE;
    }
    if (mode == 0u) {
        mode = FA_READ;
    }
    if ((flags & VFS_O_CREAT) != 0u) {
        mode |= FA_OPEN_ALWAYS;
    }
    if ((flags & VFS_O_TRUNC) != 0u) {
        mode |= FA_CREATE_ALWAYS;
    }
    slot = alloc_fil();
    if (slot < 0) {
        return ERR_BUSY;
    }
    e = map_fr(f_open(&g_fil[slot], fat, mode));
    if (e != ERR_OK) {
        g_fil_used[slot] = 0u;
        return e;
    }
    *fd = slot;
    return ERR_OK;
}

err_t vfs_read(vfs_file_t fd, void *buf, size_t n, size_t *got)
{
    UINT br = 0;

    if (got != NULL) {
        *got = 0u;
    }
    if (fd < 0 || fd >= VFS_FILE_MAX || g_fil_used[fd] == 0u || buf == NULL) {
        return ERR_INVAL;
    }
    if (map_fr(f_read(&g_fil[fd], buf, (UINT)n, &br)) != ERR_OK) {
        return ERR_IO;
    }
    if (got != NULL) {
        *got = (size_t)br;
    }
    return ERR_OK;
}

err_t vfs_write(vfs_file_t fd, const void *buf, size_t n, size_t *put)
{
    UINT bw = 0;

    if (put != NULL) {
        *put = 0u;
    }
    if (fd < 0 || fd >= VFS_FILE_MAX || g_fil_used[fd] == 0u || buf == NULL) {
        return ERR_INVAL;
    }
    if (map_fr(f_write(&g_fil[fd], buf, (UINT)n, &bw)) != ERR_OK) {
        return ERR_IO;
    }
    if (put != NULL) {
        *put = (size_t)bw;
    }
    return ERR_OK;
}

err_t vfs_seek(vfs_file_t fd, uint32_t off)
{
    if (fd < 0 || fd >= VFS_FILE_MAX || g_fil_used[fd] == 0u) {
        return ERR_INVAL;
    }
    return map_fr(f_lseek(&g_fil[fd], off));
}

err_t vfs_close(vfs_file_t fd)
{
    err_t e;

    if (fd < 0 || fd >= VFS_FILE_MAX || g_fil_used[fd] == 0u) {
        return ERR_INVAL;
    }
    e = map_fr(f_close(&g_fil[fd]));
    g_fil_used[fd] = 0u;
    return e;
}

err_t vfs_stat(const char *path, vfs_stat_t *st)
{
    char fat[VFS_PATH_MAX + 4];
    FILINFO info;
    err_t e;

    if (st == NULL) {
        return ERR_INVAL;
    }
    memset(st, 0, sizeof(*st));
    if (!g_mounted) {
        return ERR_IO;
    }
    e = to_fat(path, fat, sizeof(fat));
    if (e != ERR_OK) {
        return e;
    }
    e = map_fr(f_stat(fat, &info));
    if (e != ERR_OK) {
        return e;
    }
    st->is_dir = ((info.fattrib & AM_DIR) != 0u) ? 1u : 0u;
    st->size = (uint32_t)info.fsize;
    return ERR_OK;
}

err_t vfs_opendir(const char *path, vfs_dir_t *dir)
{
    char fat[VFS_PATH_MAX + 4];
    int slot;
    err_t e;

    if (dir == NULL) {
        return ERR_INVAL;
    }
    *dir = -1;
    if (!g_mounted) {
        return ERR_IO;
    }
    e = to_fat(path, fat, sizeof(fat));
    if (e != ERR_OK) {
        return e;
    }
    slot = alloc_dir();
    if (slot < 0) {
        return ERR_BUSY;
    }
    e = map_fr(f_opendir(&g_dir[slot], fat));
    if (e != ERR_OK) {
        g_dir_used[slot] = 0u;
        return e;
    }
    *dir = slot;
    return ERR_OK;
}

err_t vfs_readdir(vfs_dir_t dir, vfs_dirent_t *ent)
{
    FILINFO info;
    err_t e;

    if (dir < 0 || dir >= VFS_DIR_MAX || g_dir_used[dir] == 0u || ent == NULL) {
        return ERR_INVAL;
    }
    memset(ent, 0, sizeof(*ent));
    for (;;) {
        memset(&info, 0, sizeof(info));
        e = map_fr(f_readdir(&g_dir[dir], &info));
        if (e != ERR_OK) {
            return e;
        }
        if (info.fname[0] == '\0' || (uint8_t)info.fname[0] == 0xFFu) {
            return ERR_NOENT;
        }
        if ((info.fattrib & 0x08u) != 0u) {
            continue;
        }
        if ((info.fattrib & 0x0Fu) == 0x0Fu) {
            continue;
        }
        break;
    }
    strncpy(ent->name, info.fname, VFS_NAME_MAX - 1u);
    ent->name[VFS_NAME_MAX - 1u] = '\0';
    ent->is_dir = ((info.fattrib & AM_DIR) != 0u) ? 1u : 0u;
    ent->size = (uint32_t)info.fsize;
    return ERR_OK;
}

err_t vfs_closedir(vfs_dir_t dir)
{
    err_t e;

    if (dir < 0 || dir >= VFS_DIR_MAX || g_dir_used[dir] == 0u) {
        return ERR_INVAL;
    }
    e = map_fr(f_closedir(&g_dir[dir]));
    g_dir_used[dir] = 0u;
    return e;
}

err_t vfs_mkdir(const char *path)
{
    char fat[VFS_PATH_MAX + 4];
    err_t e;

    if (!g_mounted) {
        return ERR_IO;
    }
    e = to_fat(path, fat, sizeof(fat));
    if (e != ERR_OK) {
        return e;
    }
    return map_fr(f_mkdir(fat));
}
