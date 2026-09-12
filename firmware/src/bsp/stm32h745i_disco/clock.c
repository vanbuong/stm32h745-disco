#include "bsp/board.h"

#include "cube.h"

#define D2_SYNC_MS 100u

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

static void wait_vos(uint32_t ms)
{
    uint32_t t0 = HAL_GetTick();

    while (__HAL_PWR_GET_FLAG(PWR_FLAG_VOSRDY) == 0U) {
        if ((HAL_GetTick() - t0) > ms) {
            break;
        }
    }
}

static uint8_t d2_ck_ready(void)
{
    return (__HAL_RCC_GET_FLAG(RCC_FLAG_D2CKRDY) != RESET) ? 1u : 0u;
}

static void wait_d2_ck(uint8_t want_ready, uint32_t ms)
{
    uint32_t t0 = HAL_GetTick();

    while (d2_ck_ready() != want_ready) {
        if ((HAL_GetTick() - t0) > ms) {
            break;
        }
    }
}

/* CubeMX DUAL_CORE_BOOT_SYNC_SEQUENCE Boot_Mode_Sequence_2. */
static void wake_cm4(void)
{
    board_hsem_init();
    board_hsem_wake(BOARD_HSEM_M7_TO_M4);
    wait_d2_ck(1u, D2_SYNC_MS);
    if (d2_ck_ready() == 0u) {
        board_console_puts("d2 wake to\r\n");
    }
}

static err_t apply_pll(uint32_t src, uint32_t m, uint32_t n, uint32_t latency)
{
    RCC_OscInitTypeDef osc = {0};
    RCC_ClkInitTypeDef clk = {0};

    osc.PLL.PLLState = RCC_PLL_ON;
    osc.PLL.PLLSource = src;
    osc.PLL.PLLM = m;
    osc.PLL.PLLN = n;
    osc.PLL.PLLP = 2;
    osc.PLL.PLLQ = 4;
    osc.PLL.PLLR = 2;
    osc.PLL.PLLRGE = (src == RCC_PLLSOURCE_HSE) ? RCC_PLL1VCIRANGE_2 : RCC_PLL1VCIRANGE_3;
    osc.PLL.PLLVCOSEL = RCC_PLL1VCOWIDE;
    osc.PLL.PLLFRACN = 0;
    if (src == RCC_PLLSOURCE_HSE) {
        osc.OscillatorType = RCC_OSCILLATORTYPE_HSE;
        osc.HSEState = RCC_HSE_ON;
    } else {
        osc.OscillatorType = RCC_OSCILLATORTYPE_HSI;
        osc.HSIState = RCC_HSI_DIV1;
        osc.HSICalibrationValue = RCC_HSICALIBRATION_DEFAULT;
    }
    if (HAL_RCC_OscConfig(&osc) != HAL_OK) {
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
    if (HAL_RCC_ClockConfig(&clk, latency) != HAL_OK) {
        return ERR_TIMEOUT;
    }
    return ERR_OK;
}

err_t board_clock_init(void)
{
    err_t e;

    __HAL_RCC_SYSCFG_CLK_ENABLE();
    board_cm4_boot();

    /* CubeMX Boot_Mode_Sequence_1: wait until CM4 entered D2 STOP. */
    wait_d2_ck(0u, D2_SYNC_MS);
    if (d2_ck_ready() != 0u) {
        board_console_puts("d2 stop to\r\n");
    }

    /*
     * Same PWR/PLL path as the CubeMX H745-DISCO blinky. HAL_PWREx_ConfigSupply
     * and ControlVoltageScaling return HAL_ERROR when CR3 is already locked
     * (ExitRun0Mode or a previous DIRECT_SMPS session) — do not abort; PLL
     * still has to start or LTDC PLL3 has no source.
     */
    (void)HAL_PWREx_ConfigSupply(PWR_DIRECT_SMPS_SUPPLY);
    __HAL_PWR_VOLTAGESCALING_CONFIG(PWR_REGULATOR_VOLTAGE_SCALE1);
    wait_vos(50u);

    /* 25 MHz / 5 * 160 / 2 = 400 MHz (VOS1). */
    e = apply_pll(RCC_PLLSOURCE_HSE, 5u, 160u, FLASH_LATENCY_4);
    if (e != ERR_OK) {
        /* HSI / 4 * 50 / 2 = 400 MHz, same as blinky. */
        e = apply_pll(RCC_PLLSOURCE_HSI, 4u, 50u, FLASH_LATENCY_2);
    }
    if (e != ERR_OK) {
        wake_cm4();
        return e;
    }

    g_sysclk_hz = HAL_RCC_GetSysClockFreq();
    g_pclk1_hz = HAL_RCC_GetPCLK1Freq();
    __HAL_RCC_D2SRAM1_CLK_ENABLE();
    __HAL_RCC_D2SRAM2_CLK_ENABLE();
    __HAL_RCC_D2SRAM3_CLK_ENABLE();
    wake_cm4();
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
