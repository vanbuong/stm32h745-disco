#ifndef NET_IF_H
#define NET_IF_H

#include "err.h"

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef enum { NET_LINK_DOWN = 0, NET_LINK_UP = 1 } net_link_t;

typedef struct {
    net_link_t link;
    uint32_t ipv4;
    uint8_t mac[6];
} net_status_t;

err_t net_init(void);
err_t net_status(net_status_t *out);

#ifdef __cplusplus
}
#endif

#endif /* NET_IF_H */
