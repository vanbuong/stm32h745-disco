#include "svc/zb_host.h"

#include "hal/uart.h"
#include "hal/wdog.h"
#include "svc/log.h"
#include "svc/vfs.h"
#include "svc/znp_mt.h"

#ifdef ZB_IOTDEV_DRIVER
#include "core/zb_core.h"
#include "device/zb_device_manager.h"
#include "svc/cfg.h"
#include "zb_port.h"
#include "zdo/zb_zdo.h"
#include "znp/zb_znp.h"
#endif

#include <string.h>

#define MT_SYS_SREQ 0x21u
#define MT_SYS_SRSP 0x61u
#define MT_SYS_PING 0x01u
#define ZNP_PING_WAIT_MS 200u
#define ZNP_PING_TRIES (UART_ZNP_RESET_MS / ZNP_PING_WAIT_MS)
#define DEV_REC 54u
#define DEV_HDR 6u
#define NET_LEN 18u

static zb_dev_t g_dev[HOME_DEV_MAX];
static size_t g_n;
static zb_net_info_t g_net;
static uint8_t g_ready;
static uint8_t g_dirty;
static uint32_t g_view_gen;
static uint32_t g_permit_acc;
static uint32_t g_now_ms;
static uint8_t s_dev_file[DEV_HDR + (DEV_REC * HOME_DEV_MAX)];
#ifdef ZB_IOTDEV_DRIVER
static uint8_t g_drv;
static uint8_t g_drv_inited;
static uint8_t g_znp_busy;
static uint8_t g_core_busy;
static uint32_t g_sync_acc;
#endif

static err_t forget_local(const uint8_t ieee[8]);

static void copy_str(char *dst, size_t n, const char *s)
{
    size_t i = 0u;

    if (dst == NULL || n == 0u) {
        return;
    }
    if (s == NULL) {
        dst[0] = '\0';
        return;
    }
    while (s[i] != '\0' && i + 1u < n) {
        dst[i] = s[i];
        i++;
    }
    dst[i] = '\0';
}

static int ieee_eq(const uint8_t a[8], const uint8_t b[8])
{
    return memcmp(a, b, 8u) == 0;
}

static void put_le16(uint8_t *p, uint16_t v)
{
    p[0] = (uint8_t)v;
    p[1] = (uint8_t)(v >> 8);
}

static uint16_t get_le16(const uint8_t *p)
{
    return (uint16_t)((uint16_t)p[0] | ((uint16_t)p[1] << 8));
}

static void mark_view(void)
{
    g_view_gen++;
}

static void mark_dirty(void)
{
    g_dirty = 1u;
    mark_view();
}

static size_t room_count(const char *extra)
{
    char seen[HOME_ROOM_CAP][HOME_ROOM_MAX];
    size_t n = 0u;
    size_t i;
    size_t k;

    for (i = 0u; i < g_n; i++) {
        const char *r = g_dev[i].room_id;
        uint8_t dup = 0u;
        if (r[0] == '\0') {
            continue;
        }
        for (k = 0u; k < n; k++) {
            if (strcmp(seen[k], r) == 0) {
                dup = 1u;
                break;
            }
        }
        if (dup == 0u && n < HOME_ROOM_CAP) {
            copy_str(seen[n], HOME_ROOM_MAX, r);
            n++;
        }
    }
    if (extra != NULL && extra[0] != '\0') {
        uint8_t dup = 0u;
        for (k = 0u; k < n; k++) {
            if (strcmp(seen[k], extra) == 0) {
                dup = 1u;
                break;
            }
        }
        if (dup == 0u) {
            n++;
        }
    }
    return n;
}

home_kind_t zb_host_kind_from_clusters(const uint16_t *in, uint8_t n)
{
    uint8_t onoff = 0u;
    uint8_t level = 0u;
    uint8_t occ = 0u;
    uint8_t ias = 0u;
    uint8_t temp = 0u;
    uint8_t i;

    if (in == NULL) {
        return HOME_SWITCH;
    }
    for (i = 0u; i < n; i++) {
        if (in[i] == ZB_CLUSTER_ONOFF) {
            onoff = 1u;
        } else if (in[i] == ZB_CLUSTER_LEVEL) {
            level = 1u;
        } else if (in[i] == ZB_CLUSTER_OCC) {
            occ = 1u;
        } else if (in[i] == ZB_CLUSTER_IAS) {
            ias = 1u;
        } else if (in[i] == ZB_CLUSTER_TEMP) {
            temp = 1u;
        }
    }
    if (occ != 0u || ias != 0u) {
        return HOME_BINARY_SENSOR;
    }
    if (temp != 0u && onoff == 0u) {
        return HOME_CLIMATE;
    }
    if (onoff != 0u && level != 0u) {
        return HOME_LIGHT;
    }
    if (onoff != 0u) {
        return HOME_SWITCH;
    }
    if (temp != 0u) {
        return HOME_CLIMATE;
    }
    return HOME_SWITCH;
}

err_t zb_host_find(const uint8_t ieee[8], size_t *idx)
{
    size_t i;

    if (ieee == NULL) {
        return ERR_INVAL;
    }
    for (i = 0u; i < g_n; i++) {
        if (ieee_eq(g_dev[i].ieee, ieee) != 0) {
            if (idx != NULL) {
                *idx = i;
            }
            return ERR_OK;
        }
    }
    return ERR_NOENT;
}

static err_t write_file(const char *path, const uint8_t *buf, size_t n)
{
    vfs_file_t fd = -1;
    size_t put = 0u;
    err_t e;

    if (vfs_mounted() == 0) {
        return ERR_IO;
    }
    e = vfs_mkdir(ZB_HOME_DIR);
    if (e != ERR_OK && e != ERR_DENIED) {
        return e;
    }
    e = vfs_open(path, VFS_O_WR | VFS_O_CREAT | VFS_O_TRUNC, &fd);
    if (e != ERR_OK) {
        return e;
    }
    e = vfs_write(fd, buf, n, &put);
    (void)vfs_close(fd);
    if (e != ERR_OK || put != n) {
        return (e != ERR_OK) ? e : ERR_NOSPC;
    }
    return ERR_OK;
}

static err_t read_file(const char *path, uint8_t *buf, size_t max, size_t *got)
{
    vfs_file_t fd = -1;
    err_t e;

    if (got != NULL) {
        *got = 0u;
    }
    if (vfs_mounted() == 0) {
        return ERR_IO;
    }
    e = vfs_open(path, VFS_O_RD, &fd);
    if (e != ERR_OK) {
        return e;
    }
    e = vfs_read(fd, buf, max, got);
    (void)vfs_close(fd);
    return e;
}

static void pack_dev(uint8_t *p, const zb_dev_t *d)
{
    memcpy(p, d->ieee, 8u);
    put_le16(p + 8u, d->nwk);
    memcpy(p + 10u, d->name, HOME_NAME_MAX);
    memcpy(p + 34u, d->room_id, HOME_ROOM_MAX);
    p[50] = (uint8_t)d->kind;
    p[51] = d->on;
    p[52] = d->level;
    p[53] = d->lqi;
}

static void unpack_dev(zb_dev_t *d, const uint8_t *p)
{
    memset(d, 0, sizeof(*d));
    d->used = 1u;
    memcpy(d->ieee, p, 8u);
    d->nwk = get_le16(p + 8u);
    memcpy(d->name, p + 10u, HOME_NAME_MAX);
    d->name[HOME_NAME_MAX - 1u] = '\0';
    memcpy(d->room_id, p + 34u, HOME_ROOM_MAX);
    d->room_id[HOME_ROOM_MAX - 1u] = '\0';
    d->kind = (home_kind_t)p[50];
    d->on = p[51];
    d->level = p[52];
    d->lqi = p[53];
    d->ep = 1u;
}

static void persist_save(void)
{
    uint8_t net[NET_LEN];
    size_t nwrite;
    size_t i;
    size_t count;

    count = g_n;
    if (count > HOME_DEV_MAX) {
        count = HOME_DEV_MAX;
    }
    nwrite = DEV_HDR + (count * DEV_REC);
    memset(s_dev_file, 0, nwrite);
    s_dev_file[0] = (uint8_t)'H';
    s_dev_file[1] = (uint8_t)'D';
    s_dev_file[2] = (uint8_t)'E';
    s_dev_file[3] = (uint8_t)'V';
    s_dev_file[4] = 1u;
    s_dev_file[5] = (uint8_t)count;
    for (i = 0u; i < count; i++) {
        pack_dev(s_dev_file + DEV_HDR + (i * DEV_REC), &g_dev[i]);
    }
    memset(net, 0, sizeof(net));
    net[0] = (uint8_t)'Z';
    net[1] = (uint8_t)'N';
    net[2] = (uint8_t)'E';
    net[3] = (uint8_t)'T';
    net[4] = 1u;
    net[5] = g_net.formed;
    net[6] = g_net.channel;
    put_le16(net + 7u, g_net.pan);
    memcpy(net + 9u, g_net.ext_pan, 8u);
    if (write_file(ZB_DEV_PATH, s_dev_file, nwrite) == ERR_OK &&
        write_file(ZB_NET_PATH, net, NET_LEN) == ERR_OK) {
        g_net.persist_ok = 1u;
        g_dirty = 0u;
    } else {
        g_net.persist_ok = 0u;
    }
}

static uint8_t persist_load(void)
{
    uint8_t net[NET_LEN];
    size_t got = 0u;
    size_t ngot = 0u;
    size_t i;
    uint8_t count;

    if (read_file(ZB_DEV_PATH, s_dev_file, sizeof(s_dev_file), &got) != ERR_OK || got < DEV_HDR) {
        return 0u;
    }
    if (s_dev_file[0] != (uint8_t)'H' || s_dev_file[1] != (uint8_t)'D' ||
        s_dev_file[2] != (uint8_t)'E' || s_dev_file[3] != (uint8_t)'V' || s_dev_file[4] != 1u) {
        return 0u;
    }
    count = s_dev_file[5];
    if (count > HOME_DEV_MAX || DEV_HDR + ((size_t)count * DEV_REC) > got) {
        return 0u;
    }
    g_n = 0u;
    for (i = 0u; i < (size_t)count; i++) {
        unpack_dev(&g_dev[i], s_dev_file + DEV_HDR + (i * DEV_REC));
        g_n++;
    }
    if (read_file(ZB_NET_PATH, net, sizeof(net), &ngot) == ERR_OK && ngot >= NET_LEN &&
        net[0] == (uint8_t)'Z' && net[1] == (uint8_t)'N' && net[2] == (uint8_t)'E' &&
        net[3] == (uint8_t)'T' && net[4] == 1u) {
        g_net.formed = net[5];
        g_net.channel = net[6];
        g_net.pan = get_le16(net + 7u);
        memcpy(g_net.ext_pan, net + 9u, 8u);
    }
    g_net.persist_ok = 1u;
    return 1u;
}

static void seed_one(const uint8_t *ieee, uint16_t nwk, home_kind_t kind, const char *name,
                     const char *room, uint8_t on, uint8_t level, uint8_t lqi)
{
    zb_dev_t *d;

    if (g_n >= HOME_DEV_MAX) {
        return;
    }
    d = &g_dev[g_n];
    memset(d, 0, sizeof(*d));
    d->used = 1u;
    memcpy(d->ieee, ieee, 8u);
    d->nwk = nwk;
    d->ep = 1u;
    copy_str(d->name, HOME_NAME_MAX, name);
    copy_str(d->room_id, HOME_ROOM_MAX, room);
    d->kind = kind;
    d->on = on;
    d->level = level;
    d->lqi = lqi;
    g_n++;
}

static void seed_mock(void)
{
    const uint8_t a[8] = {0x01u, 0x00u, 0x00u, 0x00u, 0x00u, 0x00u, 0x00u, 0x01u};
    const uint8_t b[8] = {0x01u, 0x00u, 0x00u, 0x00u, 0x00u, 0x00u, 0x00u, 0x02u};
    const uint8_t c[8] = {0x01u, 0x00u, 0x00u, 0x00u, 0x00u, 0x00u, 0x00u, 0x03u};
    const uint8_t d[8] = {0x01u, 0x00u, 0x00u, 0x00u, 0x00u, 0x00u, 0x00u, 0x04u};

    g_n = 0u;
    memset(g_dev, 0, sizeof(g_dev));
    seed_one(a, 0x0001u, HOME_LIGHT, "Living lamp", "Living", 1u, 80u, 210u);
    seed_one(b, 0x0002u, HOME_SWITCH, "Hall switch", "Hall", 0u, 0u, 180u);
    seed_one(c, 0x0003u, HOME_BINARY_SENSOR, "Front door", "Entrance", 0u, 0u, 160u);
    seed_one(d, 0x0004u, HOME_BINARY_SENSOR, "Motion stair", "Stair", 0u, 0u, 140u);
    g_net.formed = 1u;
    g_net.channel = 15u;
    g_net.pan = 0x1A2Bu;
    g_net.ext_pan[0] = 0xDE;
    g_net.ext_pan[1] = 0xAD;
    g_net.mock = 1u;
    mark_dirty();
}

static void try_sys_ping(void)
{
    uart_cfg_t cfg;
    uint8_t tx[16];
    uint8_t rx[16];
    uint8_t pl[8];
    uint8_t cmd0 = 0u;
    uint8_t cmd1 = 0u;
    uint8_t plen = 0u;
    size_t n = 0u;
    size_t got = 0u;
    uint32_t attempt;

    memset(&cfg, 0, sizeof(cfg));
    cfg.baud = UART_ZNP_BAUD;
    cfg.data_bits = 8u;
    cfg.stop_bits = 1u;
    if (uart_open(UART_ID_ZNP, &cfg) != ERR_OK) {
        g_net.radio_ok = 0u;
        g_net.mock = 1u;
        copy_str(g_net.znp_ver, sizeof(g_net.znp_ver), "none");
        return;
    }
    if (znp_mt_encode(MT_SYS_SREQ, MT_SYS_PING, NULL, 0u, tx, sizeof(tx), &n) != ERR_OK) {
        g_net.radio_ok = 0u;
        g_net.mock = 1u;
        return;
    }
    for (attempt = 0u; attempt < ZNP_PING_TRIES; attempt++) {
        wdog_kick();
        if (uart_write(UART_ID_ZNP, tx, n) != ERR_OK) {
            continue;
        }
        if (uart_read(UART_ID_ZNP, rx, sizeof(rx), &got, ZNP_PING_WAIT_MS) == ERR_OK &&
            znp_mt_decode(rx, got, &cmd0, &cmd1, pl, (uint8_t)sizeof(pl), &plen) == ERR_OK &&
            cmd0 == MT_SYS_SRSP && cmd1 == MT_SYS_PING) {
            g_net.radio_ok = 1u;
            g_net.mock = 0u;
            copy_str(g_net.znp_ver, sizeof(g_net.znp_ver), "ZNP");
            return;
        }
    }
    g_net.radio_ok = 0u;
    g_net.mock = 1u;
    copy_str(g_net.znp_ver, sizeof(g_net.znp_ver), "down");
}

#ifdef ZB_IOTDEV_DRIVER
static void ieee_bytes(uint8_t out[8], uint64_t v)
{
    uint8_t i;

    for (i = 0u; i < 8u; i++) {
        out[i] = (uint8_t)(v >> (8u * i));
    }
}

static uint64_t ieee_u64(const uint8_t in[8])
{
    uint64_t v = 0u;
    uint8_t i;

    for (i = 0u; i < 8u; i++) {
        v |= (uint64_t)in[i] << (8u * i);
    }
    return v;
}

static home_kind_t kind_from_joined(const s_zb_device_joined_info_t *info)
{
    uint8_t onoff = 0u;
    uint8_t level = 0u;
    uint8_t bin = 0u;
    uint8_t temp = 0u;
    uint8_t i;

    if (info == NULL) {
        return HOME_SWITCH;
    }
    for (i = 0u; i < info->function_count; i++) {
        switch (info->functions[i].type) {
        case ZB_FUNC_OCCUPANCY:
        case ZB_FUNC_IAS_MOTION_SENSOR:
        case ZB_FUNC_IAS_CONTACT_SWITCH:
        case ZB_FUNC_IAS_DOOR_WINDOW_HANDLE:
        case ZB_FUNC_IAS_FIRE_SENSOR:
        case ZB_FUNC_IAS_WATER_SENSOR:
        case ZB_FUNC_IAS_CO_SENSOR:
        case ZB_FUNC_IAS_PERSONAL_EMERGENCY:
        case ZB_FUNC_IAS_VIBRATION_SENSOR:
        case ZB_FUNC_IAS_GENERIC_SENSOR:
            bin = 1u;
            break;
        case ZB_FUNC_TEMPERATURE:
            temp = 1u;
            break;
        case ZB_FUNC_DIMMABLE_LIGHT:
        case ZB_FUNC_COLOR_TEMP_LIGHT:
        case ZB_FUNC_COLOR_LIGHT:
        case ZB_FUNC_EXTENDED_COLOR_LIGHT:
        case ZB_FUNC_DIMMABLE_PLUGIN_UNIT:
            onoff = 1u;
            level = 1u;
            break;
        case ZB_FUNC_ONOFF_LIGHT:
        case ZB_FUNC_ONOFF_PLUGIN_UNIT:
        case ZB_FUNC_ONOFF_SMART_PLUG:
        case ZB_FUNC_RELAY:
        case ZB_FUNC_MAINS_POWER_OUTLET:
        case ZB_FUNC_ONOFF_SWITCH:
        case ZB_FUNC_DIMMER_SWITCH:
            onoff = 1u;
            break;
        default:
            break;
        }
    }
    if (bin != 0u) {
        return HOME_BINARY_SENSOR;
    }
    if (temp != 0u && onoff == 0u) {
        return HOME_CLIMATE;
    }
    if (onoff != 0u && level != 0u) {
        return HOME_LIGHT;
    }
    if (temp != 0u) {
        return HOME_CLIMATE;
    }
    return HOME_SWITCH;
}

static home_kind_t kind_from_device(const s_zb_device_t *dev)
{
    s_zb_device_joined_info_t info;
    uint8_t i;
    uint8_t n;

    memset(&info, 0, sizeof(info));
    if (dev == NULL) {
        return HOME_SWITCH;
    }
    n = dev->function_count;
    if (n > ZB_MAX_FUNCTIONS) {
        n = ZB_MAX_FUNCTIONS;
    }
    info.function_count = n;
    for (i = 0u; i < n; i++) {
        info.functions[i].type = dev->functions[i].type;
    }
    return kind_from_joined(&info);
}

static void upsert_driver_dev(s_zb_device_t *dev)
{
    uint8_t ieee[8];
    size_t idx;
    const char *name;
    home_kind_t kind;

    if (dev == NULL) {
        return;
    }
    ieee_bytes(ieee, dev->ieee_addr);
    name = (dev->model[0] != '\0') ? dev->model : "New device";
    kind = kind_from_device(dev);
    if (zb_host_find(ieee, &idx) != ERR_OK) {
        (void)zb_host_add(ieee, dev->nwk_addr, kind, name, "Home");
        return;
    }
    if (g_dev[idx].nwk != dev->nwk_addr) {
        g_dev[idx].nwk = dev->nwk_addr;
        mark_dirty();
    }
    if (dev->model[0] != '\0' && strcmp(g_dev[idx].name, name) != 0) {
        copy_str(g_dev[idx].name, HOME_NAME_MAX, name);
        mark_dirty();
    }
    if (dev->function_count > 0u && g_dev[idx].kind != kind) {
        g_dev[idx].kind = kind;
        mark_dirty();
    }
    if (dev->lqi != 0u && g_dev[idx].lqi != dev->lqi) {
        g_dev[idx].lqi = dev->lqi;
        mark_view();
    }
}

static void import_driver_devices(void)
{
    uint16_t idx = 0u;
    s_zb_device_t *dev;

    while ((dev = zb_device_manager_find_device_from_start_index(&idx)) != NULL) {
        upsert_driver_dev(dev);
        idx++;
    }
}

static void driver_idle(void)
{
    zb_plat_serial_poll();
    zb_os_timer_pump();
    if (g_znp_busy == 0u) {
        g_znp_busy = 1u;
        zb_znp_task();
        g_znp_busy = 0u;
    }
    wdog_kick();
}

static void on_driver_event(const s_zb_event_t *ev)
{
    uint8_t ieee[8];
    s_zb_device_t *dev;
    const char *name;
    uint16_t nwk;

    if (ev == NULL) {
        return;
    }
    ieee_bytes(ieee, ev->ieee_addr);
    switch (ev->type) {
    case ZB_EVENT_NETWORK_INFO:
        g_net.channel = ev->network_info.channel;
        g_net.pan = ev->network_info.pan_id;
        g_net.formed = (ev->network_info.state == ZB_NETWORK_STATE_RUN) ? 1u : 0u;
        g_net.radio_ok =
            (ev->network_info.coordinator_state == ZB_COORDINATOR_STATE_READY) ? 1u : 0u;
        g_net.mock = 0u;
        mark_dirty();
        break;
    case ZB_EVENT_NETWORK_OPEN:
        break;
    case ZB_EVENT_DEVICE_JOINED:
    case ZB_EVENT_DEVICE_UPDATED:
        dev = zb_device_manager_find_by_ieee(ev->ieee_addr);
        nwk = (dev != NULL) ? dev->nwk_addr : 0u;
        name = ev->device_info.model[0] != '\0' ? ev->device_info.model : ev->name;
        if (zb_host_find(ieee, NULL) != ERR_OK) {
            (void)zb_host_add(ieee, nwk, kind_from_joined(&ev->device_info),
                              (name[0] != '\0') ? name : "New device", "Home");
        } else {
            size_t idx = 0u;
            (void)zb_host_apply_announce(nwk, ieee);
            if (zb_host_find(ieee, &idx) == ERR_OK) {
                if (ev->device_info.function_count > 0u) {
                    g_dev[idx].kind = kind_from_joined(&ev->device_info);
                }
                if (name[0] != '\0') {
                    copy_str(g_dev[idx].name, HOME_NAME_MAX, name);
                }
                if (dev != NULL && dev->lqi != 0u) {
                    g_dev[idx].lqi = dev->lqi;
                }
                mark_dirty();
            }
        }
        break;
    case ZB_EVENT_DEVICE_LEFT:
        (void)forget_local(ieee);
        break;
    case ZB_EVENT_LIGHT_ONOFF_STATE:
        (void)zb_host_apply_report(ieee, ZB_CLUSTER_ONOFF, ev->onoff_light.on ? 1u : 0u, 0u);
        break;
    case ZB_EVENT_SWITCH_ONOFF_STATE:
        (void)zb_host_apply_report(ieee, ZB_CLUSTER_ONOFF, ev->switch_onoff.on ? 1u : 0u, 0u);
        break;
    case ZB_EVENT_BINARY_STATE:
        (void)zb_host_apply_report(ieee, ZB_CLUSTER_ONOFF, ev->binary.active ? 1u : 0u, 0u);
        break;
    case ZB_EVENT_LIGHT_DIMMABLE_STATE:
        (void)zb_host_apply_report(ieee, ZB_CLUSTER_LEVEL, 0u, ev->dimmable_light.level);
        break;
    case ZB_EVENT_SWITCH_LEVEL_STATE:
        (void)zb_host_apply_report(ieee, ZB_CLUSTER_LEVEL, 0u, ev->switch_level.level);
        break;
    case ZB_EVENT_SENSOR_OCCUPANCY:
    case ZB_EVENT_SENSOR_IAS_ZONE:
        (void)zb_host_apply_report(ieee, ZB_CLUSTER_OCC, ev->binary.active ? 1u : 0u, 0u);
        break;
    default:
        break;
    }
}

static void driver_init(void)
{
    zb_os_set_idle_pump(driver_idle);
    if (g_drv_inited == 0u) {
        zb_core_init();
        g_drv_inited = 1u;
    }
    if (zb_plat_serial_ok() == 0) {
        g_drv = 0u;
        return;
    }
    g_drv = 1u;
    (void)vfs_mkdir(ZB_HOME_DIR);
    (void)vfs_mkdir("/user/home/zb");
    zb_core_set_auto_permit_join_on_form(false);
    zb_core_apply_default_network_config(cfg_zb_channel(), 0u, 5);
    zb_core_set_event_callback(on_driver_event);
    zb_device_manager_register_event_notify_callback(on_driver_event);
    (void)zb_core_request_start();
}
#endif

void zb_host_reset(void)
{
    memset(g_dev, 0, sizeof(g_dev));
    memset(&g_net, 0, sizeof(g_net));
    g_n = 0u;
    g_ready = 0u;
    g_dirty = 0u;
    g_view_gen = 0u;
    g_permit_acc = 0u;
    g_now_ms = 0u;
#ifdef ZB_IOTDEV_DRIVER
    g_drv = 0u;
    g_core_busy = 0u;
    g_sync_acc = 0u;
#endif
}

err_t zb_host_init(void)
{
    if (g_ready != 0u) {
        return ERR_OK;
    }
    memset(g_dev, 0, sizeof(g_dev));
    memset(&g_net, 0, sizeof(g_net));
    g_n = 0u;
    g_dirty = 0u;
    g_view_gen = 0u;
    g_permit_acc = 0u;
    g_now_ms = 1u;
#ifdef ZB_IOTDEV_DRIVER
    driver_init();
    if (g_drv != 0u) {
        g_net.radio_ok = 1u;
        g_net.mock = 0u;
        copy_str(g_net.znp_ver, sizeof(g_net.znp_ver), "ZNP");
        if (persist_load() == 0u) {
            g_net.persist_ok = 0u;
        }
    } else {
        g_net.radio_ok = 0u;
        g_net.mock = 1u;
        copy_str(g_net.znp_ver, sizeof(g_net.znp_ver), "none");
        if (persist_load() == 0u) {
            seed_mock();
        }
    }
#else
    try_sys_ping();
    if (persist_load() == 0u) {
        seed_mock();
    }
#endif
    g_ready = 1u;
    if (g_dirty != 0u) {
        persist_save();
    }
    log_write((g_net.radio_ok != 0u) ? LOG_INFO : LOG_WARN, "zb", "znp %s",
              g_net.znp_ver[0] != '\0' ? g_net.znp_ver : "?");
    return ERR_OK;
}

void zb_host_poll(uint32_t dt_ms)
{
    if (g_ready == 0u) {
        return;
    }
    g_now_ms += dt_ms;
    if (g_net.permit_left > 0u) {
        g_permit_acc += dt_ms;
        while (g_permit_acc >= 1000u && g_net.permit_left > 0u) {
            g_permit_acc -= 1000u;
            g_net.permit_left--;
            if (g_net.permit_left == 0u) {
                mark_view();
            }
        }
    } else {
        g_permit_acc = 0u;
    }
#ifdef ZB_IOTDEV_DRIVER
    if (g_drv != 0u && g_core_busy == 0u) {
        g_core_busy = 1u;
        driver_idle();
        zb_core_task();
        g_core_busy = 0u;
        g_sync_acc += dt_ms;
        if (g_sync_acc >= 1000u) {
            g_sync_acc = 0u;
            import_driver_devices();
        }
    }
#endif
    if (g_dirty != 0u) {
        persist_save();
    }
}

err_t zb_form(const zb_net_cfg_t *cfg)
{
    if (cfg == NULL) {
        return ERR_INVAL;
    }
#ifdef ZB_IOTDEV_DRIVER
    if (g_drv != 0u && g_core_busy == 0u) {
        zb_core_apply_default_network_config(cfg->channel, 0u, 5);
        if (zb_core_get_running_status() == false) {
            (void)zb_core_request_start();
        }
    }
#endif
    g_net.formed = 1u;
    g_net.channel = cfg->channel;
    g_net.pan = cfg->pan;
    mark_dirty();
    return ERR_OK;
}

err_t zb_permit_join(uint8_t seconds)
{
#ifdef ZB_IOTDEV_DRIVER
    if (g_drv != 0u && g_core_busy == 0u) {
        (void)zb_zdo_permit_join(seconds);
    }
#endif
    g_net.permit_left = seconds;
    g_permit_acc = 0u;
    mark_view();
    return ERR_OK;
}

static err_t forget_local(const uint8_t ieee[8])
{
    size_t idx;
    size_t i;

    if (zb_host_find(ieee, &idx) != ERR_OK) {
        return ERR_NOENT;
    }
    for (i = idx; i + 1u < g_n; i++) {
        g_dev[i] = g_dev[i + 1u];
    }
    g_n--;
    memset(&g_dev[g_n], 0, sizeof(g_dev[0]));
    mark_dirty();
    return ERR_OK;
}

err_t zb_leave(const uint8_t ieee[8])
{
#ifdef ZB_IOTDEV_DRIVER
    if (g_drv != 0u && g_core_busy == 0u && ieee != NULL) {
        uint64_t addr = ieee_u64(ieee);
        s_zb_device_t *dev = zb_device_manager_find_by_ieee(addr);
        if (dev != NULL) {
            (void)zb_zdo_send_mgmt_leave_req(dev->nwk_addr, addr, false, false);
        }
        (void)zb_device_manager_remove_device(addr);
    }
#endif
    return forget_local(ieee);
}

err_t zb_interview(const uint8_t ieee[8])
{
    const uint16_t cl[2] = {ZB_CLUSTER_ONOFF, ZB_CLUSTER_LEVEL};

    return zb_host_apply_clusters(ieee, cl, 2u);
}

size_t zb_host_device_count(void)
{
    return g_n;
}

const zb_dev_t *zb_host_device_at(size_t i)
{
    if (i >= g_n) {
        return NULL;
    }
    return &g_dev[i];
}

zb_dev_t *zb_host_device_mut(size_t i)
{
    if (i >= g_n) {
        return NULL;
    }
    return &g_dev[i];
}

void zb_host_net(zb_net_info_t *out)
{
    if (out == NULL) {
        return;
    }
    *out = g_net;
}

uint8_t zb_host_cmd_allowed(void)
{
    return (g_net.mock != 0u || g_net.radio_ok != 0u) ? 1u : 0u;
}

uint8_t zb_host_dirty(void)
{
    return g_dirty;
}

uint32_t zb_host_gen(void)
{
    return g_view_gen;
}

err_t zb_host_add(const uint8_t ieee[8], uint16_t nwk, home_kind_t kind, const char *name,
                  const char *room)
{
    size_t dummy;

    if (ieee == NULL || name == NULL) {
        return ERR_INVAL;
    }
    if (zb_host_find(ieee, &dummy) == ERR_OK) {
        return ERR_DENIED;
    }
    if (g_n >= HOME_DEV_MAX) {
        return ERR_NOSPC;
    }
    if (room_count(room) > HOME_ROOM_CAP) {
        return ERR_NOSPC;
    }
    seed_one(ieee, nwk, kind, name, room, 0u, 0u, 100u);
    mark_dirty();
    return ERR_OK;
}

err_t zb_host_set_meta(const uint8_t ieee[8], const char *name, const char *room)
{
    size_t idx;
    zb_dev_t *d;

    if (zb_host_find(ieee, &idx) != ERR_OK) {
        return ERR_NOENT;
    }
    d = &g_dev[idx];
    if (room != NULL && strcmp(d->room_id, room) != 0 && room_count(room) > HOME_ROOM_CAP) {
        return ERR_NOSPC;
    }
    if (name != NULL && name[0] != '\0') {
        copy_str(d->name, HOME_NAME_MAX, name);
    }
    if (room != NULL && room[0] != '\0') {
        copy_str(d->room_id, HOME_ROOM_MAX, room);
    }
    mark_dirty();
    return ERR_OK;
}

err_t zb_host_apply_announce(uint16_t nwk, const uint8_t ieee[8])
{
    size_t idx;
    zb_dev_t *d;

    if (ieee == NULL) {
        return ERR_INVAL;
    }
    if (zb_host_find(ieee, &idx) == ERR_OK) {
        if (g_dev[idx].nwk != nwk) {
            g_dev[idx].nwk = nwk;
            mark_view();
        }
        g_dev[idx].interviewing = 1u;
        return ERR_OK;
    }
    if (g_n >= HOME_DEV_MAX) {
        return ERR_NOSPC;
    }
    d = &g_dev[g_n];
    memset(d, 0, sizeof(*d));
    d->used = 1u;
    memcpy(d->ieee, ieee, 8u);
    d->nwk = nwk;
    d->ep = 1u;
    copy_str(d->name, HOME_NAME_MAX, "New device");
    copy_str(d->room_id, HOME_ROOM_MAX, "Home");
    d->kind = HOME_SWITCH;
    d->interviewing = 1u;
    d->lqi = 80u;
    g_n++;
    mark_dirty();
    return ERR_OK;
}

err_t zb_host_apply_clusters(const uint8_t ieee[8], const uint16_t *in, uint8_t n)
{
    size_t idx;

    if (zb_host_find(ieee, &idx) != ERR_OK) {
        return ERR_NOENT;
    }
    g_dev[idx].kind = zb_host_kind_from_clusters(in, n);
    g_dev[idx].interviewing = 0u;
    mark_dirty();
    return ERR_OK;
}

err_t zb_host_apply_report(const uint8_t ieee[8], uint16_t cluster, uint8_t on, uint8_t level)
{
    size_t idx;

    if (zb_host_find(ieee, &idx) != ERR_OK) {
        return ERR_NOENT;
    }
    if (cluster == ZB_CLUSTER_ONOFF || cluster == ZB_CLUSTER_OCC || cluster == ZB_CLUSTER_IAS) {
        g_dev[idx].on = (on != 0u) ? 1u : 0u;
    }
    if (cluster == ZB_CLUSTER_LEVEL) {
        g_dev[idx].level = level;
        g_dev[idx].on = (level > 0u) ? 1u : 0u;
    }
    g_dev[idx].last_seen_ms = g_now_ms;
    mark_dirty();
    return ERR_OK;
}

err_t zb_host_apply_cmd(const uint8_t ieee[8], const home_cmd_t *cmd)
{
    size_t idx;
    zb_dev_t *d;

    if (cmd == NULL) {
        return ERR_INVAL;
    }
    if (zb_host_find(ieee, &idx) != ERR_OK) {
        return ERR_NOENT;
    }
    d = &g_dev[idx];
    if (d->kind == HOME_BINARY_SENSOR || d->kind == HOME_CLIMATE) {
        return ERR_INVAL;
    }
    d->on = (cmd->on != 0u) ? 1u : 0u;
    if (cmd->has_level != 0u) {
        d->level = cmd->level;
    }
    d->last_seen_ms = g_now_ms;
    mark_dirty();
    return ERR_OK;
}

void zb_host_test_set_flags(uint8_t radio_ok, uint8_t mock)
{
    g_net.radio_ok = (radio_ok != 0u) ? 1u : 0u;
    g_net.mock = (mock != 0u) ? 1u : 0u;
}

uint32_t zb_host_now_ms(void)
{
    return g_now_ms;
}
