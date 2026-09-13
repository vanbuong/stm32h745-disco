#include "svc/time.h"

#include <stdint.h>
#include <string.h>

static uint32_t civil_to_unix(const time_civil_t *c)
{
    uint32_t y;
    uint32_t era;
    uint32_t yoe;
    uint32_t doy;
    uint32_t doe;
    uint32_t days;

    if (c == NULL || c->year < 1970u || c->month < 1u || c->month > 12u) {
        return 0u;
    }
    y = (uint32_t)c->year;
    y -= (c->month <= 2u) ? 1u : 0u;
    era = y / 400u;
    yoe = y - era * 400u;
    doy = (153u * ((c->month > 2u) ? ((uint32_t)c->month - 3u) : ((uint32_t)c->month + 9u)) + 2u) /
              5u +
          (uint32_t)c->day - 1u;
    doe = yoe * 365u + yoe / 4u - yoe / 100u + doy;
    days = era * 146097u + doe - 719468u;
    return days * 86400u + (uint32_t)c->hour * 3600u + (uint32_t)c->min * 60u + (uint32_t)c->sec;
}

uint64_t get_utc_epoch_time(void)
{
    time_civil_t c;

    memset(&c, 0, sizeof(c));
    if (time_now(&c) != ERR_OK) {
        return 0;
    }
    return (uint64_t)civil_to_unix(&c);
}
