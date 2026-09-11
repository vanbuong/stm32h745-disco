#ifndef NAV_H
#define NAV_H

#include "err.h"

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

#define NAV_STACK_MAX 8

typedef struct {
    const char *id;
    void *args;
} nav_frame_t;

typedef struct {
    nav_frame_t frames[NAV_STACK_MAX];
    uint8_t depth;
    uint32_t gen;
} nav_stack_t;

void nav_init(nav_stack_t *s);
err_t nav_push(nav_stack_t *s, const char *id, void *args);
err_t nav_pop(nav_stack_t *s);
void nav_home(nav_stack_t *s);
const nav_frame_t *nav_top(const nav_stack_t *s);

#ifdef __cplusplus
}
#endif

#endif /* NAV_H */
