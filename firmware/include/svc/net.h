#ifndef NET_H
#define NET_H

#include "err.h"
#include "hal/net_if.h"

#ifdef __cplusplus
extern "C" {
#endif

err_t net_service_init(void);
err_t net_service_status(net_status_t *out);

#ifdef __cplusplus
}
#endif

#endif /* NET_H */
