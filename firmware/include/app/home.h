#ifndef APP_HOME_H
#define APP_HOME_H

#include "svc/home.h"

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef enum { HOME_PAGE_LIST = 0, HOME_PAGE_DEVICE, HOME_PAGE_NETWORK } home_page_t;

void home_app_open(void);
void home_app_close(void);
void home_app_tick(uint32_t dt_ms);
uint8_t home_app_on_back(void);
void home_app_pair(void);
void home_app_open_device(unsigned index);
void home_app_open_network(void);
void home_app_toggle(unsigned index);

home_page_t home_app_page(void);
unsigned home_app_sel(void);
uint32_t home_app_gen(void);
const char *home_app_banner(void);
const char *home_app_title(void);
const char *home_app_state_text(const home_device_t *d);
const char *home_app_kind_text(home_kind_t kind);

#ifdef __cplusplus
}
#endif

#endif /* APP_HOME_H */
