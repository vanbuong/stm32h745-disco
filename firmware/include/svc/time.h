#ifndef TIME_SVC_H
#define TIME_SVC_H

#include "err.h"

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct {
    uint16_t year;
    uint8_t month;
    uint8_t day;
    uint8_t hour;
    uint8_t min;
    uint8_t sec;
    uint8_t wday;
} time_civil_t;

typedef enum {
    TIME_NTP_IDLE = 0,
    TIME_NTP_WAIT = 1,
    TIME_NTP_OK = 2,
    TIME_NTP_ERR = 3
} time_ntp_st_t;

err_t time_init(void);
void time_poll(uint32_t dt_ms);
err_t time_rtc_get(uint8_t *hh, uint8_t *mm, uint8_t *ss);
err_t time_rtc_set(uint8_t hh, uint8_t mm, uint8_t ss);

err_t time_now(time_civil_t *out);
err_t time_set(const time_civil_t *in);
uint8_t time_month_days(uint16_t year, uint8_t month);
uint8_t time_weekday(uint16_t year, uint8_t month, uint8_t day);
void time_unix_to_civil(uint32_t unix_sec, time_civil_t *out);
void time_ntp_apply_unix(uint32_t unix_sec);
time_ntp_st_t time_ntp_state(void);
const char *time_ntp_str(void);

#ifdef __cplusplus
}
#endif

#endif /* TIME_SVC_H */
