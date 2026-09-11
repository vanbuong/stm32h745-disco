#ifndef ETHERNETIF_H
#define ETHERNETIF_H

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

int ethernetif_start(const uint8_t mac[6]);
void ethernetif_poll(void);
void ethernetif_set_link(int up, uint16_t speed_mbps, uint8_t duplex);
void ethernetif_query(uint32_t *ipv4_host, uint8_t mac[6], uint8_t *dhcp);
int ethernetif_set_dhcp(void);
int ethernetif_set_static(uint32_t ipv4_host, uint32_t mask_host, uint32_t gw_host);
int ethernetif_phy_read(uint32_t addr, uint32_t reg, uint32_t *val);
int ethernetif_phy_write(uint32_t addr, uint32_t reg, uint32_t val);

#ifdef __cplusplus
}
#endif

#endif /* ETHERNETIF_H */
