#include "hal/wdog.h"

#if defined(CORE_CM7)
#include "stm32h7xx_hal.h"
#endif

#if defined(CORE_CM4)
#include "stm32h7xx.h"
#endif

#if defined(CORE_CM7)
static IWDG_HandleTypeDef g_iwdg;
#endif
static uint8_t g_on;

err_t wdog_start(void)
{
#if defined(CORE_CM7)
    uint32_t t0;

    /*
     * LSI ~32 kHz / 256, reload 2047 ≈ 16 s. Start only after bring-up so
     * vfs_bench / memtest cannot trip it. Window disabled (no IRQ).
     */
    __HAL_RCC_LSI_ENABLE();
    t0 = HAL_GetTick();
    while (__HAL_RCC_GET_FLAG(RCC_FLAG_LSIRDY) == 0u) {
        if ((HAL_GetTick() - t0) > 20u) {
            break;
        }
    }
    g_iwdg.Instance = IWDG1;
    g_iwdg.Init.Prescaler = IWDG_PRESCALER_256;
    g_iwdg.Init.Reload = 2047u;
    g_iwdg.Init.Window = IWDG_WINDOW_DISABLE;
    if (HAL_IWDG_Init(&g_iwdg) != HAL_OK) {
        /* HAL starts the counter before waiting on SR. Keep kicking. */
        g_on = 1u;
        (void)HAL_IWDG_Refresh(&g_iwdg);
        return ERR_IO;
    }
    g_on = 1u;
    return ERR_OK;
#else
    return ERR_OK;
#endif
}

void wdog_kick(void)
{
#if defined(CORE_CM7)
    if (g_on != 0u) {
        (void)HAL_IWDG_Refresh(&g_iwdg);
    }
#elif defined(CORE_CM4)
    /* Refresh is ignored until M7 writes the start key. */
    IWDG1->KR = 0x0000AAAAu;
#else
    (void)g_on;
#endif
}

uint8_t wdog_started(void)
{
    return g_on;
}
