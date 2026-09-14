#include "ethernetif.h"

#include "stm32h7xx_hal.h"

#include "lwip/dhcp.h"
#include "lwip/etharp.h"
#include "lwip/init.h"
#include "lwip/ip4_addr.h"
#include "lwip/netif.h"
#include "lwip/pbuf.h"
#include "lwip/timeouts.h"
#include "netif/ethernet.h"

#include <string.h>

void board_console_puts(const char *s);

#define ETH_RX_BUF_SIZE 1536u
#define ETH_RX_BUF_CNT ((uint32_t)ETH_RX_DESC_CNT)

uint8_t g_lwip_ram_heap[MEM_SIZE + 64u] __attribute__((section(".eth_dma"), aligned(32), used));

static ETH_HandleTypeDef g_eth;
static ETH_DMADescTypeDef g_rx_desc[ETH_RX_DESC_CNT]
    __attribute__((section(".eth_dma"), aligned(32)));
static ETH_DMADescTypeDef g_tx_desc[ETH_TX_DESC_CNT]
    __attribute__((section(".eth_dma"), aligned(32)));
static uint8_t g_rx_buf[ETH_RX_BUF_CNT][ETH_RX_BUF_SIZE]
    __attribute__((section(".eth_dma"), aligned(32)));
static uint8_t g_tx_buf[ETH_RX_BUF_SIZE] __attribute__((section(".eth_dma"), aligned(32)));
static uint8_t g_rx_used[ETH_RX_BUF_CNT];
static uint8_t g_mac[6];
static uint8_t g_dhcp = 1u;
static uint32_t g_static_ip;
static uint32_t g_static_mask;
static uint32_t g_static_gw;
static struct netif g_netif;
static uint8_t g_lwip_up;
static uint8_t g_started;
static uint16_t g_mac_mbps;
static uint8_t g_mac_duplex;
static uint8_t g_mac_set;
static uint8_t g_poll_log;

static int rx_buf_ok(const uint8_t *p)
{
    uint32_t i;

    if (p == NULL) {
        return 0;
    }
    for (i = 0u; i < ETH_RX_BUF_CNT; i++) {
        if (p == g_rx_buf[i]) {
            return 1;
        }
    }
    return 0;
}

static uint8_t *rx_take(void)
{
    uint32_t i;

    for (i = 0u; i < ETH_RX_BUF_CNT; i++) {
        if (g_rx_used[i] == 0u) {
            g_rx_used[i] = 1u;
            return g_rx_buf[i];
        }
    }
    return NULL;
}

static void rx_give(const uint8_t *p)
{
    uint32_t i;

    for (i = 0u; i < ETH_RX_BUF_CNT; i++) {
        if (p == g_rx_buf[i]) {
            g_rx_used[i] = 0u;
            return;
        }
    }
}

void HAL_ETH_RxAllocateCallback(uint8_t **buff)
{
    if (buff == NULL) {
        return;
    }
    *buff = rx_take();
}

void HAL_ETH_RxLinkCallback(void **p_start, void **p_end, uint8_t *buff, uint16_t length)
{
    struct pbuf *p;

    if (p_start == NULL || p_end == NULL || buff == NULL || length == 0u ||
        length > ETH_RX_BUF_SIZE || rx_buf_ok(buff) == 0) {
        rx_give(buff);
        return;
    }
    p = pbuf_alloc(PBUF_RAW, length, PBUF_RAM);
    if (p != NULL) {
        (void)pbuf_take(p, buff, length);
        if (*p_start == NULL) {
            *p_start = p;
        } else {
            pbuf_cat((struct pbuf *)*p_start, p);
        }
        *p_end = p;
    }
    rx_give(buff);
}

static err_t low_level_output(struct netif *netif, struct pbuf *p)
{
    ETH_BufferTypeDef buf = {0};
    ETH_TxPacketConfigTypeDef cfg = {0};
    u16_t n;

    (void)netif;
    if (p == NULL) {
        return ERR_ARG;
    }
    n = pbuf_copy_partial(p, g_tx_buf, (u16_t)sizeof(g_tx_buf), 0);
    if (n == 0u) {
        return ERR_BUF;
    }
    buf.buffer = g_tx_buf;
    buf.len = n;
    buf.next = NULL;
    cfg.Length = n;
    cfg.TxBuffer = &buf;
    cfg.Attributes = ETH_TX_PACKETS_FEATURES_CSUM | ETH_TX_PACKETS_FEATURES_CRCPAD;
    cfg.CRCPadCtrl = ETH_CRC_PAD_INSERT;
    cfg.ChecksumCtrl = ETH_CHECKSUM_IPHDR_PAYLOAD_INSERT_PHDR_CALC;
    if (HAL_ETH_Transmit(&g_eth, &cfg, 50u) != HAL_OK) {
        return ERR_IF;
    }
    return ERR_OK;
}

static err_t ethernetif_netif_init(struct netif *netif)
{
    if (netif == NULL) {
        return ERR_ARG;
    }
    netif->name[0] = 'e';
    netif->name[1] = '0';
    netif->output = etharp_output;
    netif->linkoutput = low_level_output;
    netif->mtu = 1500u;
    netif->hwaddr_len = ETHARP_HWADDR_LEN;
    memcpy(netif->hwaddr, g_mac, ETHARP_HWADDR_LEN);
    netif->flags = NETIF_FLAG_BROADCAST | NETIF_FLAG_ETHARP;
    return ERR_OK;
}

static void apply_addr(void)
{
    ip4_addr_t ip;
    ip4_addr_t mask;
    ip4_addr_t gw;

    if (g_dhcp != 0u) {
        ip4_addr_set_zero(&ip);
        ip4_addr_set_zero(&mask);
        ip4_addr_set_zero(&gw);
        netif_set_addr(&g_netif, &ip, &mask, &gw);
        (void)dhcp_start(&g_netif);
        return;
    }
    dhcp_stop(&g_netif);
    ip.addr = lwip_htonl(g_static_ip);
    mask.addr = lwip_htonl(g_static_mask);
    gw.addr = lwip_htonl(g_static_gw);
    netif_set_addr(&g_netif, &ip, &mask, &gw);
}

static int mac_speed(uint16_t mbps, uint8_t duplex, ETH_MACConfigTypeDef *cfg)
{
    if (HAL_ETH_GetMACConfig(&g_eth, cfg) != HAL_OK) {
        return -1;
    }
    cfg->Speed = (mbps >= 100u) ? ETH_SPEED_100M : ETH_SPEED_10M;
    cfg->DuplexMode = (duplex != 0u) ? ETH_FULLDUPLEX_MODE : ETH_HALFDUPLEX_MODE;
    (void)HAL_ETH_Stop(&g_eth);
    if (HAL_ETH_SetMACConfig(&g_eth, cfg) != HAL_OK) {
        (void)HAL_ETH_Start(&g_eth);
        return -1;
    }
    return (HAL_ETH_Start(&g_eth) == HAL_OK) ? 0 : -1;
}

u32_t sys_now(void)
{
    return HAL_GetTick();
}

int ethernetif_start(const uint8_t mac[6])
{
    ip4_addr_t z;

    if (mac == NULL) {
        return -1;
    }
    __HAL_RCC_D2SRAM3_CLK_ENABLE();
    memcpy(g_mac, mac, 6u);
    memset(g_rx_desc, 0, sizeof(g_rx_desc));
    memset(g_tx_desc, 0, sizeof(g_tx_desc));
    memset(g_rx_used, 0, sizeof(g_rx_used));

    g_eth.Instance = ETH;
    g_eth.Init.MACAddr = g_mac;
    g_eth.Init.MediaInterface = HAL_ETH_MII_MODE;
    g_eth.Init.TxDesc = g_tx_desc;
    g_eth.Init.RxDesc = g_rx_desc;
    g_eth.Init.RxBuffLen = ETH_RX_BUF_SIZE;
    if (HAL_ETH_Init(&g_eth) != HAL_OK) {
        return -1;
    }
    HAL_NVIC_DisableIRQ(ETH_IRQn);
    /* Do not HAL_ETH_Start until PHY link is up. DMA running on a down
     * link fills descriptors; the first UI poll then HardFaults. */

    lwip_init();
    ip4_addr_set_zero(&z);
    if (netif_add(&g_netif, &z, &z, &z, NULL, ethernetif_netif_init, ethernet_input) == NULL) {
        return -1;
    }
    netif_set_default(&g_netif);
    netif_set_up(&g_netif);
    g_netif.hostname = "h745";
    g_lwip_up = 1u;
    g_started = 1u;
    return 0;
}

void ethernetif_poll(void)
{
    struct pbuf *p;
    unsigned n = 0u;

    if (g_started == 0u || g_mac_set == 0u) {
        return;
    }
    if (g_poll_log == 0u) {
        g_poll_log = 1u;
        board_console_puts("eth poll\r\n");
    }
    while (n < 8u && HAL_ETH_ReadData(&g_eth, (void **)&p) == HAL_OK) {
        n++;
        if (p == NULL) {
            continue;
        }
        if (g_netif.input(p, &g_netif) != ERR_OK) {
            pbuf_free(p);
        }
    }
    sys_check_timeouts();
}

void ethernetif_set_link(int up, uint16_t speed_mbps, uint8_t duplex)
{
    ETH_MACConfigTypeDef cfg;

    if (g_started == 0u) {
        return;
    }
    if (up == 0) {
        if (netif_is_link_up(&g_netif) != 0u) {
            netif_set_link_down(&g_netif);
            dhcp_stop(&g_netif);
            netif_set_addr(&g_netif, IP4_ADDR_ANY4, IP4_ADDR_ANY4, IP4_ADDR_ANY4);
            g_mac_set = 0u;
        }
        return;
    }
    /* Stop/Start only when speed or duplex changes. Doing it every PHY
     * poll resets the DMA rings and DHCP never finishes. */
    if (g_mac_set == 0u || g_mac_mbps != speed_mbps || g_mac_duplex != duplex) {
        if (mac_speed(speed_mbps, duplex, &cfg) != 0) {
            return;
        }
        g_mac_mbps = speed_mbps;
        g_mac_duplex = duplex;
        g_mac_set = 1u;
        board_console_puts("eth mac\r\n");
    }
    if (netif_is_link_up(&g_netif) == 0u) {
        netif_set_link_up(&g_netif);
        apply_addr();
    }
}

void ethernetif_query(uint32_t *ipv4_host, uint8_t mac[6], uint8_t *dhcp)
{
    if (mac != NULL) {
        memcpy(mac, g_mac, 6u);
    }
    if (dhcp != NULL) {
        *dhcp = g_dhcp;
    }
    if (ipv4_host != NULL) {
        *ipv4_host = 0u;
        if (g_lwip_up != 0u && netif_is_up(&g_netif) != 0u) {
            *ipv4_host = lwip_ntohl(ip4_addr_get_u32(netif_ip4_addr(&g_netif)));
        }
    }
}

int ethernetif_set_dhcp(void)
{
    g_dhcp = 1u;
    if (g_started != 0u && netif_is_link_up(&g_netif) != 0u) {
        apply_addr();
    }
    return 0;
}

int ethernetif_set_static(uint32_t ipv4_host, uint32_t mask_host, uint32_t gw_host)
{
    g_dhcp = 0u;
    g_static_ip = ipv4_host;
    g_static_mask = mask_host;
    g_static_gw = gw_host;
    if (g_started != 0u && netif_is_link_up(&g_netif) != 0u) {
        apply_addr();
    }
    return 0;
}

int ethernetif_phy_read(uint32_t addr, uint32_t reg, uint32_t *val)
{
    if (val == NULL) {
        return -1;
    }
    if (HAL_ETH_ReadPHYRegister(&g_eth, addr, reg, val) != HAL_OK) {
        return -1;
    }
    return 0;
}

int ethernetif_phy_write(uint32_t addr, uint32_t reg, uint32_t val)
{
    if (HAL_ETH_WritePHYRegister(&g_eth, addr, reg, val) != HAL_OK) {
        return -1;
    }
    return 0;
}
