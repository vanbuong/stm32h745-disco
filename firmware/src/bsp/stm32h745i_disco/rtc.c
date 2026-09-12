#include "bsp/board.h"

#include "cube.h"

static RTC_HandleTypeDef g_rtc;
static uint8_t g_ready;

void HAL_RTC_MspInit(RTC_HandleTypeDef *hrtc)
{
    (void)hrtc;
    HAL_PWR_EnableBkUpAccess();
    __HAL_RCC_RTC_ENABLE();
}

static err_t rtc_clock_src(void)
{
    RCC_OscInitTypeDef osc = {0};
    RCC_PeriphCLKInitTypeDef p = {0};

    HAL_PWR_EnableBkUpAccess();

    osc.OscillatorType = RCC_OSCILLATORTYPE_LSE;
    osc.LSEState = RCC_LSE_ON;
    osc.PLL.PLLState = RCC_PLL_NONE;
    if (HAL_RCC_OscConfig(&osc) != HAL_OK) {
        osc.OscillatorType = RCC_OSCILLATORTYPE_LSI;
        osc.LSIState = RCC_LSI_ON;
        osc.LSEState = RCC_LSE_OFF;
        if (HAL_RCC_OscConfig(&osc) != HAL_OK) {
            return ERR_TIMEOUT;
        }
        p.PeriphClockSelection = RCC_PERIPHCLK_RTC;
        p.RTCClockSelection = RCC_RTCCLKSOURCE_LSI;
    } else {
        p.PeriphClockSelection = RCC_PERIPHCLK_RTC;
        p.RTCClockSelection = RCC_RTCCLKSOURCE_LSE;
    }
    if (HAL_RCCEx_PeriphCLKConfig(&p) != HAL_OK) {
        return ERR_IO;
    }
    __HAL_RCC_RTC_ENABLE();
    return ERR_OK;
}

err_t board_rtc_init(void)
{
    RTC_TimeTypeDef t = {0};
    RTC_DateTypeDef d = {0};
    err_t e;

    e = rtc_clock_src();
    if (e != ERR_OK) {
        return e;
    }

    g_rtc.Instance = RTC;
    g_rtc.Init.HourFormat = RTC_HOURFORMAT_24;
    g_rtc.Init.AsynchPrediv = 127;
    g_rtc.Init.SynchPrediv = 255;
    g_rtc.Init.OutPut = RTC_OUTPUT_DISABLE;
    g_rtc.Init.OutPutPolarity = RTC_OUTPUT_POLARITY_HIGH;
    g_rtc.Init.OutPutType = RTC_OUTPUT_TYPE_OPENDRAIN;
    g_rtc.Init.OutPutRemap = RTC_OUTPUT_REMAP_NONE;
    if (HAL_RTC_Init(&g_rtc) != HAL_OK) {
        return ERR_IO;
    }

    if (HAL_RTCEx_BKUPRead(&g_rtc, RTC_BKP_DR0) != 0x32F2u) {
        t.Hours = 0;
        t.Minutes = 0;
        t.Seconds = 0;
        t.DayLightSaving = RTC_DAYLIGHTSAVING_NONE;
        t.StoreOperation = RTC_STOREOPERATION_RESET;
        d.WeekDay = RTC_WEEKDAY_MONDAY;
        d.Month = RTC_MONTH_JANUARY;
        d.Date = 1;
        d.Year = 26;
        if (HAL_RTC_SetTime(&g_rtc, &t, RTC_FORMAT_BIN) != HAL_OK) {
            return ERR_IO;
        }
        if (HAL_RTC_SetDate(&g_rtc, &d, RTC_FORMAT_BIN) != HAL_OK) {
            return ERR_IO;
        }
        HAL_RTCEx_BKUPWrite(&g_rtc, RTC_BKP_DR0, 0x32F2u);
    }
    g_ready = 1u;
    return ERR_OK;
}

err_t board_rtc_get(uint8_t *hh, uint8_t *mm, uint8_t *ss)
{
    RTC_TimeTypeDef t = {0};
    RTC_DateTypeDef d = {0};

    if (hh == NULL || mm == NULL || ss == NULL) {
        return ERR_INVAL;
    }
    if (g_ready == 0u) {
        *hh = 0u;
        *mm = 0u;
        *ss = 0u;
        return ERR_IO;
    }
    if (HAL_RTC_GetTime(&g_rtc, &t, RTC_FORMAT_BIN) != HAL_OK) {
        return ERR_IO;
    }
    if (HAL_RTC_GetDate(&g_rtc, &d, RTC_FORMAT_BIN) != HAL_OK) {
        return ERR_IO;
    }
    *hh = (uint8_t)t.Hours;
    *mm = (uint8_t)t.Minutes;
    *ss = (uint8_t)t.Seconds;
    return ERR_OK;
}

err_t board_rtc_set(uint8_t hh, uint8_t mm, uint8_t ss)
{
    RTC_TimeTypeDef t = {0};

    if (g_ready == 0u || hh > 23u || mm > 59u || ss > 59u) {
        return ERR_INVAL;
    }
    t.Hours = hh;
    t.Minutes = mm;
    t.Seconds = ss;
    t.DayLightSaving = RTC_DAYLIGHTSAVING_NONE;
    t.StoreOperation = RTC_STOREOPERATION_RESET;
    if (HAL_RTC_SetTime(&g_rtc, &t, RTC_FORMAT_BIN) != HAL_OK) {
        return ERR_IO;
    }
    return ERR_OK;
}

err_t board_rtc_get_date(uint16_t *year, uint8_t *month, uint8_t *day, uint8_t *hh, uint8_t *mm,
                         uint8_t *ss)
{
    RTC_TimeTypeDef t = {0};
    RTC_DateTypeDef d = {0};

    if (year == NULL || month == NULL || day == NULL || hh == NULL || mm == NULL || ss == NULL) {
        return ERR_INVAL;
    }
    if (g_ready == 0u) {
        *year = 2026u;
        *month = 1u;
        *day = 1u;
        *hh = 0u;
        *mm = 0u;
        *ss = 0u;
        return ERR_IO;
    }
    if (HAL_RTC_GetTime(&g_rtc, &t, RTC_FORMAT_BIN) != HAL_OK) {
        return ERR_IO;
    }
    if (HAL_RTC_GetDate(&g_rtc, &d, RTC_FORMAT_BIN) != HAL_OK) {
        return ERR_IO;
    }
    *hh = (uint8_t)t.Hours;
    *mm = (uint8_t)t.Minutes;
    *ss = (uint8_t)t.Seconds;
    *month = (uint8_t)d.Month;
    *day = (uint8_t)d.Date;
    *year = (uint16_t)(2000u + d.Year);
    return ERR_OK;
}

err_t board_rtc_set_date(uint16_t year, uint8_t month, uint8_t day, uint8_t hh, uint8_t mm,
                         uint8_t ss)
{
    RTC_TimeTypeDef t = {0};
    RTC_DateTypeDef d = {0};
    uint16_t y;

    if (g_ready == 0u || month < 1u || month > 12u || day < 1u || hh > 23u || mm > 59u ||
        ss > 59u) {
        return ERR_INVAL;
    }
    y = (year >= 2000u) ? (uint16_t)(year - 2000u) : year;
    if (y > 99u) {
        y = 99u;
    }
    t.Hours = hh;
    t.Minutes = mm;
    t.Seconds = ss;
    t.DayLightSaving = RTC_DAYLIGHTSAVING_NONE;
    t.StoreOperation = RTC_STOREOPERATION_RESET;
    d.Month = month;
    d.Date = day;
    d.Year = (uint8_t)y;
    d.WeekDay = RTC_WEEKDAY_MONDAY;
    if (HAL_RTC_SetTime(&g_rtc, &t, RTC_FORMAT_BIN) != HAL_OK) {
        return ERR_IO;
    }
    if (HAL_RTC_SetDate(&g_rtc, &d, RTC_FORMAT_BIN) != HAL_OK) {
        return ERR_IO;
    }
    return ERR_OK;
}
