/*
 * zb_osal_freertos.c
 *
 * FreeRTOS / ESP-IDF backend for the iotdev_zigbee OS Abstraction Layer
 * (zb_osal.h). Built only for the on-target platform (ZB_PLATFORM_IOTDEV).
 *
 * Mapping summary:
 *   task   -> TaskHandle_t          (handle cast, no wrapper)
 *   queue  -> QueueHandle_t         (handle cast, no wrapper)
 *   event  -> EventGroupHandle_t    (handle cast, no wrapper)
 *   stream -> StreamBufferHandle_t  (handle cast, no wrapper)
 *   signal -> QueueHandle_t depth 1 (xQueueOverwrite = last-writer-wins)
 *   mutex  -> small wrapper {SemaphoreHandle_t, recursive}
 *   timer  -> TimerHandle_t + wrapper id {cb, arg} (trampoline)
 *
 * Author: Vo Van Buong (BRT-SG)
 */

#if defined(ZB_PLATFORM_IOTDEV)

#include "zb_osal.h"

#include <stdlib.h>

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"
#include "freertos/semphr.h"
#include "freertos/event_groups.h"
#include "freertos/timers.h"
#include "freertos/stream_buffer.h"

/* Translate the OSAL millisecond timeout contract to FreeRTOS ticks. */
static inline TickType_t
zb_osal_ticks(uint32_t timeout_ms)
{
    if (timeout_ms == ZB_OSAL_WAIT_FOREVER)
    {
        return portMAX_DELAY;
    }
    return pdMS_TO_TICKS(timeout_ms);
}

/* ------------------------------------------------------------------------- */
/* Time / delay                                                              */
/* ------------------------------------------------------------------------- */

uint32_t
zb_os_now_ms(void)
{
    return (uint32_t)(xTaskGetTickCount() * portTICK_PERIOD_MS);
}

void
zb_os_delay_ms(uint32_t ms)
{
    vTaskDelay(pdMS_TO_TICKS(ms));
}

/* ------------------------------------------------------------------------- */
/* Tasks                                                                     */
/* ------------------------------------------------------------------------- */

bool
zb_os_task_create(zb_os_task_fn_t fn, const char *name, size_t stack_bytes,
                  void *arg, uint32_t priority, zb_os_task_t *out_task)
{
    TaskHandle_t handle = NULL;
    /* ESP-IDF xTaskCreate takes the stack depth in bytes. */
    BaseType_t ok = xTaskCreate(fn, name ? name : "zb_task",
                                (uint32_t)stack_bytes, arg,
                                (UBaseType_t)priority, &handle);
    if (ok != pdPASS)
    {
        return false;
    }
    if (out_task)
    {
        *out_task = (zb_os_task_t)handle;
    }
    return true;
}

void
zb_os_task_delete(zb_os_task_t task)
{
    vTaskDelete((TaskHandle_t)task);
}

/* ------------------------------------------------------------------------- */
/* Message queue                                                             */
/* ------------------------------------------------------------------------- */

zb_os_queue_t
zb_os_queue_create(size_t depth, size_t item_size)
{
    return (zb_os_queue_t)xQueueCreate((UBaseType_t)depth,
                                       (UBaseType_t)item_size);
}

void
zb_os_queue_delete(zb_os_queue_t q)
{
    if (q)
    {
        vQueueDelete((QueueHandle_t)q);
    }
}

bool
zb_os_queue_send(zb_os_queue_t q, const void *item, uint32_t timeout_ms)
{
    return xQueueSendToBack((QueueHandle_t)q, item,
                            zb_osal_ticks(timeout_ms)) == pdTRUE;
}

bool
zb_os_queue_recv(zb_os_queue_t q, void *out, uint32_t timeout_ms)
{
    return xQueueReceive((QueueHandle_t)q, out,
                         zb_osal_ticks(timeout_ms)) == pdTRUE;
}

bool
zb_os_queue_peek(zb_os_queue_t q, void *out, uint32_t timeout_ms)
{
    return xQueuePeek((QueueHandle_t)q, out,
                      zb_osal_ticks(timeout_ms)) == pdTRUE;
}

void
zb_os_queue_reset(zb_os_queue_t q)
{
    (void)xQueueReset((QueueHandle_t)q);
}

/* ------------------------------------------------------------------------- */
/* Mutex                                                                     */
/* ------------------------------------------------------------------------- */

typedef struct zb_os_mutex
{
    SemaphoreHandle_t sem;
    bool              recursive;
} zb_os_mutex_impl_t;

zb_os_mutex_t
zb_os_mutex_create(bool recursive)
{
    zb_os_mutex_impl_t *m = malloc(sizeof(*m));
    if (!m)
    {
        return NULL;
    }
    m->recursive = recursive;
    m->sem = recursive ? xSemaphoreCreateRecursiveMutex()
                       : xSemaphoreCreateMutex();
    if (!m->sem)
    {
        free(m);
        return NULL;
    }
    return (zb_os_mutex_t)m;
}

void
zb_os_mutex_delete(zb_os_mutex_t handle)
{
    zb_os_mutex_impl_t *m = (zb_os_mutex_impl_t *)handle;
    if (m)
    {
        vSemaphoreDelete(m->sem);
        free(m);
    }
}

bool
zb_os_mutex_lock(zb_os_mutex_t handle, uint32_t timeout_ms)
{
    zb_os_mutex_impl_t *m = (zb_os_mutex_impl_t *)handle;
    TickType_t ticks = zb_osal_ticks(timeout_ms);
    if (m->recursive)
    {
        return xSemaphoreTakeRecursive(m->sem, ticks) == pdTRUE;
    }
    return xSemaphoreTake(m->sem, ticks) == pdTRUE;
}

void
zb_os_mutex_unlock(zb_os_mutex_t handle)
{
    zb_os_mutex_impl_t *m = (zb_os_mutex_impl_t *)handle;
    if (m->recursive)
    {
        xSemaphoreGiveRecursive(m->sem);
    }
    else
    {
        xSemaphoreGive(m->sem);
    }
}

/* ------------------------------------------------------------------------- */
/* Signal (1-deep overwrite mailbox)                                         */
/* ------------------------------------------------------------------------- */

zb_os_signal_t
zb_os_signal_create(void)
{
    return (zb_os_signal_t)xQueueCreate(1, sizeof(uint32_t));
}

void
zb_os_signal_delete(zb_os_signal_t s)
{
    if (s)
    {
        vQueueDelete((QueueHandle_t)s);
    }
}

void
zb_os_signal_set(zb_os_signal_t s, uint32_t value)
{
    /* Last-writer-wins: overwrite any value not yet consumed. */
    (void)xQueueOverwrite((QueueHandle_t)s, &value);
}

bool
zb_os_signal_wait(zb_os_signal_t s, uint32_t *out_value, uint32_t timeout_ms)
{
    uint32_t value = 0;
    if (xQueueReceive((QueueHandle_t)s, &value, zb_osal_ticks(timeout_ms))
        != pdTRUE)
    {
        return false;
    }
    if (out_value)
    {
        *out_value = value;
    }
    return true;
}

void
zb_os_signal_clear(zb_os_signal_t s)
{
    uint32_t scratch = 0;
    (void)xQueueReceive((QueueHandle_t)s, &scratch, 0);
}

/* ------------------------------------------------------------------------- */
/* Event flags                                                               */
/* ------------------------------------------------------------------------- */

zb_os_event_t
zb_os_event_create(void)
{
    return (zb_os_event_t)xEventGroupCreate();
}

void
zb_os_event_delete(zb_os_event_t e)
{
    if (e)
    {
        vEventGroupDelete((EventGroupHandle_t)e);
    }
}

void
zb_os_event_set(zb_os_event_t e, uint32_t bits)
{
    xEventGroupSetBits((EventGroupHandle_t)e, (EventBits_t)bits);
}

uint32_t
zb_os_event_wait(zb_os_event_t e, uint32_t bits, bool wait_all,
                 bool clear_on_exit, uint32_t timeout_ms)
{
    EventBits_t set = xEventGroupWaitBits(
        (EventGroupHandle_t)e, (EventBits_t)bits,
        clear_on_exit ? pdTRUE : pdFALSE,
        wait_all ? pdTRUE : pdFALSE,
        zb_osal_ticks(timeout_ms));
    return (uint32_t)set;
}

/* ------------------------------------------------------------------------- */
/* Software timer                                                            */
/* ------------------------------------------------------------------------- */

typedef struct
{
    zb_os_timer_fn_t cb;
    void            *arg;
} zb_os_timer_ctx_t;

static void
zb_os_timer_trampoline(TimerHandle_t xTimer)
{
    zb_os_timer_ctx_t *ctx = (zb_os_timer_ctx_t *)pvTimerGetTimerID(xTimer);
    if (ctx && ctx->cb)
    {
        ctx->cb((zb_os_timer_t)xTimer, ctx->arg);
    }
}

zb_os_timer_t
zb_os_timer_create(const char *name, uint32_t period_ms, bool auto_reload,
                   zb_os_timer_fn_t cb, void *arg)
{
    zb_os_timer_ctx_t *ctx = malloc(sizeof(*ctx));
    if (!ctx)
    {
        return NULL;
    }
    ctx->cb = cb;
    ctx->arg = arg;

    TimerHandle_t t = xTimerCreate(name ? name : "zb_timer",
                                   pdMS_TO_TICKS(period_ms),
                                   auto_reload ? pdTRUE : pdFALSE,
                                   ctx, zb_os_timer_trampoline);
    if (!t)
    {
        free(ctx);
        return NULL;
    }
    return (zb_os_timer_t)t;
}

void
zb_os_timer_delete(zb_os_timer_t handle)
{
    TimerHandle_t t = (TimerHandle_t)handle;
    if (!t)
    {
        return;
    }
    zb_os_timer_ctx_t *ctx = (zb_os_timer_ctx_t *)pvTimerGetTimerID(t);
    if (xTimerDelete(t, portMAX_DELAY) == pdPASS)
    {
        free(ctx);
    }
}

bool
zb_os_timer_start(zb_os_timer_t handle)
{
    return xTimerStart((TimerHandle_t)handle, portMAX_DELAY) == pdPASS;
}

bool
zb_os_timer_stop(zb_os_timer_t handle)
{
    return xTimerStop((TimerHandle_t)handle, portMAX_DELAY) == pdPASS;
}

bool
zb_os_timer_change_period(zb_os_timer_t handle, uint32_t period_ms)
{
    return xTimerChangePeriod((TimerHandle_t)handle, pdMS_TO_TICKS(period_ms),
                              portMAX_DELAY) == pdPASS;
}

/* ------------------------------------------------------------------------- */
/* Byte-stream buffer                                                        */
/* ------------------------------------------------------------------------- */

zb_os_stream_t
zb_os_stream_create(size_t capacity_bytes)
{
    /* Trigger level 1: unblock a reader as soon as any byte is available,
     * matching the existing xStreamBufferCreate(rx_size, 1) usage. */
    return (zb_os_stream_t)xStreamBufferCreate(capacity_bytes, 1);
}

void
zb_os_stream_delete(zb_os_stream_t s)
{
    if (s)
    {
        vStreamBufferDelete((StreamBufferHandle_t)s);
    }
}

bool
zb_os_stream_reset(zb_os_stream_t s)
{
    return xStreamBufferReset((StreamBufferHandle_t)s) == pdPASS;
}

size_t
zb_os_stream_send(zb_os_stream_t s, const void *data, size_t len,
                  uint32_t timeout_ms)
{
    return xStreamBufferSend((StreamBufferHandle_t)s, data, len,
                             zb_osal_ticks(timeout_ms));
}

size_t
zb_os_stream_recv(zb_os_stream_t s, void *out, size_t len, uint32_t timeout_ms)
{
    return xStreamBufferReceive((StreamBufferHandle_t)s, out, len,
                                zb_osal_ticks(timeout_ms));
}

#endif /* ZB_PLATFORM_IOTDEV */
