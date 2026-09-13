/*
 * zb_osal.h
 *
 * OS Abstraction Layer (OSAL) for the iotdev_zigbee driver.
 *
 * Defines an RTOS-agnostic facade over the OS synchronisation and timing
 * primitives the protocol/driver layers need: tasks, message queues, mutexes,
 * completion signals, event flags, software timers, a byte-stream buffer and
 * monotonic time. The interface exposes only opaque handle types and a
 * millisecond-based timing contract — no FreeRTOS type, no tick unit, and no
 * "notify a specific task" concept leaks across this boundary.
 *
 * Backends are selected at build time:
 *   - zb_osal_freertos.c  production backend over FreeRTOS/ESP-IDF
 *                         (built when ZB_PLATFORM_IOTDEV is defined)
 *   - zb_osal_host.c      host backend for off-target unit testing (no RTOS)
 *
 * See FSD iotdev-zigbee-framework-fsd.md sec. 2.4 / 3.4 (Phase 4) and
 * requirements FR-10.1 .. FR-10.5.
 *
 * Author: Vo Van Buong (BRT-SG)
 */

#ifndef ZB_OSAL_H
#define ZB_OSAL_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C"
{
#endif /* __cplusplus */

/* ------------------------------------------------------------------------- */
/* Timing contract                                                           */
/* ------------------------------------------------------------------------- */

/** Block until the operation can complete (no timeout). */
#define ZB_OSAL_WAIT_FOREVER    (0xFFFFFFFFu)

/** Return immediately without blocking. */
#define ZB_OSAL_NO_WAIT         (0u)

/* ------------------------------------------------------------------------- */
/* Opaque handle types                                                       */
/* ------------------------------------------------------------------------- */

typedef struct zb_os_task    *zb_os_task_t;     /**< Execution thread.        */
typedef struct zb_os_queue   *zb_os_queue_t;    /**< Fixed-item FIFO queue.   */
typedef struct zb_os_mutex   *zb_os_mutex_t;    /**< Mutual-exclusion lock.   */
typedef struct zb_os_signal  *zb_os_signal_t;   /**< 32-bit overwrite signal. */
typedef struct zb_os_event   *zb_os_event_t;    /**< Bit-group event flags.   */
typedef struct zb_os_timer   *zb_os_timer_t;    /**< One-shot/periodic timer. */
typedef struct zb_os_stream  *zb_os_stream_t;   /**< Byte-stream buffer.      */

/** Task entry point. Receives the argument passed to zb_os_task_create(). */
typedef void (*zb_os_task_fn_t)(void *arg);

/** Software-timer expiry callback. Runs in the timer service context. */
typedef void (*zb_os_timer_fn_t)(zb_os_timer_t timer, void *arg);

/* ------------------------------------------------------------------------- */
/* Time / delay                                                              */
/* ------------------------------------------------------------------------- */

/** Monotonic millisecond counter since scheduler start (wraps at 2^32 ms). */
uint32_t zb_os_now_ms(void);

/** Block the calling task for at least @p ms milliseconds. */
void zb_os_delay_ms(uint32_t ms);

/* ------------------------------------------------------------------------- */
/* Tasks                                                                     */
/* ------------------------------------------------------------------------- */

/**
 * @brief Create and start a task.
 * @param fn         Entry point.
 * @param name       Human-readable task name (may be NULL).
 * @param stack_bytes Stack size in bytes.
 * @param arg        Opaque argument passed to @p fn.
 * @param priority   Backend-relative priority (higher = more urgent).
 * @param out_task   Receives the handle (may be NULL if not needed).
 * @return true on success.
 */
bool zb_os_task_create(zb_os_task_fn_t fn, const char *name,
                       size_t stack_bytes, void *arg, uint32_t priority,
                       zb_os_task_t *out_task);

/** Delete a task; pass NULL to delete the calling task (never returns then). */
void zb_os_task_delete(zb_os_task_t task);

/* ------------------------------------------------------------------------- */
/* Message queue (fixed-size items, FIFO)                                    */
/* ------------------------------------------------------------------------- */

zb_os_queue_t zb_os_queue_create(size_t depth, size_t item_size);
void          zb_os_queue_delete(zb_os_queue_t q);

/** Enqueue a copy of @p item at the back. @return true if queued in time. */
bool zb_os_queue_send(zb_os_queue_t q, const void *item, uint32_t timeout_ms);

/** Dequeue into @p out. @return true if an item was received in time. */
bool zb_os_queue_recv(zb_os_queue_t q, void *out, uint32_t timeout_ms);

/** Copy the head item into @p out without removing it. @return true if present. */
bool zb_os_queue_peek(zb_os_queue_t q, void *out, uint32_t timeout_ms);

/** Discard all queued items, leaving the queue empty. */
void zb_os_queue_reset(zb_os_queue_t q);

/* ------------------------------------------------------------------------- */
/* Mutex                                                                     */
/* ------------------------------------------------------------------------- */

/** @param recursive true for a lock the owner may take re-entrantly. */
zb_os_mutex_t zb_os_mutex_create(bool recursive);
void          zb_os_mutex_delete(zb_os_mutex_t m);
bool          zb_os_mutex_lock(zb_os_mutex_t m, uint32_t timeout_ms);
void          zb_os_mutex_unlock(zb_os_mutex_t m);

/* ------------------------------------------------------------------------- */
/* Signal (single 32-bit value, last-writer-wins, cleared on wait)           */
/*                                                                           */
/* Replaces the direct-task-notification pattern: a requester waits on a     */
/* signal carried in its request context; the dispatch path sets it. Set     */
/* overwrites any pending value; wait returns and clears the latest value.   */
/* ------------------------------------------------------------------------- */

zb_os_signal_t zb_os_signal_create(void);
void           zb_os_signal_delete(zb_os_signal_t s);

/** Post @p value, overwriting any value not yet consumed. Task context. */
void zb_os_signal_set(zb_os_signal_t s, uint32_t value);

/**
 * @brief Wait for a posted value.
 * @param out_value Receives the value (may be NULL).
 * @return true if a value arrived within @p timeout_ms, false on timeout.
 */
bool zb_os_signal_wait(zb_os_signal_t s, uint32_t *out_value,
                       uint32_t timeout_ms);

/** Discard any pending value without blocking. */
void zb_os_signal_clear(zb_os_signal_t s);

/* ------------------------------------------------------------------------- */
/* Event flags (bit group)                                                   */
/* ------------------------------------------------------------------------- */

zb_os_event_t zb_os_event_create(void);
void          zb_os_event_delete(zb_os_event_t e);
void          zb_os_event_set(zb_os_event_t e, uint32_t bits);

/**
 * @brief Wait for bits to be set.
 * @param bits        Bit mask to wait for.
 * @param wait_all    true: wait for all bits; false: wait for any.
 * @param clear_on_exit true: atomically clear the waited bits on success.
 * @return The bit set at the moment the wait unblocked (0 on timeout).
 */
uint32_t zb_os_event_wait(zb_os_event_t e, uint32_t bits, bool wait_all,
                          bool clear_on_exit, uint32_t timeout_ms);

/* ------------------------------------------------------------------------- */
/* Software timer                                                            */
/* ------------------------------------------------------------------------- */

/** @param auto_reload true for a periodic timer, false for one-shot. */
zb_os_timer_t zb_os_timer_create(const char *name, uint32_t period_ms,
                                 bool auto_reload, zb_os_timer_fn_t cb,
                                 void *arg);
void          zb_os_timer_delete(zb_os_timer_t t);
bool          zb_os_timer_start(zb_os_timer_t t);
bool          zb_os_timer_stop(zb_os_timer_t t);
bool          zb_os_timer_change_period(zb_os_timer_t t, uint32_t period_ms);

/* ------------------------------------------------------------------------- */
/* Byte-stream buffer (single-producer / single-consumer)                    */
/* ------------------------------------------------------------------------- */

zb_os_stream_t zb_os_stream_create(size_t capacity_bytes);
void           zb_os_stream_delete(zb_os_stream_t s);
bool           zb_os_stream_reset(zb_os_stream_t s);

/** Append up to @p len bytes. @return number of bytes accepted. */
size_t zb_os_stream_send(zb_os_stream_t s, const void *data, size_t len,
                         uint32_t timeout_ms);

/** Read up to @p len bytes. @return number of bytes read. */
size_t zb_os_stream_recv(zb_os_stream_t s, void *out, size_t len,
                         uint32_t timeout_ms);

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* ZB_OSAL_H */
