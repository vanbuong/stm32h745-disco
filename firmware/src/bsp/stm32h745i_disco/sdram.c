#include "bsp/board.h"

#include "stm32h745_regs.h"

/*
 * GPIO MODER/AFR and FMC SDCR/SDTR values are the STM32H745I-DISCO numbers from
 * ST CubeH7 SystemInit_ExtMemCtl (STMicroelectronics, BSD-3). QSPI pins on the
 * same ports are programmed after this init.
 */

static void delay_loops(uint32_t n)
{
    volatile uint32_t i = n;
    while (i > 0u) {
        i--;
    }
}

static err_t fmc_wait(void)
{
    uint32_t t = 0xFFFFu;
    while ((FMC_SDSR & FMC_SDSR_BUSY) != 0u) {
        if (t == 0u) {
            return ERR_TIMEOUT;
        }
        t--;
    }
    return ERR_OK;
}

err_t board_sdram_init(void)
{
    err_t e;
    volatile uint32_t dummy;

    RCC_AHB4ENR |= 0x000001F8u; /* GPIOD–I */
    dummy = RCC_AHB4ENR;
    (void)dummy;

    GPIO_AFR(GPIOD_BASE, 0) = 0x000000CCu;
    GPIO_AFR(GPIOD_BASE, 1) = 0xCC000CCCu;
    GPIO_MODER(GPIOD_BASE) = 0xAFEAFFFAu;
    GPIO_OSPEEDR(GPIOD_BASE) = 0xF03F000Fu;
    GPIO_OTYPER(GPIOD_BASE) = 0;
    GPIO_PUPDR(GPIOD_BASE) = 0x50150005u;

    GPIO_AFR(GPIOE_BASE, 0) = 0xC00000CCu;
    GPIO_AFR(GPIOE_BASE, 1) = 0xCCCCCCCCu;
    GPIO_MODER(GPIOE_BASE) = 0xAAAABFFAu;
    GPIO_OSPEEDR(GPIOE_BASE) = 0xFFFFC00Fu;
    GPIO_OTYPER(GPIOE_BASE) = 0;
    GPIO_PUPDR(GPIOE_BASE) = 0x55554005u;

    GPIO_AFR(GPIOF_BASE, 0) = 0x00CCCCCCu;
    GPIO_AFR(GPIOF_BASE, 1) = 0xCCCCC000u;
    GPIO_MODER(GPIOF_BASE) = 0xAABFFFAAu;
    GPIO_OSPEEDR(GPIOF_BASE) = 0xFFC00FFFu;
    GPIO_OTYPER(GPIOF_BASE) = 0;
    GPIO_PUPDR(GPIOF_BASE) = 0x55400555u;

    GPIO_AFR(GPIOG_BASE, 0) = 0x00CC00CCu;
    GPIO_AFR(GPIOG_BASE, 1) = 0xC000000Cu;
    GPIO_MODER(GPIOG_BASE) = 0xBFFEFAFAu;
    GPIO_OSPEEDR(GPIOG_BASE) = 0xC0030F0Fu;
    GPIO_OTYPER(GPIOG_BASE) = 0;
    GPIO_PUPDR(GPIOG_BASE) = 0x40010505u;

    GPIO_AFR(GPIOH_BASE, 0) = 0xCCC00000u;
    GPIO_AFR(GPIOH_BASE, 1) = 0xCCCCCCCCu;
    GPIO_MODER(GPIOH_BASE) = 0xAAAAABFFu;
    GPIO_OSPEEDR(GPIOH_BASE) = 0xFFFFFC00u;
    GPIO_OTYPER(GPIOH_BASE) = 0;
    GPIO_PUPDR(GPIOH_BASE) = 0x55555400u;

    RCC_AHB3ENR |= RCC_AHB3ENR_FMCEN;
    (void)RCC_AHB3ENR;
    FMC_BTCR0 = 0x000030D2u;

    /* Bank2, 16-bit, 8 col, 12 row, CAS2, SDCLK = HCLK/2, burst, RPIPE=0. */
    FMC_SDCR1 = 0x00001800u;
    FMC_SDCR2 = 0x00000154u;
    FMC_SDTR1 = 0x00105000u;
    FMC_SDTR2 = 0x01010351u;

    FMC_SDCMR = 0x00000009u; /* clock enable, CTB2 */
    e = fmc_wait();
    if (e != ERR_OK) {
        return e;
    }
    delay_loops(200000u);

    FMC_SDCMR = 0x0000000Au; /* PALL */
    e = fmc_wait();
    if (e != ERR_OK) {
        return e;
    }

    FMC_SDCMR = 0x000000EBu; /* 8 auto-refresh */
    e = fmc_wait();
    if (e != ERR_OK) {
        return e;
    }

    FMC_SDCMR = 0x0004400Cu; /* load mode, CAS2 */
    e = fmc_wait();
    if (e != ERR_OK) {
        return e;
    }

    /* 64 ms / 4096 rows at 120 MHz SDCLK → COUNT = 1875. */
    FMC_SDRTR = (FMC_SDRTR & ~0x3FFEu) | (1875u << 1);
    FMC_SDCR2 &= ~0x00000200u; /* write protect off */
    FMC_BTCR0 |= FMC_BCR1_FMCEN;
    return ERR_OK;
}
