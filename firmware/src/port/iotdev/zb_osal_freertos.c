#include "zb_osal.h"

#include "zb_port.h"

#include "bsp/board.h"

#include "FreeRTOS.h"
#include "event_groups.h"
#include "queue.h"
#include "semphr.h"
#include "stream_buffer.h"
#include "task.h"
#include "timers.h"

#include <stdlib.h>

static inline TickType_t zb_osal_ticks(uint32_t timeout_ms)
{
    if (timeout_ms == ZB_OSAL_WAIT_FOREVER) {
        return portMAX_DELAY;
    }
    return pdMS_TO_TICKS(timeout_ms);
}

void zb_os_set_idle_pump(void (*fn)(void))
{
    (void)fn;
}

void zb_os_set_yield_pump(void (*fn)(void))
{
    (void)fn;
}

void zb_os_timer_pump(void)
{
}

uint32_t zb_os_now_ms(void)
{
    return board_millis();
}

void zb_os_delay_ms(uint32_t ms)
{
    if (xTaskGetSchedulerState() != taskSCHEDULER_RUNNING) {
        uint32_t start = board_millis();

        while ((uint32_t)(board_millis() - start) < ms) {
        }
        return;
    }
    vTaskDelay(pdMS_TO_TICKS((ms == 0u) ? 1u : ms));
}

bool zb_os_task_create(zb_os_task_fn_t fn, const char *name, size_t stack_bytes, void *arg,
                       uint32_t priority, zb_os_task_t *out_task)
{
    TaskHandle_t handle = NULL;
    uint32_t words;

    words = (uint32_t)(stack_bytes / sizeof(StackType_t));
    if (words < (uint32_t)configMINIMAL_STACK_SIZE) {
        words = (uint32_t)configMINIMAL_STACK_SIZE;
    }
    if (priority >= (uint32_t)configMAX_PRIORITIES) {
        priority = (uint32_t)configMAX_PRIORITIES - 1u;
    }
    if (xTaskCreate(fn, (name != NULL) ? name : "zb", (uint16_t)words, arg, (UBaseType_t)priority,
                    &handle) != pdPASS) {
        return false;
    }
    if (out_task != NULL) {
        *out_task = (zb_os_task_t)handle;
    }
    return true;
}

void zb_os_task_delete(zb_os_task_t task)
{
    vTaskDelete((TaskHandle_t)task);
}

zb_os_queue_t zb_os_queue_create(size_t depth, size_t item_size)
{
    return (zb_os_queue_t)xQueueCreate((UBaseType_t)depth, (UBaseType_t)item_size);
}

void zb_os_queue_delete(zb_os_queue_t q)
{
    if (q != NULL) {
        vQueueDelete((QueueHandle_t)q);
    }
}

bool zb_os_queue_send(zb_os_queue_t q, const void *item, uint32_t timeout_ms)
{
    return xQueueSendToBack((QueueHandle_t)q, item, zb_osal_ticks(timeout_ms)) == pdTRUE;
}

bool zb_os_queue_recv(zb_os_queue_t q, void *out, uint32_t timeout_ms)
{
    return xQueueReceive((QueueHandle_t)q, out, zb_osal_ticks(timeout_ms)) == pdTRUE;
}

bool zb_os_queue_peek(zb_os_queue_t q, void *out, uint32_t timeout_ms)
{
    return xQueuePeek((QueueHandle_t)q, out, zb_osal_ticks(timeout_ms)) == pdTRUE;
}

void zb_os_queue_reset(zb_os_queue_t q)
{
    (void)xQueueReset((QueueHandle_t)q);
}

struct zb_os_mutex {
    SemaphoreHandle_t sem;
    bool recursive;
};

zb_os_mutex_t zb_os_mutex_create(bool recursive)
{
    struct zb_os_mutex *m = (struct zb_os_mutex *)malloc(sizeof(*m));

    if (m == NULL) {
        return NULL;
    }
    m->recursive = recursive;
    m->sem = recursive ? xSemaphoreCreateRecursiveMutex() : xSemaphoreCreateMutex();
    if (m->sem == NULL) {
        free(m);
        return NULL;
    }
    return m;
}

void zb_os_mutex_delete(zb_os_mutex_t handle)
{
    struct zb_os_mutex *m = (struct zb_os_mutex *)handle;

    if (m != NULL) {
        vSemaphoreDelete(m->sem);
        free(m);
    }
}

bool zb_os_mutex_lock(zb_os_mutex_t handle, uint32_t timeout_ms)
{
    struct zb_os_mutex *m = (struct zb_os_mutex *)handle;

    if (m == NULL) {
        return false;
    }
    if (m->recursive) {
        return xSemaphoreTakeRecursive(m->sem, zb_osal_ticks(timeout_ms)) == pdTRUE;
    }
    return xSemaphoreTake(m->sem, zb_osal_ticks(timeout_ms)) == pdTRUE;
}

void zb_os_mutex_unlock(zb_os_mutex_t handle)
{
    struct zb_os_mutex *m = (struct zb_os_mutex *)handle;

    if (m == NULL) {
        return;
    }
    if (m->recursive) {
        (void)xSemaphoreGiveRecursive(m->sem);
    } else {
        (void)xSemaphoreGive(m->sem);
    }
}

zb_os_signal_t zb_os_signal_create(void)
{
    return (zb_os_signal_t)xQueueCreate(1, sizeof(uint32_t));
}

void zb_os_signal_delete(zb_os_signal_t s)
{
    if (s != NULL) {
        vQueueDelete((QueueHandle_t)s);
    }
}

void zb_os_signal_set(zb_os_signal_t s, uint32_t value)
{
    (void)xQueueOverwrite((QueueHandle_t)s, &value);
}

bool zb_os_signal_wait(zb_os_signal_t s, uint32_t *out_value, uint32_t timeout_ms)
{
    uint32_t value = 0u;

    if (xQueueReceive((QueueHandle_t)s, &value, zb_osal_ticks(timeout_ms)) != pdTRUE) {
        return false;
    }
    if (out_value != NULL) {
        *out_value = value;
    }
    return true;
}

void zb_os_signal_clear(zb_os_signal_t s)
{
    uint32_t scratch = 0u;

    (void)xQueueReceive((QueueHandle_t)s, &scratch, 0);
}

zb_os_event_t zb_os_event_create(void)
{
    return (zb_os_event_t)xEventGroupCreate();
}

void zb_os_event_delete(zb_os_event_t e)
{
    if (e != NULL) {
        vEventGroupDelete((EventGroupHandle_t)e);
    }
}

void zb_os_event_set(zb_os_event_t e, uint32_t bits)
{
    (void)xEventGroupSetBits((EventGroupHandle_t)e, (EventBits_t)bits);
}

uint32_t zb_os_event_wait(zb_os_event_t e, uint32_t bits, bool wait_all, bool clear_on_exit,
                          uint32_t timeout_ms)
{
    EventBits_t set;

    set = xEventGroupWaitBits((EventGroupHandle_t)e, (EventBits_t)bits,
                              clear_on_exit ? pdTRUE : pdFALSE, wait_all ? pdTRUE : pdFALSE,
                              zb_osal_ticks(timeout_ms));
    return (uint32_t)set;
}

struct zb_os_timer_ctx {
    zb_os_timer_fn_t cb;
    void *arg;
};

static void zb_os_timer_trampoline(TimerHandle_t timer)
{
    struct zb_os_timer_ctx *ctx = (struct zb_os_timer_ctx *)pvTimerGetTimerID(timer);

    if (ctx != NULL && ctx->cb != NULL) {
        ctx->cb((zb_os_timer_t)timer, ctx->arg);
    }
}

zb_os_timer_t zb_os_timer_create(const char *name, uint32_t period_ms, bool auto_reload,
                                 zb_os_timer_fn_t cb, void *arg)
{
    struct zb_os_timer_ctx *ctx = (struct zb_os_timer_ctx *)malloc(sizeof(*ctx));
    TimerHandle_t t;

    if (ctx == NULL) {
        return NULL;
    }
    ctx->cb = cb;
    ctx->arg = arg;
    t = xTimerCreate((name != NULL) ? name : "zb",
                     pdMS_TO_TICKS((period_ms == 0u) ? 1u : period_ms),
                     auto_reload ? pdTRUE : pdFALSE, ctx, zb_os_timer_trampoline);
    if (t == NULL) {
        free(ctx);
        return NULL;
    }
    return (zb_os_timer_t)t;
}

void zb_os_timer_delete(zb_os_timer_t handle)
{
    TimerHandle_t t = (TimerHandle_t)handle;
    struct zb_os_timer_ctx *ctx;

    if (t == NULL) {
        return;
    }
    ctx = (struct zb_os_timer_ctx *)pvTimerGetTimerID(t);
    if (xTimerDelete(t, portMAX_DELAY) == pdPASS) {
        free(ctx);
    }
}

bool zb_os_timer_start(zb_os_timer_t handle)
{
    return xTimerStart((TimerHandle_t)handle, portMAX_DELAY) == pdPASS;
}

bool zb_os_timer_stop(zb_os_timer_t handle)
{
    return xTimerStop((TimerHandle_t)handle, portMAX_DELAY) == pdPASS;
}

bool zb_os_timer_change_period(zb_os_timer_t handle, uint32_t period_ms)
{
    return xTimerChangePeriod((TimerHandle_t)handle,
                              pdMS_TO_TICKS((period_ms == 0u) ? 1u : period_ms),
                              portMAX_DELAY) == pdPASS;
}

zb_os_stream_t zb_os_stream_create(size_t capacity_bytes)
{
    return (zb_os_stream_t)xStreamBufferCreate(capacity_bytes, 1);
}

void zb_os_stream_delete(zb_os_stream_t s)
{
    if (s != NULL) {
        vStreamBufferDelete((StreamBufferHandle_t)s);
    }
}

bool zb_os_stream_reset(zb_os_stream_t s)
{
    return xStreamBufferReset((StreamBufferHandle_t)s) == pdPASS;
}

size_t zb_os_stream_send(zb_os_stream_t s, const void *data, size_t len, uint32_t timeout_ms)
{
    return xStreamBufferSend((StreamBufferHandle_t)s, data, len, zb_osal_ticks(timeout_ms));
}

size_t zb_os_stream_recv(zb_os_stream_t s, void *out, size_t len, uint32_t timeout_ms)
{
    return xStreamBufferReceive((StreamBufferHandle_t)s, out, len, zb_osal_ticks(timeout_ms));
}
