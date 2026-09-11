#include "app/image_view.h"

#include "svc/media.h"
#include "svc/vfs.h"

#include <string.h>

#if defined(STM32H745xx)
#include "bsp/board.h"
#define IMG_DST_BASE (BOARD_SDRAM_BASE + 0x000C0000u)
static uint16_t *dst_px(void)
{
    return (uint16_t *)IMG_DST_BASE;
}
#else
static uint16_t s_px[IMG_DST_MAX_W * IMG_DST_MAX_H];
static uint16_t *dst_px(void)
{
    return s_px;
}
#endif

#define IMG_LIST_MAX 32

static char g_path[VFS_PATH_MAX];
static char g_name[VFS_NAME_MAX];
static char g_err[40];
static err_t g_st;
static uint16_t g_w;
static uint16_t g_h;
static uint32_t g_gen;

static void bump(void)
{
    g_gen++;
}

static void set_name(const char *path)
{
    const char *slash;
    size_t n;

    g_name[0] = '\0';
    if (path == NULL) {
        return;
    }
    slash = strrchr(path, '/');
    slash = (slash != NULL) ? (slash + 1) : path;
    n = strlen(slash);
    if (n >= VFS_NAME_MAX) {
        n = VFS_NAME_MAX - 1u;
    }
    memcpy(g_name, slash, n);
    g_name[n] = '\0';
}

static void set_err(err_t e)
{
    const char *m;
    size_t k;

    if (e == ERR_CORRUPT) {
        m = "truncated or corrupt";
    } else if (e == ERR_UNSUPPORTED) {
        m = "format not supported";
    } else if (e == ERR_NOSPC) {
        m = "image too large";
    } else if (e == ERR_NOENT) {
        m = "file not found";
    } else {
        m = "can't open image";
    }
    k = strlen(m);
    if (k >= sizeof(g_err)) {
        k = sizeof(g_err) - 1u;
    }
    memcpy(g_err, m, k);
    g_err[k] = '\0';
}

static err_t decode_path(const char *path)
{
    image_buf_t buf;
    image_req_t req;
    size_t n;

    set_name(path);
    n = strlen(path);
    if (n >= VFS_PATH_MAX) {
        g_st = ERR_NOSPC;
        set_err(g_st);
        g_w = 0u;
        g_h = 0u;
        bump();
        return g_st;
    }
    memcpy(g_path, path, n + 1u);
    buf.w = IMG_DST_MAX_W;
    buf.h = IMG_DST_MAX_H;
    buf.stride = IMG_DST_MAX_W;
    buf.px = dst_px();
    req.max_w = IMG_DST_MAX_W;
    req.max_h = IMG_DST_MAX_H;
    g_st = media_decode_image(path, &buf, &req);
    if (g_st == ERR_OK) {
        g_w = buf.w;
        g_h = buf.h;
        g_err[0] = '\0';
    } else {
        g_w = 0u;
        g_h = 0u;
        set_err(g_st);
    }
    bump();
    return g_st;
}

err_t image_view_open(const char *path)
{
    image_view_close();
    if (path == NULL || path[0] == '\0') {
        g_st = ERR_INVAL;
        set_err(g_st);
        bump();
        return g_st;
    }
    return decode_path(path);
}

void image_view_close(void)
{
    g_path[0] = '\0';
    g_name[0] = '\0';
    g_err[0] = '\0';
    g_st = ERR_OK;
    g_w = 0u;
    g_h = 0u;
    bump();
}

static void sort_names(char list[][VFS_NAME_MAX], int n)
{
    int i;
    int j;
    char tmp[VFS_NAME_MAX];

    for (i = 1; i < n; i++) {
        memcpy(tmp, list[i], VFS_NAME_MAX);
        j = i;
        while (j > 0 && strcmp(tmp, list[j - 1]) < 0) {
            memcpy(list[j], list[j - 1], VFS_NAME_MAX);
            j--;
        }
        memcpy(list[j], tmp, VFS_NAME_MAX);
    }
}

static int collect_images(const char *parent, char list[][VFS_NAME_MAX])
{
    vfs_dir_t d = -1;
    vfs_dirent_t ent;
    int n = 0;
    err_t e;

    e = vfs_opendir(parent, &d);
    if (e != ERR_OK) {
        return -1;
    }
    for (;;) {
        e = vfs_readdir(d, &ent);
        if (e == ERR_NOENT) {
            break;
        }
        if (e != ERR_OK) {
            (void)vfs_closedir(d);
            return -1;
        }
        if (ent.is_dir != 0u || media_probe_ext(ent.name) != MEDIA_KIND_IMAGE) {
            continue;
        }
        if (n < IMG_LIST_MAX) {
            memcpy(list[n], ent.name, VFS_NAME_MAX);
            n++;
        }
    }
    (void)vfs_closedir(d);
    sort_names(list, n);
    return n;
}

err_t image_view_next(int dir)
{
    char parent[VFS_PATH_MAX];
    char list[IMG_LIST_MAX][VFS_NAME_MAX];
    char next_path[VFS_PATH_MAX];
    err_t e;
    int n;
    int cur = -1;
    int i;
    int pick;

    if (g_path[0] == '\0') {
        return ERR_INVAL;
    }
    e = vfs_path_parent(g_path, parent, sizeof(parent));
    if (e != ERR_OK) {
        return e;
    }
    n = collect_images(parent, list);
    if (n <= 0) {
        return ERR_NOENT;
    }
    for (i = 0; i < n; i++) {
        if (strcmp(list[i], g_name) == 0) {
            cur = i;
            break;
        }
    }
    if (cur < 0) {
        return ERR_NOENT;
    }
    pick = cur + ((dir >= 0) ? 1 : -1);
    if (pick < 0 || pick >= n) {
        return ERR_NOENT;
    }
    e = vfs_path_join(parent, list[pick], next_path, sizeof(next_path));
    if (e != ERR_OK) {
        return e;
    }
    return decode_path(next_path);
}

const uint16_t *image_view_pixels(void)
{
    return dst_px();
}

uint16_t image_view_w(void)
{
    return g_w;
}

uint16_t image_view_h(void)
{
    return g_h;
}

uint16_t image_view_stride(void)
{
    return IMG_DST_MAX_W;
}

const char *image_view_name(void)
{
    return g_name;
}

const char *image_view_err_str(void)
{
    return g_err;
}

err_t image_view_status(void)
{
    return g_st;
}

uint32_t image_view_gen(void)
{
    return g_gen;
}
