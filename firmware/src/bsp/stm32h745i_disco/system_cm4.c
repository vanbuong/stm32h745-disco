#include "stm32h7xx.h"

/*
 * M4 must not run the generic CMSIS SystemInit: that file resets RCC/PLL on
 * both cores. On the disco, CM4 often comes out of reset (or C2 boot) while
 * M7 is bringing up HSE/PLL, which leaves sysclk at 64 MHz and LTDC PLL3
 * with no source.
 */

uint32_t SystemCoreClock = 64000000u;
uint32_t SystemD2Clock = 64000000u;
const uint8_t D1CorePrescTable[16] = {0, 0, 0, 0, 1, 2, 3, 4, 1, 2, 3, 4, 6, 7, 8, 9};

void SystemInit(void)
{
#if (__FPU_PRESENT == 1) && (__FPU_USED == 1)
    SCB->CPACR |= ((3UL << (10u * 2u)) | (3UL << (11u * 2u)));
#endif
    SCB->SCR |= SCB_SCR_SEVONPEND_Msk;
    SCB->VTOR = FLASH_BANK2_BASE;
    RCC->AHB2ENR |= (RCC_AHB2ENR_D2SRAM1EN | RCC_AHB2ENR_D2SRAM2EN | RCC_AHB2ENR_D2SRAM3EN);
    (void)RCC->AHB2ENR;
}

void SystemCoreClockUpdate(void)
{
}

void ExitRun0Mode(void)
{
}
