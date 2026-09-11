#include <stddef.h>
#include <stdint.h>

#define M4_HEAP_BYTES (48u * 1024u)

static uint8_t g_heap[M4_HEAP_BYTES] __attribute__((aligned(8)));
static uintptr_t g_off;

void *_sbrk(ptrdiff_t incr)
{
    uintptr_t next = g_off + (uintptr_t)incr;

    if (next > M4_HEAP_BYTES) {
        return (void *)(intptr_t)-1;
    }
    {
        void *p = &g_heap[g_off];
        g_off = next;
        return p;
    }
}
