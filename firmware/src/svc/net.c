#include "svc/net.h"

#include <string.h>

#define NET_ETH_LEASE 0xC0A80132u /* 192.168.1.50 */
#define NET_WIFI_LEASE 0xC0A80402u /* 192.168.4.2 */

static uint8_t g_ready;
static uint8_t g_wifi_ok;
static net_path_t g_path;
static uint32_t g_eth_down_ms;
static uint8_t g_bar;
static net_info_t g_info;

#if !defined(CORE_CM7)

typedef struct {
    uint8_t inited;
    net_link_t link;
    uint32_t ipv4;
    uint8_t mac[6];
    uint16_t speed_mbps;
    uint8_t duplex;
    uint8_t dhcp;
    uint32_t static_ip;
    uint32_t static_mask;
    uint32_t static_gw;
    uint32_t link_up_ms;
    uint8_t saw_up;
} fake_if_t;

static fake_if_t g_eth;
static fake_if_t g_wifi;
static uint32_t g_dhcp_delay_ms;

static fake_if_t *fake_of(net_if_id_t id)
{
    if (id == NET_IF_ETH) {
        return &g_eth;
    }
    if (id == NET_IF_WIFI) {
        return &g_wifi;
    }
    return NULL;
}

static void fake_apply_addr(fake_if_t *n, uint32_t now_ms)
{
    if (n == NULL || n->link != NET_LINK_UP) {
        if (n != NULL) {
            n->ipv4 = 0u;
            n->saw_up = 0u;
        }
        return;
    }
    if (n->saw_up == 0u) {
        n->link_up_ms = now_ms;
        n->saw_up = 1u;
    }
    if (n->dhcp == 0u) {
        n->ipv4 = n->static_ip;
        return;
    }
    if ((now_ms - n->link_up_ms) >= g_dhcp_delay_ms) {
        n->ipv4 = (n == &g_wifi) ? NET_WIFI_LEASE : NET_ETH_LEASE;
    }
}

err_t net_if_init(net_if_id_t id)
{
    fake_if_t *n = fake_of(id);

    if (n == NULL) {
        return ERR_INVAL;
    }
    memset(n, 0, sizeof(*n));
    n->inited = 1u;
    n->dhcp = 1u;
    n->duplex = 1u;
    if (id == NET_IF_ETH) {
        n->mac[0] = 0x02u;
        n->mac[5] = 0x01u;
        n->speed_mbps = 100u;
    } else {
        n->mac[0] = 0x02u;
        n->mac[5] = 0x02u;
        n->speed_mbps = 0u;
    }
    return ERR_OK;
}

void net_if_poll(net_if_id_t id, uint32_t now_ms)
{
    fake_if_t *n = fake_of(id);

    if (n == NULL || n->inited == 0u) {
        return;
    }
    fake_apply_addr(n, now_ms);
}

err_t net_if_status(net_if_id_t id, net_status_t *out)
{
    fake_if_t *n = fake_of(id);

    if (n == NULL || out == NULL) {
        return ERR_INVAL;
    }
    if (n->inited == 0u) {
        return ERR_IO;
    }
    out->link = n->link;
    out->ipv4 = n->ipv4;
    memcpy(out->mac, n->mac, 6u);
    out->speed_mbps = n->speed_mbps;
    out->duplex = n->duplex;
    out->dhcp = n->dhcp;
    return ERR_OK;
}

err_t net_if_set_dhcp(net_if_id_t id)
{
    fake_if_t *n = fake_of(id);

    if (n == NULL || n->inited == 0u) {
        return ERR_INVAL;
    }
    n->dhcp = 1u;
    n->ipv4 = 0u;
    n->saw_up = 0u;
    return ERR_OK;
}

err_t net_if_set_static(net_if_id_t id, uint32_t ipv4, uint32_t mask, uint32_t gw)
{
    fake_if_t *n = fake_of(id);

    if (n == NULL || n->inited == 0u) {
        return ERR_INVAL;
    }
    n->dhcp = 0u;
    n->static_ip = ipv4;
    n->static_mask = mask;
    n->static_gw = gw;
    n->saw_up = 0u;
    return ERR_OK;
}

void net_test_set_eth_link(net_link_t link, uint16_t speed_mbps, uint8_t duplex)
{
    g_eth.link = link;
    g_eth.speed_mbps = speed_mbps;
    g_eth.duplex = (duplex != 0u) ? 1u : 0u;
    g_eth.saw_up = 0u;
    if (link != NET_LINK_UP) {
        g_eth.ipv4 = 0u;
    }
}

void net_test_set_wifi_link(net_link_t link)
{
    g_wifi.link = link;
    g_wifi.speed_mbps = (link == NET_LINK_UP) ? 54u : 0u;
    g_wifi.duplex = 1u;
    g_wifi.saw_up = 0u;
    if (link != NET_LINK_UP) {
        g_wifi.ipv4 = 0u;
    }
}

void net_test_set_dhcp_delay_ms(uint32_t ms)
{
    g_dhcp_delay_ms = ms;
}

#endif /* !CORE_CM7 */

static uint8_t bar_from(const net_status_t *st)
{
    if (st->link != NET_LINK_UP) {
        return 1u;
    }
    if (st->ipv4 == 0u) {
        return 2u;
    }
    return 3u;
}

static void copy_info(const net_status_t *st, net_path_t path)
{
    g_info.link = st->link;
    g_info.ipv4 = st->ipv4;
    memcpy(g_info.mac, st->mac, 6u);
    g_info.speed_mbps = st->speed_mbps;
    g_info.duplex = st->duplex;
    g_info.dhcp = st->dhcp;
    g_info.path = path;
    g_info.bar = bar_from(st);
    g_bar = g_info.bar;
}

err_t net_init(void)
{
    return net_if_init(NET_IF_ETH);
}

err_t net_status(net_status_t *out)
{
    return net_if_status(NET_IF_ETH, out);
}

err_t net_service_init(void)
{
    err_t e;

    g_ready = 0u;
    g_wifi_ok = 0u;
    g_path = NET_PATH_ETH;
    g_eth_down_ms = 0xFFFFFFFFu;
    g_bar = 0u;
    memset(&g_info, 0, sizeof(g_info));

    e = net_if_init(NET_IF_ETH);
    if (e != ERR_OK) {
        return e;
    }
    if (net_if_init(NET_IF_WIFI) == ERR_OK) {
        g_wifi_ok = 1u;
    }
    g_ready = 1u;
    g_info.path = NET_PATH_ETH;
    g_info.bar = 1u;
    g_bar = 1u;
    return ERR_OK;
}

void net_service_poll(uint32_t now_ms)
{
    net_status_t eth;
    net_status_t wifi;
    net_status_t *active;

    if (g_ready == 0u) {
        return;
    }

    net_if_poll(NET_IF_ETH, now_ms);
    if (g_wifi_ok != 0u) {
        net_if_poll(NET_IF_WIFI, now_ms);
    }

    if (net_if_status(NET_IF_ETH, &eth) != ERR_OK) {
        memset(&eth, 0, sizeof(eth));
    }
    memset(&wifi, 0, sizeof(wifi));
    if (g_wifi_ok != 0u) {
        (void)net_if_status(NET_IF_WIFI, &wifi);
    }

    if (eth.link == NET_LINK_UP) {
        g_eth_down_ms = 0xFFFFFFFFu;
        g_path = NET_PATH_ETH;
    } else {
        if (g_eth_down_ms == 0xFFFFFFFFu) {
            g_eth_down_ms = now_ms;
        }
        if (g_wifi_ok != 0u && wifi.link == NET_LINK_UP &&
            (now_ms - g_eth_down_ms) >= NET_FAILOVER_MS) {
            g_path = NET_PATH_WIFI;
        } else {
            g_path = NET_PATH_ETH;
        }
    }

    active = (g_path == NET_PATH_WIFI) ? &wifi : &eth;
    copy_info(active, g_path);
}

err_t net_service_status(net_status_t *out)
{
    net_info_t info;
    err_t e;

    if (out == NULL) {
        return ERR_INVAL;
    }
    e = net_service_info(&info);
    if (e != ERR_OK) {
        return e;
    }
    out->link = info.link;
    out->ipv4 = info.ipv4;
    memcpy(out->mac, info.mac, 6u);
    out->speed_mbps = info.speed_mbps;
    out->duplex = info.duplex;
    out->dhcp = info.dhcp;
    return ERR_OK;
}

err_t net_service_info(net_info_t *out)
{
    if (out == NULL) {
        return ERR_INVAL;
    }
    if (g_ready == 0u) {
        memset(out, 0, sizeof(*out));
        return ERR_IO;
    }
    *out = g_info;
    return ERR_OK;
}

uint8_t net_bar_level(void)
{
    if (g_ready == 0u) {
        return 0u;
    }
    return g_bar;
}

static void put_u8(char *out, size_t n, size_t *i, uint8_t v)
{
    char tmp[3];
    unsigned k = 0u;
    unsigned t;
    unsigned x = v;

    if (x >= 100u) {
        tmp[k++] = (char)('0' + (x / 100u));
        x = (unsigned)(x % 100u);
        tmp[k++] = (char)('0' + (x / 10u));
        tmp[k++] = (char)('0' + (x % 10u));
    } else if (x >= 10u) {
        tmp[k++] = (char)('0' + (x / 10u));
        tmp[k++] = (char)('0' + (x % 10u));
    } else {
        tmp[k++] = (char)('0' + x);
    }
    for (t = 0u; t < k && *i + 1u < n; t++) {
        out[(*i)++] = tmp[t];
    }
}

void net_ipv4_fmt(uint32_t ipv4, char *out, size_t n)
{
    size_t i = 0u;
    unsigned b;

    if (out == NULL || n == 0u) {
        return;
    }
    for (b = 0u; b < 4u; b++) {
        uint8_t v = (uint8_t)((ipv4 >> (24u - (8u * b))) & 0xFFu);

        if (b > 0u && i + 1u < n) {
            out[i++] = '.';
        }
        put_u8(out, n, &i, v);
    }
    out[i] = '\0';
}

void net_mac_fmt(const uint8_t mac[6], char *out, size_t n)
{
    static const char hex[] = "0123456789abcdef";
    size_t i;
    size_t o = 0u;

    if (out == NULL || n == 0u) {
        return;
    }
    if (mac == NULL) {
        out[0] = '\0';
        return;
    }
    for (i = 0u; i < 6u && o + 3u < n; i++) {
        if (i > 0u) {
            out[o++] = ':';
        }
        out[o++] = hex[(mac[i] >> 4) & 0x0Fu];
        out[o++] = hex[mac[i] & 0x0Fu];
    }
    out[o] = '\0';
}

err_t net_set_dhcp(void)
{
    err_t e;

    e = net_if_set_dhcp(NET_IF_ETH);
    if (g_wifi_ok != 0u) {
        (void)net_if_set_dhcp(NET_IF_WIFI);
    }
    return e;
}

err_t net_set_static(uint32_t ipv4, uint32_t mask, uint32_t gw)
{
    return net_if_set_static(NET_IF_ETH, ipv4, mask, gw);
}
