#include "hal/uart.h"

/*
 * Sprint 11: no USART1 IRQ and no HAL UART driver.
 * ZNP stays closed so zb_host runs the mock / last-known list.
 * USART3 remains the console and is never opened here.
 */
err_t uart_open(uart_id_t id, const uart_cfg_t *cfg)
{
    (void)cfg;
    if (id == UART_ID_ZNP) {
        return ERR_IO;
    }
    return ERR_UNSUPPORTED;
}

err_t uart_write(uart_id_t id, const void *data, size_t n)
{
    (void)id;
    (void)data;
    (void)n;
    return ERR_IO;
}

err_t uart_read(uart_id_t id, void *data, size_t n, size_t *got, uint32_t timeout_ms)
{
    (void)id;
    (void)data;
    (void)n;
    (void)timeout_ms;
    if (got != NULL) {
        *got = 0u;
    }
    return ERR_IO;
}

err_t uart_set_gpio(uart_id_t id, uint8_t pin_id, int level)
{
    (void)id;
    (void)pin_id;
    (void)level;
    return ERR_IO;
}
