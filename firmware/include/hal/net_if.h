#ifndef NET_IF_H
#define NET_IF_H

#include "err.h"

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef enum { NET_LINK_DOWN = 0, NET_LINK_UP = 1 } net_link_t;

typedef enum { NET_IF_ETH = 0, NET_IF_WIFI = 1 } net_if_id_t;

typedef struct {
    net_link_t link;
    uint32_t ipv4; /* host byte order */
    uint8_t mac[6];
    uint16_t speed_mbps;
    uint8_t duplex; /* 0 half, 1 full */
    uint8_t dhcp;   /* 1 DHCP, 0 static */
} net_status_t;

err_t net_if_init(net_if_id_t id);
void net_if_poll(net_if_id_t id, uint32_t now_ms);
err_t net_if_status(net_if_id_t id, net_status_t *out);
err_t net_if_set_dhcp(net_if_id_t id);
err_t net_if_set_static(net_if_id_t id, uint32_t ipv4, uint32_t mask, uint32_t gw);

err_t net_init(void);
err_t net_status(net_status_t *out);

#ifdef __cplusplus
}
#endif

#endif /* NET_IF_H */
