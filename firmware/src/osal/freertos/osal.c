#include "osal/osal.h"

#include "FreeRTOS.h"
#include "semphr.h"
#include "task.h"

#include "bsp/board.h"

#include <stdlib.h>

struct osal_mutex {
    SemaphoreHandle_t m;
};

struct osal_sem {
    SemaphoreHandle_t s;
};

struct osal_thread {
    TaskHandle_t t;
};

static TickType_t ticks_of(uint32_t timeout_ms)
{
    if (timeout_ms == OSAL_WAIT_FOREVER) {
        return portMAX_DELAY;
    }
    return pdMS_TO_TICKS(timeout_ms);
}

uint8_t osal_scheduler_running(void)
{
    return (xTaskGetSchedulerState() == taskSCHEDULER_RUNNING) ? 1u : 0u;
}

err_t osal_mutex_create(osal_mutex_t **m)
{
    osal_mutex_t *p;

    if (m == NULL) {
        return ERR_INVAL;
    }
    /* xSemaphoreCreateMutex() takes a critical section. On Cortex-M that
     * raises BASEPRI; uxCriticalNesting is 0xaaaaaaaa until the scheduler
     * starts, so EXIT never unmasks SysTick and HAL_Delay hangs. */
    if (xTaskGetSchedulerState() != taskSCHEDULER_RUNNING) {
        return ERR_BUSY;
    }
    p = (osal_mutex_t *)pvPortMalloc(sizeof(*p));
    if (p == NULL) {
        return ERR_NOMEM;
    }
    p->m = xSemaphoreCreateMutex();
    if (p->m == NULL) {
        vPortFree(p);
        return ERR_NOMEM;
    }
    *m = p;
    return ERR_OK;
}

err_t osal_mutex_ensure(osal_mutex_t **m)
{
    osal_mutex_t *p;
    err_t e;

    if (m == NULL) {
        return ERR_INVAL;
    }
    if (*m != NULL) {
        return ERR_OK;
    }
    if (xTaskGetSchedulerState() != taskSCHEDULER_RUNNING) {
        return ERR_OK;
    }
    e = osal_mutex_create(&p);
    if (e != ERR_OK) {
        return e;
    }
    vTaskSuspendAll();
    if (*m != NULL) {
        (void)xTaskResumeAll();
        osal_mutex_destroy(p);
        return ERR_OK;
    }
    *m = p;
    (void)xTaskResumeAll();
    return ERR_OK;
}

err_t osal_mutex_lock(osal_mutex_t *m, uint32_t timeout_ms)
{
    if (m == NULL) {
        return ERR_INVAL;
    }
    return (xSemaphoreTake(m->m, ticks_of(timeout_ms)) == pdTRUE) ? ERR_OK : ERR_TIMEOUT;
}

err_t osal_mutex_unlock(osal_mutex_t *m)
{
    if (m == NULL) {
        return ERR_INVAL;
    }
    return (xSemaphoreGive(m->m) == pdTRUE) ? ERR_OK : ERR_IO;
}

void osal_mutex_destroy(osal_mutex_t *m)
{
    if (m == NULL) {
        return;
    }
    vSemaphoreDelete(m->m);
    vPortFree(m);
}

err_t osal_sem_create(osal_sem_t **s, uint32_t initial)
{
    osal_sem_t *p;

    if (s == NULL) {
        return ERR_INVAL;
    }
    p = (osal_sem_t *)pvPortMalloc(sizeof(*p));
    if (p == NULL) {
        return ERR_NOMEM;
    }
    p->s = xSemaphoreCreateCounting(0xFFFFu, initial);
    if (p->s == NULL) {
        vPortFree(p);
        return ERR_NOMEM;
    }
    *s = p;
    return ERR_OK;
}

err_t osal_sem_take(osal_sem_t *s, uint32_t timeout_ms)
{
    if (s == NULL) {
        return ERR_INVAL;
    }
    return (xSemaphoreTake(s->s, ticks_of(timeout_ms)) == pdTRUE) ? ERR_OK : ERR_TIMEOUT;
}

err_t osal_sem_give(osal_sem_t *s)
{
    if (s == NULL) {
        return ERR_INVAL;
    }
    return (xSemaphoreGive(s->s) == pdTRUE) ? ERR_OK : ERR_IO;
}

void osal_sem_destroy(osal_sem_t *s)
{
    if (s == NULL) {
        return;
    }
    vSemaphoreDelete(s->s);
    vPortFree(s);
}

err_t osal_thread_create(osal_thread_t **t, const osal_thread_attr_t *a, osal_fn fn, void *arg)
{
    osal_thread_t *p;
    uint32_t words;
    uint32_t prio;

    if (t == NULL || fn == NULL) {
        return ERR_INVAL;
    }
    p = (osal_thread_t *)pvPortMalloc(sizeof(*p));
    if (p == NULL) {
        return ERR_NOMEM;
    }
    words = (a != NULL && a->stack_bytes != 0u) ? (a->stack_bytes / sizeof(StackType_t))
                                                : (uint32_t)configMINIMAL_STACK_SIZE;
    if (words < (uint32_t)configMINIMAL_STACK_SIZE) {
        words = (uint32_t)configMINIMAL_STACK_SIZE;
    }
    prio = (a != NULL) ? (uint32_t)a->priority : 1u;
    if (prio >= (uint32_t)configMAX_PRIORITIES) {
        prio = (uint32_t)configMAX_PRIORITIES - 1u;
    }
    if (xTaskCreate(fn, (a != NULL && a->name != NULL) ? a->name : "app", (uint16_t)words, arg,
                    (UBaseType_t)prio, &p->t) != pdPASS) {
        vPortFree(p);
        return ERR_NOMEM;
    }
    *t = p;
    return ERR_OK;
}

void osal_sleep_ms(uint32_t ms)
{
    if (xTaskGetSchedulerState() != taskSCHEDULER_RUNNING) {
        uint32_t start = board_millis();

        while ((uint32_t)(board_millis() - start) < ms) {
        }
        return;
    }
    vTaskDelay(pdMS_TO_TICKS((ms == 0u) ? 1u : ms));
}

uint32_t osal_millis(void)
{
    return board_millis();
}

void *osal_malloc(size_t n)
{
    return pvPortMalloc(n);
}

void osal_free(void *p)
{
    vPortFree(p);
}

void osal_panic(const char *why)
{
    board_console_puts("osal_panic ");
    board_console_puts(why != NULL ? why : "");
    board_console_puts("\r\n");
    taskDISABLE_INTERRUPTS();
    for (;;) {
    }
}
