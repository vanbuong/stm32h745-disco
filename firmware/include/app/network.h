#ifndef APP_NETWORK_H
#define APP_NETWORK_H

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

void network_refresh(void);
uint32_t network_gen(void);
const char *network_link_str(void);
const char *network_ip_str(void);
const char *network_mac_str(void);
const char *network_mode_str(void);
const char *network_path_str(void);
uint16_t network_speed_mbps(void);

#ifdef __cplusplus
}
#endif

#endif /* APP_NETWORK_H */
