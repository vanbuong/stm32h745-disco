#include "svc/time.h"

#include <stddef.h>
#include <stdint.h>

#if defined(CORE_CM7)
#include "bsp/board.h"
#else

static uint8_t g_ready;
static uint32_t g_ms;
static uint8_t g_hh;
static uint8_t g_mm;
static uint8_t g_ss;

static void from_ms(void)
{
    uint32_t sec = g_ms / 1000u;
    uint32_t mins;

    g_ss = (uint8_t)(sec % 60u);
    mins = sec / 60u;
    g_mm = (uint8_t)(mins % 60u);
    g_hh = (uint8_t)((mins / 60u) % 24u);
}

#endif

err_t time_init(void)
{
#if defined(CORE_CM7)
    return board_rtc_init();
#else
    g_ready = 1u;
    g_ms = 0u;
    g_hh = 0u;
    g_mm = 0u;
    g_ss = 0u;
    return ERR_OK;
#endif
}

void time_poll(uint32_t dt_ms)
{
#if defined(CORE_CM7)
    (void)dt_ms;
#else
    g_ms += dt_ms;
    from_ms();
    (void)g_ready;
#endif
}

err_t time_rtc_get(uint8_t *hh, uint8_t *mm, uint8_t *ss)
{
#if defined(CORE_CM7)
    return board_rtc_get(hh, mm, ss);
#else
    if (hh == NULL || mm == NULL || ss == NULL) {
        return ERR_INVAL;
    }
    *hh = g_hh;
    *mm = g_mm;
    *ss = g_ss;
    return ERR_OK;
#endif
}

err_t time_rtc_set(uint8_t hh, uint8_t mm, uint8_t ss)
{
#if defined(CORE_CM7)
    return board_rtc_set(hh, mm, ss);
#else
    uint32_t total;

    if (hh > 23u || mm > 59u || ss > 59u) {
        return ERR_INVAL;
    }
    total = ((uint32_t)hh * 3600u) + ((uint32_t)mm * 60u) + ss;
    g_ms = total * 1000u;
    g_hh = hh;
    g_mm = mm;
    g_ss = ss;
    return ERR_OK;
#endif
}
