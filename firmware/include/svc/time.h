#ifndef TIME_SVC_H
#define TIME_SVC_H

#include "err.h"

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

err_t time_init(void);
void time_poll(uint32_t dt_ms);
err_t time_rtc_get(uint8_t *hh, uint8_t *mm, uint8_t *ss);
err_t time_rtc_set(uint8_t hh, uint8_t mm, uint8_t ss);

#ifdef __cplusplus
}
#endif

#endif /* TIME_SVC_H */
