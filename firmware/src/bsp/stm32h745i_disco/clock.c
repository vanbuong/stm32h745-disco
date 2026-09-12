#include "bsp/board.h"

#include "cube.h"

static uint32_t g_sysclk_hz = BOARD_HSI_HZ;
static uint32_t g_pclk1_hz = BOARD_HSI_HZ;

uint32_t board_sysclk_hz(void)
{
    return g_sysclk_hz;
}

uint32_t board_pclk1_hz(void)
{
    return g_pclk1_hz;
}

static err_t enable_hse(void)
{
    RCC_OscInitTypeDef osc = {0};

    osc.OscillatorType = RCC_OSCILLATORTYPE_HSE;
    osc.HSEState = RCC_HSE_ON;
    osc.PLL.PLLState = RCC_PLL_NONE;
    if (HAL_RCC_OscConfig(&osc) != HAL_OK) {
        return ERR_IO;
    }
    /* PLL3 (LTDC) refuses to start unless PLLCKSELR already names a source. */
    __HAL_RCC_PLL_PLLSOURCE_CONFIG(RCC_PLLSOURCE_HSE);
    return ERR_OK;
}

err_t board_clock_init(void)
{
    RCC_OscInitTypeDef osc = {0};
    RCC_ClkInitTypeDef clk = {0};
    uint32_t plln = 192u;
    uint8_t vos0 = 0u;

    __HAL_RCC_SYSCFG_CLK_ENABLE();

    /*
     * CR3 locks after the first supply write (ExitRun0Mode, or a previous
     * DIRECT_SMPS debug session). A hard fail here used to skip HSE/PLL, so
     * sysclk stayed 64 MHz and LTDC PLL3 had no source (white panel).
     */
    (void)HAL_PWREx_ConfigSupply(PWR_SMPS_1V8_SUPPLIES_LDO);
    if (((PWR->CR3 & PWR_CR3_LDOEN) != 0U) &&
        (HAL_PWREx_ControlVoltageScaling(PWR_REGULATOR_VOLTAGE_SCALE0) == HAL_OK)) {
        vos0 = 1u;
    } else if (HAL_PWREx_ControlVoltageScaling(PWR_REGULATOR_VOLTAGE_SCALE1) != HAL_OK) {
        (void)enable_hse();
        return ERR_IO;
    }
    if (vos0 == 0u) {
        /* VOS1 max is 400 MHz: 25 MHz / 5 * 160 / 2. */
        plln = 160u;
    }

    osc.OscillatorType = RCC_OSCILLATORTYPE_HSE;
    osc.HSEState = RCC_HSE_ON;
    osc.PLL.PLLState = RCC_PLL_ON;
    osc.PLL.PLLSource = RCC_PLLSOURCE_HSE;
    osc.PLL.PLLM = 5;
    osc.PLL.PLLN = plln;
    osc.PLL.PLLP = 2;
    osc.PLL.PLLQ = 4;
    osc.PLL.PLLR = 2;
    osc.PLL.PLLRGE = RCC_PLL1VCIRANGE_2;
    osc.PLL.PLLVCOSEL = RCC_PLL1VCOWIDE;
    osc.PLL.PLLFRACN = 0;
    if (HAL_RCC_OscConfig(&osc) != HAL_OK) {
        (void)enable_hse();
        return ERR_TIMEOUT;
    }

    clk.ClockType = RCC_CLOCKTYPE_HCLK | RCC_CLOCKTYPE_SYSCLK | RCC_CLOCKTYPE_PCLK1 |
                    RCC_CLOCKTYPE_PCLK2 | RCC_CLOCKTYPE_D3PCLK1 | RCC_CLOCKTYPE_D1PCLK1;
    clk.SYSCLKSource = RCC_SYSCLKSOURCE_PLLCLK;
    clk.SYSCLKDivider = RCC_SYSCLK_DIV1;
    clk.AHBCLKDivider = RCC_HCLK_DIV2;
    clk.APB3CLKDivider = RCC_APB3_DIV2;
    clk.APB1CLKDivider = RCC_APB1_DIV2;
    clk.APB2CLKDivider = RCC_APB2_DIV2;
    clk.APB4CLKDivider = RCC_APB4_DIV2;
    if (HAL_RCC_ClockConfig(&clk, FLASH_LATENCY_4) != HAL_OK) {
        (void)enable_hse();
        return ERR_TIMEOUT;
    }

    g_sysclk_hz = HAL_RCC_GetSysClockFreq();
    g_pclk1_hz = HAL_RCC_GetPCLK1Freq();
    __HAL_RCC_D2SRAM1_CLK_ENABLE();
    __HAL_RCC_D2SRAM2_CLK_ENABLE();
    __HAL_RCC_D2SRAM3_CLK_ENABLE();
    return ERR_OK;
}

uint32_t board_millis(void)
{
    return HAL_GetTick();
}

void board_cm4_boot(void)
{
    HAL_RCCEx_EnableBootCore(RCC_BOOT_C2);
}
