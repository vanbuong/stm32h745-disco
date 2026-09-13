#include "bsp/board.h"

#include <stddef.h>
#include <stdint.h>

static uint8_t g_heap[BOARD_ZB_HEAP_BYTES] __attribute__((section(".axi_heap"), aligned(8)));
static uintptr_t g_off;

void *_sbrk(ptrdiff_t incr)
{
    uintptr_t next = g_off + (uintptr_t)incr;

    if (next > BOARD_ZB_HEAP_BYTES) {
        return (void *)(intptr_t)-1;
    }
    {
        void *p = &g_heap[g_off];
        g_off = next;
        return p;
    }
}
