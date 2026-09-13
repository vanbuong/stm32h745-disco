/*
 * zb_osal_host.c
 *
 * Host backend for the iotdev_zigbee OS Abstraction Layer (zb_osal.h),
 * used for off-target unit testing of the protocol/driver layers with no
 * RTOS present. Built only for non-target builds (i.e. when ZB_PLATFORM_IOTDEV
 * is NOT defined) — the production ESP-IDF build compiles zb_osal_freertos.c.
 *
 * Concurrency model: the host backend is SINGLE-THREADED. Unit tests drive the
 * code under test synchronously, so the primitives are non-blocking — a wait on
 * an empty queue/signal/stream returns "nothing available" immediately instead
 * of blocking (which would deadlock a single-threaded test). Queues, the signal
 * mailbox, the byte stream, event flags and the monotonic clock are real,
 * deterministic in-memory implementations; tasks and software timers are
 * lightweight state-tracking stubs (no scheduler runs them).
 *
 * Author: Vo Van Buong (BRT-SG)
 */

#if !defined(ZB_PLATFORM_IOTDEV)

#include "zb_osal.h"

#include <stdlib.h>
#include <string.h>

/* ------------------------------------------------------------------------- */
/* Time / delay                                                              */
/* ------------------------------------------------------------------------- */

static uint32_t g_now_ms = 0;

uint32_t
zb_os_now_ms(void)
{
    return g_now_ms;
}

void
zb_os_delay_ms(uint32_t ms)
{
    /* No scheduler on the host: a delay simply advances the monotonic clock so
     * deadline arithmetic in the code under test behaves deterministically. */
    g_now_ms += ms;
}

/* ------------------------------------------------------------------------- */
/* Tasks (state-tracking stub — no thread is spawned)                        */
/* ------------------------------------------------------------------------- */

struct zb_os_task
{
    zb_os_task_fn_t fn;
    void           *arg;
};

bool
zb_os_task_create(zb_os_task_fn_t fn, const char *name, size_t stack_bytes,
                  void *arg, uint32_t priority, zb_os_task_t *out_task)
{
    (void)name;
    (void)stack_bytes;
    (void)priority;
    struct zb_os_task *t = (struct zb_os_task *)malloc(sizeof(*t));
    if (!t)
    {
        return false;
    }
    t->fn = fn;
    t->arg = arg;
    if (out_task)
    {
        *out_task = t;
    }
    return true;
}

void
zb_os_task_delete(zb_os_task_t task)
{
    free(task);
}

/* ------------------------------------------------------------------------- */
/* Message queue (real FIFO ring buffer, non-blocking)                       */
/* ------------------------------------------------------------------------- */

struct zb_os_queue
{
    uint8_t *storage;
    size_t   item_size;
    size_t   depth;
    size_t   count;
    size_t   head;     /* index of next item to dequeue */
};

zb_os_queue_t
zb_os_queue_create(size_t depth, size_t item_size)
{
    struct zb_os_queue *q = (struct zb_os_queue *)malloc(sizeof(*q));
    if (!q)
    {
        return NULL;
    }
    q->storage = (uint8_t *)malloc(depth * item_size);
    if (!q->storage)
    {
        free(q);
        return NULL;
    }
    q->item_size = item_size;
    q->depth = depth;
    q->count = 0;
    q->head = 0;
    return q;
}

void
zb_os_queue_delete(zb_os_queue_t q)
{
    if (q)
    {
        free(q->storage);
        free(q);
    }
}

bool
zb_os_queue_send(zb_os_queue_t q, const void *item, uint32_t timeout_ms)
{
    (void)timeout_ms;
    if (!q || q->count == q->depth)
    {
        return false;
    }
    size_t tail = (q->head + q->count) % q->depth;
    memcpy(q->storage + tail * q->item_size, item, q->item_size);
    q->count++;
    return true;
}

bool
zb_os_queue_recv(zb_os_queue_t q, void *out, uint32_t timeout_ms)
{
    (void)timeout_ms;
    if (!q || q->count == 0)
    {
        return false;
    }
    memcpy(out, q->storage + q->head * q->item_size, q->item_size);
    q->head = (q->head + 1) % q->depth;
    q->count--;
    return true;
}

bool
zb_os_queue_peek(zb_os_queue_t q, void *out, uint32_t timeout_ms)
{
    (void)timeout_ms;
    if (!q || q->count == 0)
    {
        return false;
    }
    memcpy(out, q->storage + q->head * q->item_size, q->item_size);
    return true;
}

void
zb_os_queue_reset(zb_os_queue_t q)
{
    if (q)
    {
        q->count = 0;
        q->head = 0;
    }
}

/* ------------------------------------------------------------------------- */
/* Mutex (no-op on a single-threaded host)                                   */
/* ------------------------------------------------------------------------- */

struct zb_os_mutex
{
    bool recursive;
};

zb_os_mutex_t
zb_os_mutex_create(bool recursive)
{
    struct zb_os_mutex *m = (struct zb_os_mutex *)malloc(sizeof(*m));
    if (m)
    {
        m->recursive = recursive;
    }
    return m;
}

void
zb_os_mutex_delete(zb_os_mutex_t m)
{
    free(m);
}

bool
zb_os_mutex_lock(zb_os_mutex_t m, uint32_t timeout_ms)
{
    (void)m;
    (void)timeout_ms;
    return true;
}

void
zb_os_mutex_unlock(zb_os_mutex_t m)
{
    (void)m;
}

/* ------------------------------------------------------------------------- */
/* Signal (single value, last-writer-wins, cleared on consume)               */
/* ------------------------------------------------------------------------- */

struct zb_os_signal
{
    bool     has_value;
    uint32_t value;
};

zb_os_signal_t
zb_os_signal_create(void)
{
    struct zb_os_signal *s = (struct zb_os_signal *)malloc(sizeof(*s));
    if (s)
    {
        s->has_value = false;
        s->value = 0;
    }
    return s;
}

void
zb_os_signal_delete(zb_os_signal_t s)
{
    free(s);
}

void
zb_os_signal_set(zb_os_signal_t s, uint32_t value)
{
    if (s)
    {
        s->value = value;     /* overwrite any value not yet consumed */
        s->has_value = true;
    }
}

bool
zb_os_signal_wait(zb_os_signal_t s, uint32_t *out_value, uint32_t timeout_ms)
{
    (void)timeout_ms;
    if (!s || !s->has_value)
    {
        return false;
    }
    if (out_value)
    {
        *out_value = s->value;
    }
    s->has_value = false;     /* consume */
    return true;
}

void
zb_os_signal_clear(zb_os_signal_t s)
{
    if (s)
    {
        s->has_value = false;
    }
}

/* ------------------------------------------------------------------------- */
/* Event flags                                                               */
/* ------------------------------------------------------------------------- */

struct zb_os_event
{
    uint32_t bits;
};

zb_os_event_t
zb_os_event_create(void)
{
    struct zb_os_event *e = (struct zb_os_event *)malloc(sizeof(*e));
    if (e)
    {
        e->bits = 0;
    }
    return e;
}

void
zb_os_event_delete(zb_os_event_t e)
{
    free(e);
}

void
zb_os_event_set(zb_os_event_t e, uint32_t bits)
{
    if (e)
    {
        e->bits |= bits;
    }
}

uint32_t
zb_os_event_wait(zb_os_event_t e, uint32_t bits, bool wait_all,
                 bool clear_on_exit, uint32_t timeout_ms)
{
    (void)timeout_ms;
    if (!e)
    {
        return 0;
    }
    bool satisfied = wait_all ? ((e->bits & bits) == bits)
                              : ((e->bits & bits) != 0);
    if (!satisfied)
    {
        return 0;
    }
    uint32_t snapshot = e->bits;
    if (clear_on_exit)
    {
        e->bits &= ~bits;
    }
    return snapshot;
}

/* ------------------------------------------------------------------------- */
/* Software timer (state-tracking stub — no scheduler fires it)              */
/* ------------------------------------------------------------------------- */

struct zb_os_timer
{
    uint32_t         period_ms;
    bool             auto_reload;
    bool             running;
    zb_os_timer_fn_t cb;
    void            *arg;
};

zb_os_timer_t
zb_os_timer_create(const char *name, uint32_t period_ms, bool auto_reload,
                   zb_os_timer_fn_t cb, void *arg)
{
    (void)name;
    struct zb_os_timer *t = (struct zb_os_timer *)malloc(sizeof(*t));
    if (t)
    {
        t->period_ms = period_ms;
        t->auto_reload = auto_reload;
        t->running = false;
        t->cb = cb;
        t->arg = arg;
    }
    return t;
}

void
zb_os_timer_delete(zb_os_timer_t t)
{
    free(t);
}

bool
zb_os_timer_start(zb_os_timer_t t)
{
    if (!t)
    {
        return false;
    }
    t->running = true;
    return true;
}

bool
zb_os_timer_stop(zb_os_timer_t t)
{
    if (!t)
    {
        return false;
    }
    t->running = false;
    return true;
}

bool
zb_os_timer_change_period(zb_os_timer_t t, uint32_t period_ms)
{
    if (!t)
    {
        return false;
    }
    t->period_ms = period_ms;
    return true;
}

/* ------------------------------------------------------------------------- */
/* Byte-stream buffer (real SPSC byte ring, non-blocking)                    */
/* ------------------------------------------------------------------------- */

struct zb_os_stream
{
    uint8_t *storage;
    size_t   capacity;
    size_t   count;
    size_t   head;
};

zb_os_stream_t
zb_os_stream_create(size_t capacity_bytes)
{
    struct zb_os_stream *s = (struct zb_os_stream *)malloc(sizeof(*s));
    if (!s)
    {
        return NULL;
    }
    s->storage = (uint8_t *)malloc(capacity_bytes);
    if (!s->storage)
    {
        free(s);
        return NULL;
    }
    s->capacity = capacity_bytes;
    s->count = 0;
    s->head = 0;
    return s;
}

void
zb_os_stream_delete(zb_os_stream_t s)
{
    if (s)
    {
        free(s->storage);
        free(s);
    }
}

bool
zb_os_stream_reset(zb_os_stream_t s)
{
    if (!s)
    {
        return false;
    }
    s->count = 0;
    s->head = 0;
    return true;
}

size_t
zb_os_stream_send(zb_os_stream_t s, const void *data, size_t len,
                  uint32_t timeout_ms)
{
    (void)timeout_ms;
    if (!s)
    {
        return 0;
    }
    const uint8_t *src = (const uint8_t *)data;
    size_t written = 0;
    while (written < len && s->count < s->capacity)
    {
        size_t tail = (s->head + s->count) % s->capacity;
        s->storage[tail] = src[written++];
        s->count++;
    }
    return written;
}

size_t
zb_os_stream_recv(zb_os_stream_t s, void *out, size_t len, uint32_t timeout_ms)
{
    (void)timeout_ms;
    if (!s)
    {
        return 0;
    }
    uint8_t *dst = (uint8_t *)out;
    size_t read = 0;
    while (read < len && s->count > 0)
    {
        dst[read++] = s->storage[s->head];
        s->head = (s->head + 1) % s->capacity;
        s->count--;
    }
    return read;
}

#endif /* !ZB_PLATFORM_IOTDEV */
