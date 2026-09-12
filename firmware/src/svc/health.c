#include "svc/health.h"

#include "hal/wdog.h"
#include "svc/cfg.h"
#include "svc/vfs.h"

#if defined(CORE_CM7)
#include "stm32h7xx.h"
#endif

static uint8_t g_ready;
static uint8_t g_expired;
static uint8_t g_oom;
static uint8_t g_peer;
static uint32_t g_now_ms;
static uint32_t g_last_kick;
static health_reason_t g_reason;
static health_reason_t g_boot;

static void read_boot_reason(void)
{
    g_boot = HEALTH_REASON_NONE;
#if defined(CORE_CM7)
    if ((RCC->RSR & RCC_RSR_IWDG1RSTF) != 0u) {
        g_boot = HEALTH_REASON_WDOG;
    } else if ((RCC->RSR & RCC_RSR_BORRSTF) != 0u) {
        g_boot = HEALTH_REASON_BOR;
    }
    RCC->RSR |= RCC_RSR_RMVF;
#endif
}

void health_reset(void)
{
    g_ready = 0u;
    g_expired = 0u;
    g_oom = 0u;
    g_peer = 1u;
    g_now_ms = 0u;
    g_last_kick = 0u;
    g_reason = HEALTH_REASON_NONE;
    g_boot = HEALTH_REASON_NONE;
}

err_t health_init(void)
{
    health_reset();
    g_ready = 1u;
    read_boot_reason();
    return ERR_OK;
}

void health_kick(void)
{
    if (g_ready == 0u) {
        return;
    }
    g_last_kick = g_now_ms;
}

void health_poll(uint32_t dt_ms)
{
    if (g_ready == 0u) {
        return;
    }
    g_now_ms += dt_ms;
    wdog_kick();
    if ((g_now_ms - g_last_kick) > HEALTH_WDOG_MS) {
        g_expired = 1u;
        g_reason = HEALTH_REASON_WDOG;
    }
}

void health_note_peer(uint8_t ok)
{
    g_peer = (ok != 0u) ? 1u : 0u;
}

void health_mark_oom(void)
{
    g_oom = 1u;
    g_reason = HEALTH_REASON_OOM;
}

uint8_t health_ready(void)
{
    return g_ready;
}

uint8_t health_expired(void)
{
    return g_expired;
}

uint8_t health_oom(void)
{
    return g_oom;
}

uint8_t health_peer_ok(void)
{
    return g_peer;
}

health_reason_t health_reason(void)
{
    return g_reason;
}

health_reason_t health_boot_reason(void)
{
    return g_boot;
}

const char *health_reason_text(health_reason_t reason)
{
    if (reason == HEALTH_REASON_WDOG) {
        return "watchdog";
    }
    if (reason == HEALTH_REASON_BOR) {
        return "brown-out";
    }
    if (reason == HEALTH_REASON_OOM) {
        return "out of memory";
    }
    return "ok";
}

err_t health_selftest(void)
{
    err_t e;

    if (g_ready == 0u) {
        (void)health_init();
    }
    health_kick();
    health_poll(1u);
    if (health_expired() != 0u) {
        return ERR_IO;
    }
    if (vfs_mounted() != 0) {
        e = vfs_remount();
        if (e != ERR_OK && vfs_mounted() == 0) {
            e = vfs_mount();
            if (e != ERR_OK) {
                return e;
            }
        }
    }
    (void)cfg_init();
    if (cfg_brightness() < CFG_BRIGHT_MIN || cfg_brightness() > 100u) {
        return ERR_INVAL;
    }
    if (cfg_volume() > 100u) {
        return ERR_INVAL;
    }
    return ERR_OK;
}
