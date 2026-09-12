#ifndef WDOG_H
#define WDOG_H

#include "err.h"

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

err_t wdog_start(void);
void wdog_kick(void);
uint8_t wdog_started(void);

#ifdef __cplusplus
}
#endif

#endif /* WDOG_H */
