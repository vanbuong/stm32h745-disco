#include "zb_osal.h"

#include "zb_port.h"

#include "bsp/board.h"
#include "hal/wdog.h"

#include <stdlib.h>
#include <string.h>

#define ZB_OS_TIMER_MAX 16u
#define ZB_OS_YIELD_MS 16u

static void (*g_idle)(void);
static void (*g_yield)(void);
static uint8_t g_pump_depth;
static uint32_t g_yield_last;

void zb_os_set_idle_pump(void (*fn)(void))
{
    g_idle = fn;
}

void zb_os_set_yield_pump(void (*fn)(void))
{
    g_yield = fn;
}

static void idle_pump(void)
{
    if (g_pump_depth > 2u) {
        return;
    }
    g_pump_depth++;
    if (g_idle != NULL) {
        g_idle();
    } else {
        wdog_kick();
    }
    if (g_pump_depth == 1u && g_yield != NULL) {
        uint32_t now = zb_os_now_ms();

        if ((uint32_t)(now - g_yield_last) >= ZB_OS_YIELD_MS) {
            g_yield_last = now;
            g_yield();
        }
    }
    g_pump_depth--;
}

uint32_t zb_os_now_ms(void)
{
    return board_millis();
}

void zb_os_delay_ms(uint32_t ms)
{
    uint32_t start = zb_os_now_ms();

    if (ms == 0u) {
        idle_pump();
        return;
    }
    while ((uint32_t)(zb_os_now_ms() - start) < ms) {
        idle_pump();
    }
}

struct zb_os_task {
    zb_os_task_fn_t fn;
    void *arg;
};

bool zb_os_task_create(zb_os_task_fn_t fn, const char *name, size_t stack_bytes, void *arg,
                       uint32_t priority, zb_os_task_t *out_task)
{
    struct zb_os_task *t;

    (void)name;
    (void)stack_bytes;
    (void)priority;
    t = (struct zb_os_task *)malloc(sizeof(*t));
    if (t == NULL) {
        return false;
    }
    t->fn = fn;
    t->arg = arg;
    if (out_task != NULL) {
        *out_task = t;
    }
    return true;
}

void zb_os_task_delete(zb_os_task_t task)
{
    free(task);
}

struct zb_os_queue {
    uint8_t *storage;
    size_t item_size;
    size_t depth;
    size_t count;
    size_t head;
};

zb_os_queue_t zb_os_queue_create(size_t depth, size_t item_size)
{
    struct zb_os_queue *q;

    q = (struct zb_os_queue *)malloc(sizeof(*q));
    if (q == NULL) {
        return NULL;
    }
    q->storage = (uint8_t *)malloc(depth * item_size);
    if (q->storage == NULL) {
        free(q);
        return NULL;
    }
    q->item_size = item_size;
    q->depth = depth;
    q->count = 0u;
    q->head = 0u;
    return q;
}

void zb_os_queue_delete(zb_os_queue_t q)
{
    if (q != NULL) {
        free(q->storage);
        free(q);
    }
}

bool zb_os_queue_send(zb_os_queue_t q, const void *item, uint32_t timeout_ms)
{
    uint32_t start = zb_os_now_ms();
    size_t tail;

    (void)timeout_ms;
    if (q == NULL || item == NULL) {
        return false;
    }
    while (q->count == q->depth) {
        if (timeout_ms == ZB_OSAL_NO_WAIT) {
            return false;
        }
        if (timeout_ms != ZB_OSAL_WAIT_FOREVER &&
            (uint32_t)(zb_os_now_ms() - start) >= timeout_ms) {
            return false;
        }
        idle_pump();
    }
    tail = (q->head + q->count) % q->depth;
    memcpy(q->storage + tail * q->item_size, item, q->item_size);
    q->count++;
    return true;
}

bool zb_os_queue_recv(zb_os_queue_t q, void *out, uint32_t timeout_ms)
{
    uint32_t start = zb_os_now_ms();

    if (q == NULL || out == NULL) {
        return false;
    }
    while (q->count == 0u) {
        if (timeout_ms == ZB_OSAL_NO_WAIT) {
            return false;
        }
        if (timeout_ms != ZB_OSAL_WAIT_FOREVER &&
            (uint32_t)(zb_os_now_ms() - start) >= timeout_ms) {
            return false;
        }
        idle_pump();
    }
    memcpy(out, q->storage + q->head * q->item_size, q->item_size);
    q->head = (q->head + 1u) % q->depth;
    q->count--;
    return true;
}

bool zb_os_queue_peek(zb_os_queue_t q, void *out, uint32_t timeout_ms)
{
    (void)timeout_ms;
    if (q == NULL || q->count == 0u || out == NULL) {
        return false;
    }
    memcpy(out, q->storage + q->head * q->item_size, q->item_size);
    return true;
}

void zb_os_queue_reset(zb_os_queue_t q)
{
    if (q != NULL) {
        q->count = 0u;
        q->head = 0u;
    }
}

struct zb_os_mutex {
    bool recursive;
};

zb_os_mutex_t zb_os_mutex_create(bool recursive)
{
    struct zb_os_mutex *m = (struct zb_os_mutex *)malloc(sizeof(*m));

    if (m != NULL) {
        m->recursive = recursive;
    }
    return m;
}

void zb_os_mutex_delete(zb_os_mutex_t m)
{
    free(m);
}

bool zb_os_mutex_lock(zb_os_mutex_t m, uint32_t timeout_ms)
{
    (void)m;
    (void)timeout_ms;
    return true;
}

void zb_os_mutex_unlock(zb_os_mutex_t m)
{
    (void)m;
}

struct zb_os_signal {
    bool has_value;
    uint32_t value;
};

zb_os_signal_t zb_os_signal_create(void)
{
    struct zb_os_signal *s = (struct zb_os_signal *)malloc(sizeof(*s));

    if (s != NULL) {
        s->has_value = false;
        s->value = 0u;
    }
    return s;
}

void zb_os_signal_delete(zb_os_signal_t s)
{
    free(s);
}

void zb_os_signal_set(zb_os_signal_t s, uint32_t value)
{
    if (s != NULL) {
        s->value = value;
        s->has_value = true;
    }
}

bool zb_os_signal_wait(zb_os_signal_t s, uint32_t *out_value, uint32_t timeout_ms)
{
    uint32_t start = zb_os_now_ms();

    if (s == NULL) {
        return false;
    }
    while (!s->has_value) {
        if (timeout_ms == ZB_OSAL_NO_WAIT) {
            return false;
        }
        if (timeout_ms != ZB_OSAL_WAIT_FOREVER &&
            (uint32_t)(zb_os_now_ms() - start) >= timeout_ms) {
            return false;
        }
        idle_pump();
    }
    if (out_value != NULL) {
        *out_value = s->value;
    }
    s->has_value = false;
    return true;
}

void zb_os_signal_clear(zb_os_signal_t s)
{
    if (s != NULL) {
        s->has_value = false;
    }
}

struct zb_os_event {
    uint32_t bits;
};

zb_os_event_t zb_os_event_create(void)
{
    struct zb_os_event *e = (struct zb_os_event *)malloc(sizeof(*e));

    if (e != NULL) {
        e->bits = 0u;
    }
    return e;
}

void zb_os_event_delete(zb_os_event_t e)
{
    free(e);
}

void zb_os_event_set(zb_os_event_t e, uint32_t bits)
{
    if (e != NULL) {
        e->bits |= bits;
    }
}

uint32_t zb_os_event_wait(zb_os_event_t e, uint32_t bits, bool wait_all, bool clear_on_exit,
                          uint32_t timeout_ms)
{
    uint32_t start = zb_os_now_ms();
    uint32_t snapshot;

    if (e == NULL) {
        return 0u;
    }
    for (;;) {
        bool ok = wait_all ? ((e->bits & bits) == bits) : ((e->bits & bits) != 0u);
        if (ok) {
            snapshot = e->bits;
            if (clear_on_exit) {
                e->bits &= ~bits;
            }
            return snapshot;
        }
        if (timeout_ms == ZB_OSAL_NO_WAIT) {
            return 0u;
        }
        if (timeout_ms != ZB_OSAL_WAIT_FOREVER &&
            (uint32_t)(zb_os_now_ms() - start) >= timeout_ms) {
            return 0u;
        }
        idle_pump();
    }
}

struct zb_os_timer {
    uint32_t period_ms;
    uint32_t expiry_ms;
    bool auto_reload;
    bool running;
    zb_os_timer_fn_t cb;
    void *arg;
};

static struct zb_os_timer *g_timers[ZB_OS_TIMER_MAX];

zb_os_timer_t zb_os_timer_create(const char *name, uint32_t period_ms, bool auto_reload,
                                 zb_os_timer_fn_t cb, void *arg)
{
    struct zb_os_timer *t;
    uint32_t i;

    (void)name;
    t = (struct zb_os_timer *)malloc(sizeof(*t));
    if (t == NULL) {
        return NULL;
    }
    t->period_ms = period_ms;
    t->expiry_ms = 0u;
    t->auto_reload = auto_reload;
    t->running = false;
    t->cb = cb;
    t->arg = arg;
    for (i = 0u; i < ZB_OS_TIMER_MAX; i++) {
        if (g_timers[i] == NULL) {
            g_timers[i] = t;
            break;
        }
    }
    return t;
}

void zb_os_timer_delete(zb_os_timer_t t)
{
    uint32_t i;

    for (i = 0u; i < ZB_OS_TIMER_MAX; i++) {
        if (g_timers[i] == t) {
            g_timers[i] = NULL;
        }
    }
    free(t);
}

bool zb_os_timer_start(zb_os_timer_t t)
{
    if (t == NULL) {
        return false;
    }
    t->expiry_ms = zb_os_now_ms() + t->period_ms;
    t->running = true;
    return true;
}

bool zb_os_timer_stop(zb_os_timer_t t)
{
    if (t == NULL) {
        return false;
    }
    t->running = false;
    return true;
}

bool zb_os_timer_change_period(zb_os_timer_t t, uint32_t period_ms)
{
    if (t == NULL) {
        return false;
    }
    t->period_ms = period_ms;
    if (t->running) {
        t->expiry_ms = zb_os_now_ms() + period_ms;
    }
    return true;
}

void zb_os_timer_pump(void)
{
    uint32_t now = zb_os_now_ms();
    uint32_t i;

    for (i = 0u; i < ZB_OS_TIMER_MAX; i++) {
        struct zb_os_timer *t = g_timers[i];
        if (t == NULL || !t->running) {
            continue;
        }
        if ((int32_t)(now - t->expiry_ms) < 0) {
            continue;
        }
        if (t->auto_reload) {
            t->expiry_ms = now + t->period_ms;
        } else {
            t->running = false;
        }
        if (t->cb != NULL) {
            t->cb(t, t->arg);
        }
    }
}

struct zb_os_stream {
    uint8_t *storage;
    size_t capacity;
    size_t count;
    size_t head;
};

zb_os_stream_t zb_os_stream_create(size_t capacity_bytes)
{
    struct zb_os_stream *s = (struct zb_os_stream *)malloc(sizeof(*s));

    if (s == NULL) {
        return NULL;
    }
    s->storage = (uint8_t *)malloc(capacity_bytes);
    if (s->storage == NULL) {
        free(s);
        return NULL;
    }
    s->capacity = capacity_bytes;
    s->count = 0u;
    s->head = 0u;
    return s;
}

void zb_os_stream_delete(zb_os_stream_t s)
{
    if (s != NULL) {
        free(s->storage);
        free(s);
    }
}

bool zb_os_stream_reset(zb_os_stream_t s)
{
    if (s == NULL) {
        return false;
    }
    s->count = 0u;
    s->head = 0u;
    return true;
}

size_t zb_os_stream_send(zb_os_stream_t s, const void *data, size_t len, uint32_t timeout_ms)
{
    const uint8_t *src = (const uint8_t *)data;
    size_t written = 0u;

    (void)timeout_ms;
    if (s == NULL || data == NULL) {
        return 0u;
    }
    while (written < len && s->count < s->capacity) {
        size_t tail = (s->head + s->count) % s->capacity;
        s->storage[tail] = src[written++];
        s->count++;
    }
    return written;
}

size_t zb_os_stream_recv(zb_os_stream_t s, void *out, size_t len, uint32_t timeout_ms)
{
    uint8_t *dst = (uint8_t *)out;
    size_t n = 0u;
    uint32_t start = zb_os_now_ms();

    if (s == NULL || out == NULL) {
        return 0u;
    }
    while (n < len) {
        if (s->count > 0u) {
            dst[n++] = s->storage[s->head];
            s->head = (s->head + 1u) % s->capacity;
            s->count--;
            continue;
        }
        if (n > 0u || timeout_ms == ZB_OSAL_NO_WAIT) {
            break;
        }
        if (timeout_ms != ZB_OSAL_WAIT_FOREVER &&
            (uint32_t)(zb_os_now_ms() - start) >= timeout_ms) {
            break;
        }
        idle_pump();
    }
    return n;
}
