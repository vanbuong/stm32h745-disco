#include "unity.h"

#include "app/network.h"
#include "svc/net.h"
#include "svc/time.h"

#include <string.h>

static void test_net_init_down(void)
{
    net_info_t inf;
    net_status_t st;

    TEST_ASSERT_EQUAL_UINT8(0u, net_bar_level());
    TEST_ASSERT_EQUAL_INT(ERR_IO, net_service_info(&inf));
    TEST_ASSERT_EQUAL_INT(ERR_INVAL, net_service_info(NULL));
    TEST_ASSERT_EQUAL_INT(ERR_OK, net_service_init());
    TEST_ASSERT_EQUAL_UINT8(1u, net_bar_level());
    TEST_ASSERT_EQUAL_INT(ERR_OK, net_service_info(&inf));
    TEST_ASSERT_EQUAL_UINT8(NET_PATH_ETH, inf.path);
    TEST_ASSERT_EQUAL_UINT8(NET_LINK_DOWN, inf.link);
    TEST_ASSERT_EQUAL_INT(ERR_INVAL, net_service_status(NULL));
    TEST_ASSERT_EQUAL_INT(ERR_OK, net_service_status(&st));
    TEST_ASSERT_EQUAL_UINT8(NET_LINK_DOWN, st.link);
    TEST_ASSERT_EQUAL_UINT32(0u, st.ipv4);
}

static void test_net_dhcp_lease(void)
{
    net_info_t inf;
    char ip[16];

    TEST_ASSERT_EQUAL_INT(ERR_OK, net_service_init());
    net_test_set_dhcp_delay_ms(100u);
    net_test_set_eth_link(NET_LINK_UP, 100u, 1u);
    net_service_poll(0u);
    TEST_ASSERT_EQUAL_INT(ERR_OK, net_service_info(&inf));
    TEST_ASSERT_EQUAL_UINT8(NET_LINK_UP, inf.link);
    TEST_ASSERT_EQUAL_UINT8(2u, inf.bar);
    TEST_ASSERT_EQUAL_UINT32(0u, inf.ipv4);
    net_service_poll(100u);
    TEST_ASSERT_EQUAL_INT(ERR_OK, net_service_info(&inf));
    TEST_ASSERT_EQUAL_UINT8(3u, inf.bar);
    TEST_ASSERT_EQUAL_UINT32(0xC0A80132u, inf.ipv4);
    TEST_ASSERT_EQUAL_UINT16(100u, inf.speed_mbps);
    TEST_ASSERT_EQUAL_UINT8(1u, inf.dhcp);
    net_ipv4_fmt(inf.ipv4, ip, sizeof(ip));
    TEST_ASSERT_EQUAL_STRING("192.168.1.50", ip);
}

static void test_net_unplug(void)
{
    net_info_t inf;

    TEST_ASSERT_EQUAL_INT(ERR_OK, net_service_init());
    net_test_set_dhcp_delay_ms(0u);
    net_test_set_eth_link(NET_LINK_UP, 100u, 1u);
    net_service_poll(0u);
    TEST_ASSERT_EQUAL_UINT8(3u, net_bar_level());
    net_test_set_eth_link(NET_LINK_DOWN, 0u, 1u);
    net_service_poll(200u);
    TEST_ASSERT_EQUAL_INT(ERR_OK, net_service_info(&inf));
    TEST_ASSERT_EQUAL_UINT8(1u, inf.bar);
    TEST_ASSERT_EQUAL_UINT8(NET_LINK_DOWN, inf.link);
    TEST_ASSERT_EQUAL_UINT32(0u, inf.ipv4);
}

static void test_net_static(void)
{
    net_info_t inf;

    TEST_ASSERT_EQUAL_INT(ERR_OK, net_service_init());
    TEST_ASSERT_EQUAL_INT(ERR_OK, net_set_static(0x0A00000Au, 0xFFFFFF00u, 0x0A000001u));
    net_test_set_eth_link(NET_LINK_UP, 100u, 1u);
    net_service_poll(0u);
    TEST_ASSERT_EQUAL_INT(ERR_OK, net_service_info(&inf));
    TEST_ASSERT_EQUAL_UINT8(0u, inf.dhcp);
    TEST_ASSERT_EQUAL_UINT32(0x0A00000Au, inf.ipv4);
    TEST_ASSERT_EQUAL_UINT8(3u, inf.bar);
    TEST_ASSERT_EQUAL_INT(ERR_OK, net_set_dhcp());
    net_test_set_dhcp_delay_ms(0u);
    net_service_poll(0u);
    TEST_ASSERT_EQUAL_INT(ERR_OK, net_service_info(&inf));
    TEST_ASSERT_EQUAL_UINT8(1u, inf.dhcp);
    TEST_ASSERT_EQUAL_UINT32(0xC0A80132u, inf.ipv4);
}

static void test_net_failover(void)
{
    net_info_t inf;

    TEST_ASSERT_EQUAL_INT(ERR_OK, net_service_init());
    net_test_set_dhcp_delay_ms(0u);
    net_test_set_wifi_link(NET_LINK_UP);
    net_test_set_eth_link(NET_LINK_DOWN, 0u, 1u);
    net_service_poll(0u);
    TEST_ASSERT_EQUAL_INT(ERR_OK, net_service_info(&inf));
    TEST_ASSERT_EQUAL_UINT8(NET_PATH_ETH, inf.path);
    TEST_ASSERT_EQUAL_UINT8(1u, inf.bar);
    net_service_poll(NET_FAILOVER_MS - 1u);
    TEST_ASSERT_EQUAL_INT(ERR_OK, net_service_info(&inf));
    TEST_ASSERT_EQUAL_UINT8(NET_PATH_ETH, inf.path);
    net_service_poll(NET_FAILOVER_MS);
    TEST_ASSERT_EQUAL_INT(ERR_OK, net_service_info(&inf));
    TEST_ASSERT_EQUAL_UINT8(NET_PATH_WIFI, inf.path);
    TEST_ASSERT_EQUAL_UINT8(3u, inf.bar);
    TEST_ASSERT_EQUAL_UINT32(0xC0A80402u, inf.ipv4);
    net_test_set_eth_link(NET_LINK_UP, 100u, 1u);
    net_service_poll(NET_FAILOVER_MS);
    TEST_ASSERT_EQUAL_INT(ERR_OK, net_service_info(&inf));
    TEST_ASSERT_EQUAL_UINT8(NET_PATH_ETH, inf.path);
}

static void test_net_failover_wifi_down(void)
{
    net_info_t inf;

    TEST_ASSERT_EQUAL_INT(ERR_OK, net_service_init());
    net_test_set_wifi_link(NET_LINK_DOWN);
    net_test_set_eth_link(NET_LINK_DOWN, 0u, 1u);
    net_service_poll(NET_FAILOVER_MS);
    TEST_ASSERT_EQUAL_INT(ERR_OK, net_service_info(&inf));
    TEST_ASSERT_EQUAL_UINT8(NET_PATH_ETH, inf.path);
    TEST_ASSERT_EQUAL_UINT8(1u, inf.bar);
}

static void test_net_fmt(void)
{
    char ip[16];
    char mac[20];
    uint8_t addr[6] = {0x02u, 0x00u, 0x0Au, 0x0Bu, 0x0Cu, 0x0Du};

    net_ipv4_fmt(0u, ip, sizeof(ip));
    TEST_ASSERT_EQUAL_STRING("0.0.0.0", ip);
    net_ipv4_fmt(0xC0A80101u, ip, sizeof(ip));
    TEST_ASSERT_EQUAL_STRING("192.168.1.1", ip);
    net_ipv4_fmt(0xFFFFFFFFu, ip, sizeof(ip));
    TEST_ASSERT_EQUAL_STRING("255.255.255.255", ip);
    net_ipv4_fmt(1u, NULL, 8u);
    net_mac_fmt(addr, mac, sizeof(mac));
    TEST_ASSERT_EQUAL_STRING("02:00:0a:0b:0c:0d", mac);
    net_mac_fmt(NULL, mac, sizeof(mac));
    TEST_ASSERT_EQUAL_STRING("", mac);
}

static void test_net_screen(void)
{
    TEST_ASSERT_EQUAL_INT(ERR_OK, net_service_init());
    net_test_set_dhcp_delay_ms(0u);
    net_test_set_eth_link(NET_LINK_UP, 100u, 1u);
    net_service_poll(0u);
    network_refresh();
    TEST_ASSERT_EQUAL_STRING("eth", network_path_str());
    TEST_ASSERT_EQUAL_STRING("up full", network_link_str());
    TEST_ASSERT_EQUAL_STRING("192.168.1.50", network_ip_str());
    TEST_ASSERT_EQUAL_STRING("dhcp", network_mode_str());
    TEST_ASSERT_EQUAL_UINT16(100u, network_speed_mbps());
    TEST_ASSERT_GREATER_THAN_UINT32(0u, network_gen());
    network_refresh();
    /* unchanged snapshot does not bump gen */
}

static void test_time_clock(void)
{
    uint8_t hh;
    uint8_t mm;
    uint8_t ss;

    TEST_ASSERT_EQUAL_INT(ERR_OK, time_init());
    TEST_ASSERT_EQUAL_INT(ERR_INVAL, time_rtc_get(NULL, &mm, &ss));
    TEST_ASSERT_EQUAL_INT(ERR_OK, time_rtc_get(&hh, &mm, &ss));
    TEST_ASSERT_EQUAL_UINT8(0u, hh);
    TEST_ASSERT_EQUAL_UINT8(0u, mm);
    time_poll(60000u);
    TEST_ASSERT_EQUAL_INT(ERR_OK, time_rtc_get(&hh, &mm, &ss));
    TEST_ASSERT_EQUAL_UINT8(0u, hh);
    TEST_ASSERT_EQUAL_UINT8(1u, mm);
    TEST_ASSERT_EQUAL_INT(ERR_OK, time_rtc_set(1u, 2u, 3u));
    TEST_ASSERT_EQUAL_INT(ERR_OK, time_rtc_get(&hh, &mm, &ss));
    TEST_ASSERT_EQUAL_UINT8(1u, hh);
    TEST_ASSERT_EQUAL_UINT8(2u, mm);
    TEST_ASSERT_EQUAL_UINT8(3u, ss);
    TEST_ASSERT_EQUAL_INT(ERR_INVAL, time_rtc_set(24u, 0u, 0u));
    time_poll(60u * 60000u);
    TEST_ASSERT_EQUAL_INT(ERR_OK, time_rtc_get(&hh, &mm, &ss));
    TEST_ASSERT_EQUAL_UINT8(2u, hh);
}

void test_net_run(void)
{
    UnitySetTestFile(__FILE__);
    RUN_TEST(test_net_init_down);
    RUN_TEST(test_net_dhcp_lease);
    RUN_TEST(test_net_unplug);
    RUN_TEST(test_net_static);
    RUN_TEST(test_net_failover);
    RUN_TEST(test_net_failover_wifi_down);
    RUN_TEST(test_net_fmt);
    RUN_TEST(test_net_screen);
    RUN_TEST(test_time_clock);
}
