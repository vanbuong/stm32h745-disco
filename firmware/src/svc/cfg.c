#include "svc/cfg.h"

#include "svc/vfs.h"

#include <string.h>

#define CFG_MAGIC0 (uint8_t)'C'
#define CFG_MAGIC1 (uint8_t)'F'
#define CFG_MAGIC2 (uint8_t)'G'
#define CFG_MAGIC3 (uint8_t)'1'
#define CFG_SIZE 8u

static uint8_t g_ready;
static uint8_t g_dirty;
static uint8_t g_bright = CFG_BRIGHT_DEFAULT;
static uint8_t g_vol = CFG_VOL_DEFAULT;
static uint8_t g_ch = CFG_ZB_CH_DEFAULT;
static uint8_t g_join = CFG_JOIN_S_DEFAULT;

static uint8_t clamp_u8(uint8_t v, uint8_t lo, uint8_t hi)
{
    if (v < lo) {
        return lo;
    }
    if (v > hi) {
        return hi;
    }
    return v;
}

static void defaults(void)
{
    g_bright = CFG_BRIGHT_DEFAULT;
    g_vol = CFG_VOL_DEFAULT;
    g_ch = CFG_ZB_CH_DEFAULT;
    g_join = CFG_JOIN_S_DEFAULT;
    g_dirty = 0u;
}

static err_t persist_save(void)
{
    uint8_t buf[CFG_SIZE];
    vfs_file_t fd = -1;
    size_t put = 0u;
    err_t e;

    if (vfs_mounted() == 0) {
        return ERR_IO;
    }
    buf[0] = CFG_MAGIC0;
    buf[1] = CFG_MAGIC1;
    buf[2] = CFG_MAGIC2;
    buf[3] = CFG_MAGIC3;
    buf[4] = g_bright;
    buf[5] = g_vol;
    buf[6] = g_ch;
    buf[7] = g_join;
    e = vfs_open(CFG_PATH, VFS_O_WR | VFS_O_CREAT | VFS_O_TRUNC, &fd);
    if (e != ERR_OK) {
        return e;
    }
    e = vfs_write(fd, buf, CFG_SIZE, &put);
    (void)vfs_close(fd);
    if (e != ERR_OK || put != CFG_SIZE) {
        return (e != ERR_OK) ? e : ERR_NOSPC;
    }
    g_dirty = 0u;
    return ERR_OK;
}

static uint8_t persist_load(void)
{
    uint8_t buf[CFG_SIZE];
    vfs_file_t fd = -1;
    size_t got = 0u;

    if (vfs_mounted() == 0) {
        return 0u;
    }
    if (vfs_open(CFG_PATH, VFS_O_RD, &fd) != ERR_OK) {
        return 0u;
    }
    if (vfs_read(fd, buf, CFG_SIZE, &got) != ERR_OK || got < CFG_SIZE) {
        (void)vfs_close(fd);
        return 0u;
    }
    (void)vfs_close(fd);
    if (buf[0] != CFG_MAGIC0 || buf[1] != CFG_MAGIC1 || buf[2] != CFG_MAGIC2 ||
        buf[3] != CFG_MAGIC3) {
        return 0u;
    }
    g_bright = clamp_u8(buf[4], CFG_BRIGHT_MIN, 100u);
    g_vol = clamp_u8(buf[5], 0u, 100u);
    g_ch = clamp_u8(buf[6], CFG_ZB_CH_MIN, CFG_ZB_CH_MAX);
    g_join = clamp_u8(buf[7], 1u, 254u);
    g_dirty = 0u;
    return 1u;
}

err_t cfg_init(void)
{
    defaults();
    g_ready = 1u;
    (void)persist_load();
    return ERR_OK;
}

void cfg_reset(void)
{
    defaults();
    g_ready = 0u;
}

void cfg_poll(void)
{
    if (g_ready != 0u && g_dirty != 0u) {
        (void)persist_save();
    }
}

uint8_t cfg_brightness(void)
{
    return g_bright;
}

uint8_t cfg_volume(void)
{
    return g_vol;
}

uint8_t cfg_zb_channel(void)
{
    return g_ch;
}

uint8_t cfg_join_s(void)
{
    return g_join;
}

static err_t commit(void)
{
    if (g_ready == 0u) {
        g_ready = 1u;
    }
    g_dirty = 1u;
    return persist_save();
}

err_t cfg_set_brightness(uint8_t pct)
{
    g_bright = clamp_u8(pct, CFG_BRIGHT_MIN, 100u);
    return commit();
}

err_t cfg_set_volume(uint8_t pct)
{
    g_vol = clamp_u8(pct, 0u, 100u);
    return commit();
}

err_t cfg_set_zb_channel(uint8_t ch)
{
    g_ch = clamp_u8(ch, CFG_ZB_CH_MIN, CFG_ZB_CH_MAX);
    return commit();
}

err_t cfg_set_join_s(uint8_t seconds)
{
    g_join = clamp_u8(seconds, 1u, 254u);
    return commit();
}
