#include "app/network.h"

#include "svc/net.h"

#include <string.h>

static uint32_t g_gen;
static char g_link[24];
static char g_ip[20];
static char g_mac[20];
static char g_mode[12];
static char g_path[12];
static uint16_t g_speed;
static net_info_t g_last;

static void set_str(char *dst, size_t n, const char *s)
{
    size_t i;

    if (n == 0u) {
        return;
    }
    if (s == NULL) {
        dst[0] = '\0';
        return;
    }
    for (i = 0u; i + 1u < n && s[i] != '\0'; i++) {
        dst[i] = s[i];
    }
    dst[i] = '\0';
}

static int info_same(const net_info_t *a, const net_info_t *b)
{
    return (a->link == b->link) && (a->ipv4 == b->ipv4) && (a->speed_mbps == b->speed_mbps) &&
           (a->duplex == b->duplex) && (a->dhcp == b->dhcp) && (a->path == b->path) &&
           (a->bar == b->bar) && (memcmp(a->mac, b->mac, 6u) == 0);
}

void network_refresh(void)
{
    net_info_t inf;
    const char *link;
    const char *path;
    const char *mode;

    if (net_service_info(&inf) != ERR_OK) {
        inf.link = NET_LINK_DOWN;
        inf.ipv4 = 0u;
        inf.path = NET_PATH_NONE;
        inf.dhcp = 1u;
        inf.speed_mbps = 0u;
        inf.duplex = 0u;
        inf.bar = 0u;
        memset(inf.mac, 0, sizeof(inf.mac));
    }

    if (info_same(&inf, &g_last) != 0) {
        return;
    }
    g_last = inf;
    g_gen++;
    g_speed = inf.speed_mbps;

    if (inf.link == NET_LINK_UP) {
        link = (inf.duplex != 0u) ? "up full" : "up half";
    } else {
        link = "down";
    }
    set_str(g_link, sizeof(g_link), link);

    if (inf.ipv4 == 0u) {
        set_str(g_ip, sizeof(g_ip), "no ipv4");
    } else {
        net_ipv4_fmt(inf.ipv4, g_ip, sizeof(g_ip));
    }
    net_mac_fmt(inf.mac, g_mac, sizeof(g_mac));
    mode = (inf.dhcp != 0u) ? "dhcp" : "static";
    set_str(g_mode, sizeof(g_mode), mode);
    if (inf.path == NET_PATH_WIFI) {
        path = "wifi";
    } else if (inf.path == NET_PATH_ETH) {
        path = "eth";
    } else {
        path = "none";
    }
    set_str(g_path, sizeof(g_path), path);
}

uint32_t network_gen(void)
{
    return g_gen;
}

const char *network_link_str(void)
{
    return g_link;
}

const char *network_ip_str(void)
{
    return g_ip;
}

const char *network_mac_str(void)
{
    return g_mac;
}

const char *network_mode_str(void)
{
    return g_mode;
}

const char *network_path_str(void)
{
    return g_path;
}

uint16_t network_speed_mbps(void)
{
    return g_speed;
}
