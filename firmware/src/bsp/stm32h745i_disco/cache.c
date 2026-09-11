#include "bsp/board.h"

#include "cube.h"

void board_cache_invalidate_d(void)
{
    SCB_InvalidateDCache();
}

void board_cache_d_enable(void)
{
    SCB_EnableDCache();
}

void board_cache_d_disable(void)
{
    SCB_DisableDCache();
}

void board_cache_init(void)
{
    SCB_EnableICache();
    SCB_EnableDCache();
}
