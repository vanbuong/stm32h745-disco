#include "hal/wdog.h"

static uint8_t g_on;

err_t wdog_start(void)
{
    g_on = 1u;
    return ERR_OK;
}

void wdog_kick(void)
{
}

uint8_t wdog_started(void)
{
    return g_on;
}
