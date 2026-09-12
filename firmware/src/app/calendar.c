#include "app/calendar.h"

#include "svc/time.h"

#include <stddef.h>

static uint16_t g_year = 2026u;
static uint8_t g_month = 1u;
static uint32_t g_gen;
static char g_title[24];
static char g_clock[12];
static char g_date[24];

static const char *const k_months[12] = {"January",   "February", "March",    "April",
                                         "May",       "June",     "July",     "August",
                                         "September", "October",  "November", "December"};
static const char *const k_wdays[7] = {"Sun", "Mon", "Tue", "Wed", "Thu", "Fri", "Sat"};

static void bump(void)
{
    g_gen++;
}

static void put_u16(char *out, size_t n, uint16_t v, size_t *o)
{
    char tmp[6];
    int i = 6;

    if (out == NULL || o == NULL || n == 0u) {
        return;
    }
    tmp[5] = '\0';
    if (v == 0u) {
        tmp[--i] = '0';
    }
    while (v > 0u && i > 0) {
        tmp[--i] = (char)('0' + (v % 10u));
        v = (uint16_t)(v / 10u);
    }
    while (tmp[i] != '\0' && *o + 1u < n) {
        out[(*o)++] = tmp[i++];
    }
}

static void copy_str(char *out, size_t n, const char *s, size_t *o)
{
    if (out == NULL || s == NULL || o == NULL) {
        return;
    }
    while (*s != '\0' && *o + 1u < n) {
        out[(*o)++] = *s++;
    }
}

static void refresh_title(void)
{
    size_t o = 0u;
    const char *m = (g_month >= 1u && g_month <= 12u) ? k_months[g_month - 1u] : "?";

    copy_str(g_title, sizeof(g_title), m, &o);
    if (o + 1u < sizeof(g_title)) {
        g_title[o++] = ' ';
    }
    put_u16(g_title, sizeof(g_title), g_year, &o);
    g_title[o] = '\0';
}

static void two(char *out, uint8_t v)
{
    out[0] = (char)('0' + ((v / 10u) % 10u));
    out[1] = (char)('0' + (v % 10u));
}

void calendar_open(void)
{
    calendar_go_today();
}

void calendar_close(void)
{
}

void calendar_prev_month(void)
{
    if (g_month <= 1u) {
        g_month = 12u;
        if (g_year > 2000u) {
            g_year--;
        }
    } else {
        g_month--;
    }
    refresh_title();
    bump();
}

void calendar_next_month(void)
{
    if (g_month >= 12u) {
        g_month = 1u;
        if (g_year < 2099u) {
            g_year++;
        }
    } else {
        g_month++;
    }
    refresh_title();
    bump();
}

void calendar_go_today(void)
{
    time_civil_t n;

    if (time_now(&n) == ERR_OK && n.year >= 2000u && n.month >= 1u && n.month <= 12u) {
        g_year = n.year;
        g_month = n.month;
    } else {
        g_year = 2026u;
        g_month = 1u;
    }
    refresh_title();
    bump();
}

uint32_t calendar_gen(void)
{
    return g_gen;
}

const char *calendar_title(void)
{
    if (g_title[0] == '\0') {
        refresh_title();
    }
    return g_title;
}

const char *calendar_clock(void)
{
    time_civil_t n;

    if (time_now(&n) != ERR_OK) {
        n.hour = 0u;
        n.min = 0u;
        n.sec = 0u;
    }
    two(&g_clock[0], n.hour);
    g_clock[2] = ':';
    two(&g_clock[3], n.min);
    g_clock[5] = ':';
    two(&g_clock[6], n.sec);
    g_clock[8] = '\0';
    return g_clock;
}

const char *calendar_date_line(void)
{
    time_civil_t n;
    size_t o = 0u;
    const char *w;
    const char *m;

    if (time_now(&n) != ERR_OK) {
        n.wday = 0u;
        n.day = 1u;
        n.month = 1u;
        n.year = 2026u;
    }
    w = (n.wday < 7u) ? k_wdays[n.wday] : "---";
    m = (n.month >= 1u && n.month <= 12u) ? k_months[n.month - 1u] : "?";
    copy_str(g_date, sizeof(g_date), w, &o);
    if (o + 1u < sizeof(g_date)) {
        g_date[o++] = ' ';
    }
    put_u16(g_date, sizeof(g_date), n.day, &o);
    if (o + 1u < sizeof(g_date)) {
        g_date[o++] = ' ';
    }
    copy_str(g_date, sizeof(g_date), m, &o);
    if (o + 1u < sizeof(g_date)) {
        g_date[o++] = ' ';
    }
    put_u16(g_date, sizeof(g_date), n.year, &o);
    g_date[o] = '\0';
    return g_date;
}

const char *calendar_ntp_line(void)
{
    return time_ntp_str();
}

uint8_t calendar_cell_day(unsigned index)
{
    uint8_t first;
    uint8_t md;
    unsigned d;

    if (index >= CALENDAR_CELLS) {
        return 0u;
    }
    first = time_weekday(g_year, g_month, 1u);
    md = time_month_days(g_year, g_month);
    if (index < (unsigned)first) {
        return 0u;
    }
    d = index - (unsigned)first + 1u;
    if (d > (unsigned)md) {
        return 0u;
    }
    return (uint8_t)d;
}

uint8_t calendar_cell_today(unsigned index)
{
    time_civil_t n;
    uint8_t day = calendar_cell_day(index);

    if (day == 0u || time_now(&n) != ERR_OK) {
        return 0u;
    }
    if (n.year == g_year && n.month == g_month && n.day == day) {
        return 1u;
    }
    return 0u;
}
