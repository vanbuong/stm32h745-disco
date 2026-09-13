#ifndef UART_H
#define UART_H

#include "err.h"

#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef enum { UART_ID_CONSOLE = 0, UART_ID_ZNP = 1 } uart_id_t;

#define UART_ZNP_BAUD 921600u
#define UART_ZNP_RESET_MS 6000u /* CC26xx ZNP may take this long after RESET */
#define UART_PIN_RESET 0u       /* STMOD#12 PH10, 0 = assert (active low) */
#define UART_PIN_BOOT 1u        /* STMOD#13 PA4, 1 = ZNP app; 0 during reset = SBL */

typedef struct {
    uint32_t baud;
    uint8_t data_bits;
    uint8_t stop_bits;
    uint8_t parity; /* 0 none */
} uart_cfg_t;

err_t uart_open(uart_id_t id, const uart_cfg_t *cfg);
err_t uart_write(uart_id_t id, const void *data, size_t n);
err_t uart_read(uart_id_t id, void *data, size_t n, size_t *got, uint32_t timeout_ms);
err_t uart_set_gpio(uart_id_t id, uint8_t pin_id, int level);

#ifdef __cplusplus
}
#endif

#endif /* UART_H */
