#ifndef APP_HOME_H
#define APP_HOME_H

#include "svc/home.h"

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef enum {
    HOME_PAGE_LIST = 0,
    HOME_PAGE_DEVICE,
    HOME_PAGE_NETWORK,
    HOME_PAGE_AUTOS,
    HOME_PAGE_RULE
} home_page_t;

void home_app_open(void);
void home_app_close(void);
void home_app_tick(uint32_t dt_ms);
uint8_t home_app_on_back(void);
void home_app_pair(void);
void home_app_form(void);
void home_app_open_device(unsigned index);
void home_app_open_network(void);
void home_app_open_autos(void);
void home_app_open_rule(unsigned index);
void home_app_toggle(unsigned index);
void home_app_toggle_rule(unsigned index);
void home_app_add_rule(void);
void home_app_delete_rule(void);
void home_app_remove_device(void);

home_page_t home_app_page(void);
unsigned home_app_sel(void);
uint32_t home_app_gen(void);
const char *home_app_banner(void);
const char *home_app_title(void);
const char *home_app_state_text(const home_device_t *d);
const char *home_app_kind_text(home_kind_t kind);
const char *home_app_last_seen(const home_device_t *d);
const char *home_app_clusters(home_kind_t kind);

unsigned home_app_rule_count(void);
uint16_t home_app_rule_id(unsigned index);
uint8_t home_app_rule_enabled(unsigned index);
const char *home_app_rule_name(unsigned index);
const char *home_app_rule_summary(unsigned index);
const char *home_app_rule_delay(unsigned index);

#ifdef __cplusplus
}
#endif

#endif /* APP_HOME_H */
