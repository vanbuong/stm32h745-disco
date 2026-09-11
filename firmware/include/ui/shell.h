#ifndef SHELL_H
#define SHELL_H

#include "err.h"

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct {
    uint8_t hour;
    uint8_t min;
    uint8_t storage_ok;
    uint8_t net; /* 0 hidden, 1 err, 2 warn, 3 ok */
    uint8_t zb;
} shell_status_t;

void shell_init(void);
err_t shell_push(const char *app_id, void *args);
err_t shell_pop(void);
void shell_home(void);
void shell_tick(uint32_t dt_ms);

const char *shell_top_id(void);
const char *shell_top_title(void);
int shell_depth(void);
uint32_t shell_nav_gen(void);
const shell_status_t *shell_status(void);
void shell_status_set_storage(uint8_t ok);

#ifdef __cplusplus
}
#endif

#endif /* SHELL_H */
