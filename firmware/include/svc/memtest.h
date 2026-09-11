#ifndef MEMTEST_H
#define MEMTEST_H

#include "err.h"

#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

err_t memtest_walking(volatile uint32_t *base, size_t words, uint32_t *fail_off);

#ifdef __cplusplus
}
#endif

#endif /* MEMTEST_H */
