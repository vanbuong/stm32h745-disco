#ifndef HEALTH_H
#define HEALTH_H

#include "err.h"

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

#define HEALTH_WDOG_MS 8000u

typedef enum {
    HEALTH_REASON_NONE = 0,
    HEALTH_REASON_WDOG,
    HEALTH_REASON_BOR,
    HEALTH_REASON_OOM
} health_reason_t;

err_t health_init(void);
void health_reset(void);
void health_kick(void);
void health_poll(uint32_t dt_ms);
void health_note_peer(uint8_t ok);
void health_mark_oom(void);

uint8_t health_ready(void);
uint8_t health_expired(void);
uint8_t health_oom(void);
uint8_t health_peer_ok(void);
health_reason_t health_reason(void);
health_reason_t health_boot_reason(void);
const char *health_reason_text(health_reason_t reason);

/* Host-testable HIL pack: kick, remount if mounted, cfg present or defaults. */
err_t health_selftest(void);

#ifdef __cplusplus
}
#endif

#endif /* HEALTH_H */
