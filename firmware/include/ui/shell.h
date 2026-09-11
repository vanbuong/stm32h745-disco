#ifndef SHELL_H
#define SHELL_H

#include "err.h"

#ifdef __cplusplus
extern "C" {
#endif

err_t shell_push(const char *app_id, void *args);
err_t shell_pop(void);
void shell_tick(uint32_t dt_ms);

#ifdef __cplusplus
}
#endif

#endif /* SHELL_H */
