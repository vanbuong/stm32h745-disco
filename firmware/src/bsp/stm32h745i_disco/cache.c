#include "bsp/board.h"

#include "stm32h745_regs.h"

static void dcache_set_way(volatile uint32_t *reg)
{
    uint32_t ccsidr;
    uint32_t sets;
    uint32_t ways;
    uint32_t set;
    uint32_t way;

    SCB_CSSELR = 0;
    dsb();
    ccsidr = SCB_CCSIDR;
    sets = (ccsidr >> 13) & 0x7FFFu;
    ways = (ccsidr >> 3) & 0x3FFu;

    for (set = 0; set <= sets; set++) {
        for (way = 0; way <= ways; way++) {
            *reg = (way << 30) | (set << 5);
        }
    }
    dsb();
    isb();
}

void board_cache_invalidate_d(void)
{
    dcache_set_way(&SCB_DCISW);
}

void board_cache_d_enable(void)
{
    if ((SCB_CCR & SCB_CCR_DC) != 0u) {
        return;
    }
    board_cache_invalidate_d();
    SCB_CCR |= SCB_CCR_DC;
    dsb();
    isb();
}

void board_cache_d_disable(void)
{
    if ((SCB_CCR & SCB_CCR_DC) == 0u) {
        return;
    }
    dcache_set_way(&SCB_DCCISW);
    SCB_CCR &= ~SCB_CCR_DC;
    dsb();
    isb();
}

void board_cache_init(void)
{
    dsb();
    isb();
    SCB_ICIALLU = 0;
    dsb();
    isb();
    SCB_CCR |= SCB_CCR_IC;
    dsb();
    isb();
    board_cache_d_enable();
}
