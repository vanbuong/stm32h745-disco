#include "svc/memtest.h"

err_t memtest_walking(volatile uint32_t *base, size_t words, uint32_t *fail_off)
{
    size_t i;
    unsigned b;

    if (base == NULL || words == 0u) {
        return ERR_INVAL;
    }
    if (fail_off != NULL) {
        *fail_off = 0;
    }

    for (b = 0; b < 32u; b++) {
        uint32_t pat = 1u << b;
        base[0] = pat;
        if (base[0] != pat) {
            if (fail_off != NULL) {
                *fail_off = 0;
            }
            return ERR_IO;
        }
    }

    for (i = 0; i < words; i++) {
        base[i] = (uint32_t)i;
    }
    for (i = 0; i < words; i++) {
        if (base[i] != (uint32_t)i) {
            if (fail_off != NULL) {
                *fail_off = (uint32_t)(i * 4u);
            }
            return ERR_IO;
        }
    }
    for (i = 0; i < words; i++) {
        base[i] = ~(uint32_t)i;
    }
    for (i = 0; i < words; i++) {
        if (base[i] != ~(uint32_t)i) {
            if (fail_off != NULL) {
                *fail_off = (uint32_t)(i * 4u);
            }
            return ERR_IO;
        }
    }
    return ERR_OK;
}
