#include "hal/net_if.h"

#include "ethernetif.h"
#include "lan8742.h"

#include "cube.h"

#include <string.h>

#define PHY_POLL_MS 200u

static lan8742_Object_t g_phy;
static uint8_t g_eth_ok;
static net_link_t g_link;
static uint16_t g_speed;
static uint8_t g_duplex;
static uint32_t g_last_phy_ms;
static uint8_t g_mac[6];

static int32_t phy_io_read(uint32_t addr, uint32_t reg, uint32_t *val)
{
    return ethernetif_phy_read(addr, reg, val);
}

static int32_t phy_io_write(uint32_t addr, uint32_t reg, uint32_t val)
{
    return ethernetif_phy_write(addr, reg, val);
}

static int32_t phy_io_tick(void)
{
    return (int32_t)HAL_GetTick();
}

void HAL_ETH_MspInit(ETH_HandleTypeDef *heth)
{
    (void)heth;
    __HAL_RCC_D2SRAM3_CLK_ENABLE();
    __HAL_RCC_SYSCFG_CLK_ENABLE();
    __HAL_RCC_ETH1MAC_CLK_ENABLE();
    __HAL_RCC_ETH1TX_CLK_ENABLE();
    __HAL_RCC_ETH1RX_CLK_ENABLE();
    __HAL_RCC_ETH1MAC_FORCE_RESET();
    __HAL_RCC_ETH1MAC_RELEASE_RESET();

    __HAL_RCC_GPIOA_CLK_ENABLE();
    __HAL_RCC_GPIOB_CLK_ENABLE();
    __HAL_RCC_GPIOC_CLK_ENABLE();
    __HAL_RCC_GPIOE_CLK_ENABLE();
    __HAL_RCC_GPIOG_CLK_ENABLE();

    /* MII without PH2/PH3 (QSPI BK2 / ETH CRS/COL). 100 Mbit full-duplex. */
    cube_gpio_af(GPIOA, GPIO_PIN_1, GPIO_AF11_ETH, GPIO_NOPULL);  /* RX_CLK */
    cube_gpio_af(GPIOA, GPIO_PIN_2, GPIO_AF11_ETH, GPIO_NOPULL);  /* MDIO */
    cube_gpio_af(GPIOA, GPIO_PIN_7, GPIO_AF11_ETH, GPIO_NOPULL);  /* RX_DV */
    cube_gpio_af(GPIOB, GPIO_PIN_0, GPIO_AF11_ETH, GPIO_NOPULL);  /* RXD2 */
    cube_gpio_af(GPIOB, GPIO_PIN_1, GPIO_AF11_ETH, GPIO_NOPULL);  /* RXD3 */
    cube_gpio_af(GPIOC, GPIO_PIN_1, GPIO_AF11_ETH, GPIO_NOPULL);  /* MDC */
    cube_gpio_af(GPIOC, GPIO_PIN_2, GPIO_AF11_ETH, GPIO_NOPULL);  /* TXD2 */
    cube_gpio_af(GPIOC, GPIO_PIN_3, GPIO_AF11_ETH, GPIO_NOPULL);  /* TX_CLK */
    cube_gpio_af(GPIOC, GPIO_PIN_4, GPIO_AF11_ETH, GPIO_NOPULL);  /* RXD0 */
    cube_gpio_af(GPIOC, GPIO_PIN_5, GPIO_AF11_ETH, GPIO_NOPULL);  /* RXD1 */
    cube_gpio_af(GPIOE, GPIO_PIN_2, GPIO_AF11_ETH, GPIO_NOPULL);  /* TXD3 */
    cube_gpio_af(GPIOG, GPIO_PIN_11, GPIO_AF11_ETH, GPIO_NOPULL); /* TX_EN */
    cube_gpio_af(GPIOG, GPIO_PIN_12, GPIO_AF11_ETH, GPIO_NOPULL); /* TXD1 */
    cube_gpio_af(GPIOG, GPIO_PIN_13, GPIO_AF11_ETH, GPIO_NOPULL); /* TXD0 */
}

static void mac_from_uid(uint8_t mac[6])
{
    uint32_t w0 = HAL_GetUIDw0();
    uint32_t w1 = HAL_GetUIDw1();

    mac[0] = 0x02u;
    mac[1] = (uint8_t)((w0 >> 16) & 0xFFu);
    mac[2] = (uint8_t)((w0 >> 8) & 0xFFu);
    mac[3] = (uint8_t)(w0 & 0xFFu);
    mac[4] = (uint8_t)((w1 >> 8) & 0xFFu);
    mac[5] = (uint8_t)(w1 & 0xFFu);
}

static void phy_decode(int32_t st)
{
    g_link = NET_LINK_DOWN;
    g_speed = 0u;
    g_duplex = 1u;
    if (st == LAN8742_STATUS_100MBITS_FULLDUPLEX) {
        g_link = NET_LINK_UP;
        g_speed = 100u;
        g_duplex = 1u;
    } else if (st == LAN8742_STATUS_100MBITS_HALFDUPLEX) {
        g_link = NET_LINK_UP;
        g_speed = 100u;
        g_duplex = 0u;
    } else if (st == LAN8742_STATUS_10MBITS_FULLDUPLEX) {
        g_link = NET_LINK_UP;
        g_speed = 10u;
        g_duplex = 1u;
    } else if (st == LAN8742_STATUS_10MBITS_HALFDUPLEX) {
        g_link = NET_LINK_UP;
        g_speed = 10u;
        g_duplex = 0u;
    }
}

static void phy_poll(uint32_t now_ms)
{
    int32_t st;

    if ((now_ms - g_last_phy_ms) < PHY_POLL_MS && g_last_phy_ms != 0u) {
        return;
    }
    g_last_phy_ms = now_ms;
    st = LAN8742_GetLinkState(&g_phy);
    phy_decode(st);
    ethernetif_set_link((g_link == NET_LINK_UP) ? 1 : 0, g_speed, g_duplex);
}

static err_t eth_bringup(void)
{
    lan8742_IOCtx_t io = {0};

    __HAL_RCC_D2SRAM3_CLK_ENABLE();
    mac_from_uid(g_mac);
    if (ethernetif_start(g_mac) != 0) {
        return ERR_IO;
    }
    io.ReadReg = phy_io_read;
    io.WriteReg = phy_io_write;
    io.GetTick = phy_io_tick;
    if (LAN8742_RegisterBusIO(&g_phy, &io) != LAN8742_STATUS_OK) {
        return ERR_IO;
    }
    if (LAN8742_Init(&g_phy) != LAN8742_STATUS_OK) {
        return ERR_IO;
    }
    (void)LAN8742_StartAutoNego(&g_phy);
    g_eth_ok = 1u;
    g_link = NET_LINK_DOWN;
    return ERR_OK;
}

err_t net_if_init(net_if_id_t id)
{
    if (id == NET_IF_WIFI) {
        return ERR_UNSUPPORTED;
    }
    if (id != NET_IF_ETH) {
        return ERR_INVAL;
    }
    return eth_bringup();
}

void net_if_poll(net_if_id_t id, uint32_t now_ms)
{
    if (id != NET_IF_ETH || g_eth_ok == 0u) {
        return;
    }
    phy_poll(now_ms);
    ethernetif_poll();
}

err_t net_if_status(net_if_id_t id, net_status_t *out)
{
    uint32_t ip = 0u;
    uint8_t dhcp = 1u;

    if (out == NULL) {
        return ERR_INVAL;
    }
    if (id == NET_IF_WIFI) {
        return ERR_UNSUPPORTED;
    }
    if (id != NET_IF_ETH || g_eth_ok == 0u) {
        return ERR_IO;
    }
    ethernetif_query(&ip, g_mac, &dhcp);
    out->link = g_link;
    out->ipv4 = ip;
    memcpy(out->mac, g_mac, 6u);
    out->speed_mbps = g_speed;
    out->duplex = g_duplex;
    out->dhcp = dhcp;
    return ERR_OK;
}

err_t net_if_set_dhcp(net_if_id_t id)
{
    if (id != NET_IF_ETH || g_eth_ok == 0u) {
        return ERR_INVAL;
    }
    return (ethernetif_set_dhcp() == 0) ? ERR_OK : ERR_IO;
}

err_t net_if_set_static(net_if_id_t id, uint32_t ipv4, uint32_t mask, uint32_t gw)
{
    if (id != NET_IF_ETH || g_eth_ok == 0u) {
        return ERR_INVAL;
    }
    return (ethernetif_set_static(ipv4, mask, gw) == 0) ? ERR_OK : ERR_IO;
}
