#ifndef OSAL_H
#define OSAL_H

#include "err.h"

#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

#define OSAL_WAIT_FOREVER 0xFFFFFFFFu

typedef struct osal_mutex osal_mutex_t;
typedef struct osal_sem osal_sem_t;
typedef struct osal_queue osal_queue_t;
typedef struct osal_thread osal_thread_t;

typedef void (*osal_fn)(void *arg);

typedef struct {
    const char *name;
    uint32_t stack_bytes;
    uint8_t priority;
} osal_thread_attr_t;

err_t osal_mutex_create(osal_mutex_t **m);
err_t osal_mutex_lock(osal_mutex_t *m, uint32_t timeout_ms);
err_t osal_mutex_unlock(osal_mutex_t *m);
void osal_mutex_destroy(osal_mutex_t *m);

err_t osal_sem_create(osal_sem_t **s, uint32_t initial);
err_t osal_sem_take(osal_sem_t *s, uint32_t timeout_ms);
err_t osal_sem_give(osal_sem_t *s);
void osal_sem_destroy(osal_sem_t *s);

err_t osal_thread_create(osal_thread_t **t, const osal_thread_attr_t *a, osal_fn fn, void *arg);

void osal_sleep_ms(uint32_t ms);
uint32_t osal_millis(void);
void *osal_malloc(size_t n);
void osal_free(void *p);
void osal_panic(const char *why);

#ifdef __cplusplus
}
#endif

#endif /* OSAL_H */
