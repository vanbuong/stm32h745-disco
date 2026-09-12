#ifndef APP_CALENDAR_H
#define APP_CALENDAR_H

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

#define CALENDAR_CELLS 42u

void calendar_open(void);
void calendar_close(void);
void calendar_prev_month(void);
void calendar_next_month(void);
void calendar_go_today(void);

uint32_t calendar_gen(void);
const char *calendar_title(void);
const char *calendar_clock(void);
const char *calendar_date_line(void);
const char *calendar_ntp_line(void);
uint8_t calendar_cell_day(unsigned index);
uint8_t calendar_cell_today(unsigned index);

#ifdef __cplusplus
}
#endif

#endif /* APP_CALENDAR_H */
