#include "bsp/board.h"

#include "cube.h"

static SDRAM_HandleTypeDef g_sdram;

static err_t sdram_cmd(uint32_t mode, uint32_t refresh, uint32_t mrd)
{
    FMC_SDRAM_CommandTypeDef c = {0};

    c.CommandMode = mode;
    c.CommandTarget = FMC_SDRAM_CMD_TARGET_BANK2;
    c.AutoRefreshNumber = refresh;
    c.ModeRegisterDefinition = mrd;
    return cube_err(HAL_SDRAM_SendCommand(&g_sdram, &c, 0xFFFFu));
}

void HAL_SDRAM_MspInit(SDRAM_HandleTypeDef *hsdram)
{
    GPIO_InitTypeDef g = {0};

    (void)hsdram;
    __HAL_RCC_FMC_CLK_ENABLE();
    __HAL_RCC_GPIOD_CLK_ENABLE();
    __HAL_RCC_GPIOE_CLK_ENABLE();
    __HAL_RCC_GPIOF_CLK_ENABLE();
    __HAL_RCC_GPIOG_CLK_ENABLE();
    __HAL_RCC_GPIOH_CLK_ENABLE();

    g.Mode = GPIO_MODE_AF_PP;
    g.Pull = GPIO_PULLUP;
    g.Speed = GPIO_SPEED_FREQ_VERY_HIGH;
    g.Alternate = GPIO_AF12_FMC;

    g.Pin =
        GPIO_PIN_0 | GPIO_PIN_1 | GPIO_PIN_8 | GPIO_PIN_9 | GPIO_PIN_10 | GPIO_PIN_14 | GPIO_PIN_15;
    HAL_GPIO_Init(GPIOD, &g);

    g.Pin = GPIO_PIN_0 | GPIO_PIN_1 | GPIO_PIN_7 | GPIO_PIN_8 | GPIO_PIN_9 | GPIO_PIN_10 |
            GPIO_PIN_11 | GPIO_PIN_12 | GPIO_PIN_13 | GPIO_PIN_14 | GPIO_PIN_15;
    HAL_GPIO_Init(GPIOE, &g);

    g.Pin = GPIO_PIN_0 | GPIO_PIN_1 | GPIO_PIN_2 | GPIO_PIN_3 | GPIO_PIN_4 | GPIO_PIN_5 |
            GPIO_PIN_11 | GPIO_PIN_12 | GPIO_PIN_13 | GPIO_PIN_14 | GPIO_PIN_15;
    HAL_GPIO_Init(GPIOF, &g);

    g.Pin = GPIO_PIN_0 | GPIO_PIN_1 | GPIO_PIN_4 | GPIO_PIN_5 | GPIO_PIN_8 | GPIO_PIN_15;
    HAL_GPIO_Init(GPIOG, &g);

    g.Pin = GPIO_PIN_5 | GPIO_PIN_6 | GPIO_PIN_7;
    HAL_GPIO_Init(GPIOH, &g);
}

err_t board_sdram_init(void)
{
    FMC_SDRAM_TimingTypeDef t = {0};

    g_sdram.Instance = FMC_SDRAM_DEVICE;
    g_sdram.Init.SDBank = FMC_SDRAM_BANK2;
    /* 16-bit bus × 12 × 8 × 4 = 8 MB. 9 col remaps CPU/LTDC addresses and
     * breaks the panel; do not use it to “get” the other 8 MB. */
    g_sdram.Init.ColumnBitsNumber = FMC_SDRAM_COLUMN_BITS_NUM_8;
    g_sdram.Init.RowBitsNumber = FMC_SDRAM_ROW_BITS_NUM_12;
    g_sdram.Init.MemoryDataWidth = FMC_SDRAM_MEM_BUS_WIDTH_16;
    g_sdram.Init.InternalBankNumber = FMC_SDRAM_INTERN_BANKS_NUM_4;
    /* 100 MHz SDCLK (AHB 200 MHz / 2). CAS2 + tRC=6 is too tight; ST Cube
     * examples that pass a walking test use CAS3 and 0x603 refresh. */
    g_sdram.Init.CASLatency = FMC_SDRAM_CAS_LATENCY_3;
    g_sdram.Init.WriteProtection = FMC_SDRAM_WRITE_PROTECTION_DISABLE;
    g_sdram.Init.SDClockPeriod = FMC_SDRAM_CLOCK_PERIOD_2;
    g_sdram.Init.ReadBurst = FMC_SDRAM_RBURST_ENABLE;
    g_sdram.Init.ReadPipeDelay = FMC_SDRAM_RPIPE_DELAY_0;

    t.LoadToActiveDelay = 2;
    t.ExitSelfRefreshDelay = 7;
    t.SelfRefreshTime = 4;
    t.RowCycleDelay = 7;
    t.WriteRecoveryTime = 2;
    t.RPDelay = 2;
    t.RCDDelay = 2;

    if (HAL_SDRAM_Init(&g_sdram, &t) != HAL_OK) {
        return ERR_IO;
    }

    if (sdram_cmd(FMC_SDRAM_CMD_CLK_ENABLE, 1, 0) != ERR_OK) {
        return ERR_IO;
    }
    HAL_Delay(1);
    if (sdram_cmd(FMC_SDRAM_CMD_PALL, 1, 0) != ERR_OK) {
        return ERR_IO;
    }
    if (sdram_cmd(FMC_SDRAM_CMD_AUTOREFRESH_MODE, 8, 0) != ERR_OK) {
        return ERR_IO;
    }
    /* Burst length 1, sequential, CAS3, single write burst. */
    if (sdram_cmd(FMC_SDRAM_CMD_LOAD_MODE, 1, 0x230u) != ERR_OK) {
        return ERR_IO;
    }
    /* 100 MHz * 64 ms / 4096 rows − 20 ≈ 0x603 (Cube H745I-DISCO). 1875 is
     * the 120 MHz (480 MHz SYSCLK) count and drops rows at 400 MHz. */
    return cube_err(HAL_SDRAM_ProgramRefreshRate(&g_sdram, 0x603u));
}
