#include "svc/time.h"

#include <stddef.h>
#include <stdint.h>

#if defined(CORE_CM7)
#include "bsp/board.h"
#include "lwip/apps/sntp.h"
#include "lwip/ip_addr.h"
#include "svc/net.h"
#endif

static time_ntp_st_t g_ntp = TIME_NTP_IDLE;

#if !defined(CORE_CM7)
static uint8_t g_ready;
static uint32_t g_ms;
static uint16_t g_year;
static uint8_t g_month;
static uint8_t g_day;
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

static void add_days(uint32_t days)
{
    while (days > 0u) {
        uint8_t md = time_month_days(g_year, g_month);
        uint8_t left;

        if (md == 0u || g_day < 1u || g_day > md) {
            g_day = 1u;
            md = time_month_days(g_year, g_month);
        }
        left = (uint8_t)((md - g_day) + 1u);
        if (days < (uint32_t)left) {
            g_day = (uint8_t)(g_day + days);
            return;
        }
        days -= (uint32_t)left;
        g_day = 1u;
        g_month++;
        if (g_month > 12u) {
            g_month = 1u;
            g_year++;
        }
    }
}
#endif

uint8_t time_month_days(uint16_t year, uint8_t month)
{
    static const uint8_t k_days[12] = {31u, 28u, 31u, 30u, 31u, 30u, 31u, 31u, 30u, 31u, 30u, 31u};
    uint8_t leap;

    if (month < 1u || month > 12u) {
        return 0u;
    }
    leap = ((year % 4u) == 0u && (year % 100u) != 0u) || ((year % 400u) == 0u) ? 1u : 0u;
    if (month == 2u && leap != 0u) {
        return 29u;
    }
    return k_days[month - 1u];
}

uint8_t time_weekday(uint16_t year, uint8_t month, uint8_t day)
{
    static const uint8_t k_t[12] = {0u, 3u, 2u, 5u, 0u, 3u, 5u, 1u, 4u, 6u, 2u, 4u};
    uint16_t y = year;

    if (month < 1u || month > 12u || day < 1u) {
        return 0u;
    }
    if (month < 3u) {
        y--;
    }
    return (uint8_t)((y + (y / 4u) - (y / 100u) + (y / 400u) + k_t[month - 1u] + day) % 7u);
}

void time_unix_to_civil(uint32_t unix_sec, time_civil_t *out)
{
    uint32_t days;
    uint32_t rem;
    uint16_t year;
    uint8_t month;

    if (out == NULL) {
        return;
    }
    days = unix_sec / 86400u;
    rem = unix_sec % 86400u;
    out->hour = (uint8_t)(rem / 3600u);
    out->min = (uint8_t)((rem % 3600u) / 60u);
    out->sec = (uint8_t)(rem % 60u);
    year = 1970u;
    for (;;) {
        uint16_t ydays =
            (uint16_t)(((year % 4u) == 0u && (year % 100u) != 0u) || ((year % 400u) == 0u) ? 366u
                                                                                           : 365u);
        if (days < (uint32_t)ydays) {
            break;
        }
        days -= (uint32_t)ydays;
        year++;
        if (year > 2199u) {
            break;
        }
    }
    out->year = year;
    month = 1u;
    while (month <= 12u) {
        uint8_t md = time_month_days(year, month);
        if (days < (uint32_t)md) {
            break;
        }
        days -= (uint32_t)md;
        month++;
    }
    out->month = month;
    out->day = (uint8_t)(days + 1u);
    out->wday = time_weekday(out->year, out->month, out->day);
}

static void apply_civil(const time_civil_t *in)
{
#if defined(CORE_CM7)
    (void)board_rtc_set_date(in->year, in->month, in->day, in->hour, in->min, in->sec);
#else
    uint32_t total;

    g_year = in->year;
    g_month = in->month;
    g_day = in->day;
    g_hh = in->hour;
    g_mm = in->min;
    g_ss = in->sec;
    total = ((uint32_t)in->hour * 3600u) + ((uint32_t)in->min * 60u) + in->sec;
    g_ms = total * 1000u;
#endif
}

void time_ntp_apply_unix(uint32_t unix_sec)
{
    time_civil_t c;

    time_unix_to_civil(unix_sec, &c);
    apply_civil(&c);
    g_ntp = TIME_NTP_OK;
}

time_ntp_st_t time_ntp_state(void)
{
    return g_ntp;
}

const char *time_ntp_str(void)
{
    if (g_ntp == TIME_NTP_OK) {
        return "NTP synced (UTC)";
    }
    if (g_ntp == TIME_NTP_WAIT) {
        return "NTP waiting";
    }
    if (g_ntp == TIME_NTP_ERR) {
        return "NTP error";
    }
    return "NTP idle";
}

static void ntp_poll(void)
{
#if defined(CORE_CM7)
    net_info_t inf;
    ip_addr_t addr;

    if (net_service_info(&inf) != ERR_OK || inf.link != NET_LINK_UP || inf.ipv4 == 0u) {
        if (sntp_enabled() != 0u) {
            sntp_stop();
        }
        if (g_ntp == TIME_NTP_WAIT) {
            g_ntp = TIME_NTP_IDLE;
        }
        return;
    }
    if (sntp_enabled() == 0u) {
        IP4_ADDR(&addr, 216, 239, 35, 0);
        sntp_setoperatingmode(SNTP_OPMODE_POLL);
        sntp_setserver(0, &addr);
        sntp_init();
        g_ntp = TIME_NTP_WAIT;
    }
#else
    (void)0;
#endif
}

err_t time_init(void)
{
    g_ntp = TIME_NTP_IDLE;
#if defined(CORE_CM7)
    return board_rtc_init();
#else
    g_ready = 1u;
    g_ms = 0u;
    g_year = 2026u;
    g_month = 1u;
    g_day = 1u;
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
    uint32_t days;

    g_ms += dt_ms;
    days = g_ms / 86400000u;
    if (days > 0u) {
        g_ms %= 86400000u;
        add_days(days);
    }
    from_ms();
    (void)g_ready;
#endif
    ntp_poll();
}

err_t time_now(time_civil_t *out)
{
#if defined(CORE_CM7)
    err_t e;

    if (out == NULL) {
        return ERR_INVAL;
    }
    e = board_rtc_get_date(&out->year, &out->month, &out->day, &out->hour, &out->min, &out->sec);
    if (e != ERR_OK) {
        return e;
    }
    out->wday = time_weekday(out->year, out->month, out->day);
    return ERR_OK;
#else
    if (out == NULL) {
        return ERR_INVAL;
    }
    out->year = g_year;
    out->month = g_month;
    out->day = g_day;
    out->hour = g_hh;
    out->min = g_mm;
    out->sec = g_ss;
    out->wday = time_weekday(g_year, g_month, g_day);
    return ERR_OK;
#endif
}

err_t time_set(const time_civil_t *in)
{
    if (in == NULL || in->month < 1u || in->month > 12u || in->day < 1u || in->hour > 23u ||
        in->min > 59u || in->sec > 59u) {
        return ERR_INVAL;
    }
    if (in->day > time_month_days(in->year, in->month)) {
        return ERR_INVAL;
    }
    apply_civil(in);
    return ERR_OK;
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
