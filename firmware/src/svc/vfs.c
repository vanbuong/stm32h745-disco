#include "svc/vfs.h"

#include "ff.h"
#include "osal/osal.h"

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

/* FatFs is not reentrant (FF_FS_REENTRANT=0). ui and zb share this volume. */
static osal_mutex_t *g_lock;

static err_t vfs_lock(void)
{
    err_t e;

    /* Boot bring-up is single-threaded. Creating a FreeRTOS mutex here
     * masks SysTick until the scheduler starts (vfs_ms 0, then hang). */
    if (osal_scheduler_running() == 0u) {
        return ERR_OK;
    }
    e = osal_mutex_ensure(&g_lock);
    if (e != ERR_OK) {
        return e;
    }
    if (g_lock == NULL) {
        return ERR_IO;
    }
    return osal_mutex_lock(g_lock, OSAL_WAIT_FOREVER);
}

static void vfs_unlock(void)
{
    if (g_lock == NULL) {
        return;
    }
    (void)osal_mutex_unlock(g_lock);
}

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

static err_t mount_unlocked(void)
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

static err_t unmount_unlocked(void)
{
    FRESULT r;

    if (g_mounted == 0u) {
        return ERR_OK;
    }
    r = f_mount(0, "0:", 0);
    g_mounted = 0u;
    return map_fr(r);
}

static err_t format_unlocked(void)
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

err_t vfs_format(void)
{
    err_t e;

    e = vfs_lock();
    if (e != ERR_OK) {
        return e;
    }
    e = format_unlocked();
    vfs_unlock();
    return e;
}

uint8_t vfs_formatted_on_mount(void)
{
    uint8_t v;

    if (vfs_lock() != ERR_OK) {
        return 0u;
    }
    v = g_did_format;
    vfs_unlock();
    return v;
}

err_t vfs_mount(void)
{
    err_t e;

    e = vfs_lock();
    if (e != ERR_OK) {
        return e;
    }
    e = mount_unlocked();
    vfs_unlock();
    return e;
}

int vfs_mounted(void)
{
    int m;

    if (vfs_lock() != ERR_OK) {
        return 0;
    }
    m = g_mounted ? 1 : 0;
    vfs_unlock();
    return m;
}

err_t vfs_unmount(void)
{
    err_t e;

    e = vfs_lock();
    if (e != ERR_OK) {
        return e;
    }
    e = unmount_unlocked();
    vfs_unlock();
    return e;
}

err_t vfs_remount(void)
{
    err_t e;

    e = vfs_lock();
    if (e != ERR_OK) {
        return e;
    }
    e = unmount_unlocked();
    if (e == ERR_OK) {
        e = mount_unlocked();
    }
    vfs_unlock();
    return e;
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
    e = vfs_lock();
    if (e != ERR_OK) {
        return e;
    }
    if (!g_mounted) {
        vfs_unlock();
        return ERR_IO;
    }
    slot = alloc_fil();
    if (slot < 0) {
        vfs_unlock();
        return ERR_BUSY;
    }
    e = map_fr(f_open(&g_fil[slot], fat, mode));
    if (e != ERR_OK) {
        g_fil_used[slot] = 0u;
        vfs_unlock();
        return e;
    }
    *fd = slot;
    vfs_unlock();
    return ERR_OK;
}

err_t vfs_read(vfs_file_t fd, void *buf, size_t n, size_t *got)
{
    UINT br = 0;
    err_t e;

    if (got != NULL) {
        *got = 0u;
    }
    e = vfs_lock();
    if (e != ERR_OK) {
        return e;
    }
    if (fd < 0 || fd >= VFS_FILE_MAX || g_fil_used[fd] == 0u || buf == NULL) {
        vfs_unlock();
        return ERR_INVAL;
    }
    if (map_fr(f_read(&g_fil[fd], buf, (UINT)n, &br)) != ERR_OK) {
        vfs_unlock();
        return ERR_IO;
    }
    if (got != NULL) {
        *got = (size_t)br;
    }
    vfs_unlock();
    return ERR_OK;
}

err_t vfs_write(vfs_file_t fd, const void *buf, size_t n, size_t *put)
{
    UINT bw = 0;
    err_t e;

    if (put != NULL) {
        *put = 0u;
    }
    e = vfs_lock();
    if (e != ERR_OK) {
        return e;
    }
    if (fd < 0 || fd >= VFS_FILE_MAX || g_fil_used[fd] == 0u || buf == NULL) {
        vfs_unlock();
        return ERR_INVAL;
    }
    if (map_fr(f_write(&g_fil[fd], buf, (UINT)n, &bw)) != ERR_OK) {
        vfs_unlock();
        return ERR_IO;
    }
    if (put != NULL) {
        *put = (size_t)bw;
    }
    vfs_unlock();
    return ERR_OK;
}

err_t vfs_seek(vfs_file_t fd, uint32_t off)
{
    err_t e;

    e = vfs_lock();
    if (e != ERR_OK) {
        return e;
    }
    if (fd < 0 || fd >= VFS_FILE_MAX || g_fil_used[fd] == 0u) {
        vfs_unlock();
        return ERR_INVAL;
    }
    e = map_fr(f_lseek(&g_fil[fd], off));
    vfs_unlock();
    return e;
}

err_t vfs_close(vfs_file_t fd)
{
    err_t e;

    e = vfs_lock();
    if (e != ERR_OK) {
        return e;
    }
    if (fd < 0 || fd >= VFS_FILE_MAX || g_fil_used[fd] == 0u) {
        vfs_unlock();
        return ERR_INVAL;
    }
    e = map_fr(f_close(&g_fil[fd]));
    g_fil_used[fd] = 0u;
    vfs_unlock();
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
    e = to_fat(path, fat, sizeof(fat));
    if (e != ERR_OK) {
        return e;
    }
    e = vfs_lock();
    if (e != ERR_OK) {
        return e;
    }
    if (!g_mounted) {
        vfs_unlock();
        return ERR_IO;
    }
    e = map_fr(f_stat(fat, &info));
    if (e != ERR_OK) {
        vfs_unlock();
        return e;
    }
    st->is_dir = ((info.fattrib & AM_DIR) != 0u) ? 1u : 0u;
    st->size = (uint32_t)info.fsize;
    vfs_unlock();
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
    e = to_fat(path, fat, sizeof(fat));
    if (e != ERR_OK) {
        return e;
    }
    e = vfs_lock();
    if (e != ERR_OK) {
        return e;
    }
    if (!g_mounted) {
        vfs_unlock();
        return ERR_IO;
    }
    slot = alloc_dir();
    if (slot < 0) {
        vfs_unlock();
        return ERR_BUSY;
    }
    e = map_fr(f_opendir(&g_dir[slot], fat));
    if (e != ERR_OK) {
        g_dir_used[slot] = 0u;
        vfs_unlock();
        return e;
    }
    *dir = slot;
    vfs_unlock();
    return ERR_OK;
}

err_t vfs_readdir(vfs_dir_t dir, vfs_dirent_t *ent)
{
    FILINFO info;
    err_t e;

    e = vfs_lock();
    if (e != ERR_OK) {
        return e;
    }
    if (dir < 0 || dir >= VFS_DIR_MAX || g_dir_used[dir] == 0u || ent == NULL) {
        vfs_unlock();
        return ERR_INVAL;
    }
    memset(ent, 0, sizeof(*ent));
    for (;;) {
        memset(&info, 0, sizeof(info));
        e = map_fr(f_readdir(&g_dir[dir], &info));
        if (e != ERR_OK) {
            vfs_unlock();
            return e;
        }
        if (info.fname[0] == '\0' || (uint8_t)info.fname[0] == 0xFFu) {
            vfs_unlock();
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
    vfs_unlock();
    return ERR_OK;
}

err_t vfs_closedir(vfs_dir_t dir)
{
    err_t e;

    e = vfs_lock();
    if (e != ERR_OK) {
        return e;
    }
    if (dir < 0 || dir >= VFS_DIR_MAX || g_dir_used[dir] == 0u) {
        vfs_unlock();
        return ERR_INVAL;
    }
    e = map_fr(f_closedir(&g_dir[dir]));
    g_dir_used[dir] = 0u;
    vfs_unlock();
    return e;
}

err_t vfs_mkdir(const char *path)
{
    char fat[VFS_PATH_MAX + 4];
    err_t e;

    e = to_fat(path, fat, sizeof(fat));
    if (e != ERR_OK) {
        return e;
    }
    e = vfs_lock();
    if (e != ERR_OK) {
        return e;
    }
    if (!g_mounted) {
        vfs_unlock();
        return ERR_IO;
    }
    e = map_fr(f_mkdir(fat));
    vfs_unlock();
    return e;
}

err_t vfs_unlink(const char *path)
{
    char fat[VFS_PATH_MAX + 4];
    err_t e;

    e = to_fat(path, fat, sizeof(fat));
    if (e != ERR_OK) {
        return e;
    }
    e = vfs_lock();
    if (e != ERR_OK) {
        return e;
    }
    if (!g_mounted) {
        vfs_unlock();
        return ERR_IO;
    }
    e = map_fr(f_unlink(fat));
    vfs_unlock();
    return e;
}

err_t vfs_rename(const char *from, const char *to)
{
    char fat_from[VFS_PATH_MAX + 4];
    char fat_to[VFS_PATH_MAX + 4];
    err_t e;

    e = to_fat(from, fat_from, sizeof(fat_from));
    if (e != ERR_OK) {
        return e;
    }
    e = to_fat(to, fat_to, sizeof(fat_to));
    if (e != ERR_OK) {
        return e;
    }
    e = vfs_lock();
    if (e != ERR_OK) {
        return e;
    }
    if (!g_mounted) {
        vfs_unlock();
        return ERR_IO;
    }
    e = map_fr(f_rename(fat_from, fat_to));
    vfs_unlock();
    return e;
}
