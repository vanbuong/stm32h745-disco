#ifndef NET_H
#define NET_H

#include "err.h"
#include "hal/net_if.h"

#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

#define NET_FAILOVER_MS 10000u

typedef enum { NET_PATH_NONE = 0, NET_PATH_ETH = 1, NET_PATH_WIFI = 2 } net_path_t;

typedef struct {
    net_link_t link;
    uint32_t ipv4;
    uint8_t mac[6];
    uint16_t speed_mbps;
    uint8_t duplex;
    uint8_t dhcp;
    net_path_t path;
    uint8_t bar; /* 0 hidden, 1 err, 2 warn, 3 ok */
} net_info_t;

err_t net_service_init(void);
void net_service_poll(uint32_t now_ms);
err_t net_service_status(net_status_t *out);
err_t net_service_info(net_info_t *out);
uint8_t net_bar_level(void);
void net_ipv4_fmt(uint32_t ipv4, char *out, size_t n);
void net_mac_fmt(const uint8_t mac[6], char *out, size_t n);
err_t net_set_dhcp(void);
err_t net_set_static(uint32_t ipv4, uint32_t mask, uint32_t gw);

#if !defined(CORE_CM7)
void net_test_set_eth_link(net_link_t link, uint16_t speed_mbps, uint8_t duplex);
void net_test_set_wifi_link(net_link_t link);
void net_test_set_dhcp_delay_ms(uint32_t ms);
#endif

#ifdef __cplusplus
}
#endif

#endif /* NET_H */
