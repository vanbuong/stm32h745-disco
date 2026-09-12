#include "bsp/board.h"

#include "stm32h7xx.h"

void board_hsem_init(void)
{
    SET_BIT(RCC->AHB4ENR, RCC_AHB4ENR_HSEMEN);
    (void)RCC->AHB4ENR;
#if defined(CORE_CM7)
    HSEM->C1IER |= (1uL << BOARD_HSEM_M4_TO_M7);
#else
    HSEM->C2IER |= (1uL << BOARD_HSEM_M7_TO_M4);
#endif
}

void board_hsem_notify(uint32_t sem)
{
    if (sem > 31u) {
        return;
    }
    (void)HSEM->RLR[sem];
    HSEM->R[sem] = HSEM_CR_COREID_CURRENT;
}

void board_hsem_wake(uint32_t sem)
{
    if (sem > 31u) {
        return;
    }
    /* Take then release so the peer WFE/STOP sees an HSEM event. */
    HSEM->R[sem] = HSEM_CR_COREID_CURRENT | HSEM_R_LOCK;
    HSEM->R[sem] = HSEM_CR_COREID_CURRENT;
}

uint8_t board_hsem_poll(uint32_t sem)
{
    uint32_t mask;

    if (sem > 31u) {
        return 0u;
    }
    mask = 1uL << sem;
#if defined(CORE_CM7)
    if ((HSEM->C1ISR & mask) == 0u) {
        return 0u;
    }
    HSEM->C1ICR = mask;
#else
    if ((HSEM->C2ISR & mask) == 0u) {
        return 0u;
    }
    HSEM->C2ICR = mask;
#endif
    return 1u;
}
