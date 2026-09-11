#include "osal/osal.h"

#include <pthread.h>
#include <semaphore.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <unistd.h>

struct osal_mutex {
    pthread_mutex_t m;
};

struct osal_sem {
    sem_t s;
};

err_t osal_mutex_create(osal_mutex_t **m)
{
    osal_mutex_t *p;
    if (m == NULL) {
        return ERR_INVAL;
    }
    p = (osal_mutex_t *)malloc(sizeof(*p));
    if (p == NULL) {
        return ERR_NOMEM;
    }
    if (pthread_mutex_init(&p->m, NULL) != 0) {
        free(p);
        return ERR_IO;
    }
    *m = p;
    return ERR_OK;
}

err_t osal_mutex_lock(osal_mutex_t *m, uint32_t timeout_ms)
{
    if (m == NULL) {
        return ERR_INVAL;
    }
    (void)timeout_ms;
    if (pthread_mutex_lock(&m->m) != 0) {
        return ERR_IO;
    }
    return ERR_OK;
}

err_t osal_mutex_unlock(osal_mutex_t *m)
{
    if (m == NULL) {
        return ERR_INVAL;
    }
    if (pthread_mutex_unlock(&m->m) != 0) {
        return ERR_IO;
    }
    return ERR_OK;
}

void osal_mutex_destroy(osal_mutex_t *m)
{
    if (m == NULL) {
        return;
    }
    pthread_mutex_destroy(&m->m);
    free(m);
}

err_t osal_sem_create(osal_sem_t **s, uint32_t initial)
{
    osal_sem_t *p;
    if (s == NULL) {
        return ERR_INVAL;
    }
    p = (osal_sem_t *)malloc(sizeof(*p));
    if (p == NULL) {
        return ERR_NOMEM;
    }
    if (sem_init(&p->s, 0, initial) != 0) {
        free(p);
        return ERR_IO;
    }
    *s = p;
    return ERR_OK;
}

err_t osal_sem_take(osal_sem_t *s, uint32_t timeout_ms)
{
    if (s == NULL) {
        return ERR_INVAL;
    }
    (void)timeout_ms;
    if (sem_wait(&s->s) != 0) {
        return ERR_IO;
    }
    return ERR_OK;
}

err_t osal_sem_give(osal_sem_t *s)
{
    if (s == NULL) {
        return ERR_INVAL;
    }
    if (sem_post(&s->s) != 0) {
        return ERR_IO;
    }
    return ERR_OK;
}

void osal_sem_destroy(osal_sem_t *s)
{
    if (s == NULL) {
        return;
    }
    sem_destroy(&s->s);
    free(s);
}

err_t osal_thread_create(osal_thread_t **t, const osal_thread_attr_t *a, osal_fn fn, void *arg)
{
    (void)t;
    (void)a;
    (void)fn;
    (void)arg;
    return ERR_UNSUPPORTED;
}

void osal_sleep_ms(uint32_t ms)
{
    usleep(ms * 1000u);
}

uint32_t osal_millis(void)
{
    struct timespec ts;
    if (clock_gettime(CLOCK_MONOTONIC, &ts) != 0) {
        return 0;
    }
    return (uint32_t)((uint64_t)ts.tv_sec * 1000u + (uint64_t)ts.tv_nsec / 1000000u);
}

void *osal_malloc(size_t n)
{
    return malloc(n);
}

void osal_free(void *p)
{
    free(p);
}

void osal_panic(const char *why)
{
    fprintf(stderr, "osal_panic: %s\n", why != NULL ? why : "");
    abort();
}
