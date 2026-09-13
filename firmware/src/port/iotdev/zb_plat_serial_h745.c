#include "platform/iotdev/zb_plat_serial.h"

#include "zb_port.h"

#include "hal/uart.h"

#include <string.h>

static zb_serial_rx_cb_t g_rx;
static uint8_t g_open;

void zb_plat_serial_open(const s_zb_serial_cfg_t *cfg, zb_serial_rx_cb_t on_rx)
{
    uart_cfg_t ucfg;

    (void)cfg;
    memset(&ucfg, 0, sizeof(ucfg));
    ucfg.baud = UART_ZNP_BAUD;
    ucfg.data_bits = 8u;
    ucfg.stop_bits = 1u;
    g_rx = on_rx;
    g_open = (uart_open(UART_ID_ZNP, &ucfg) == ERR_OK) ? 1u : 0u;
}

void zb_plat_serial_close(void)
{
    g_open = 0u;
    g_rx = NULL;
}

int zb_plat_serial_write(const uint8_t *data, uint16_t len)
{
    if (g_open == 0u) {
        return -1;
    }
    if (uart_write(UART_ID_ZNP, data, len) != ERR_OK) {
        return -1;
    }
    return (int)len;
}

void zb_plat_serial_flush_rx(void)
{
    uint8_t buf[32];
    size_t got = 0u;

    if (g_open == 0u) {
        return;
    }
    while (uart_read(UART_ID_ZNP, buf, sizeof(buf), &got, 0u) == ERR_OK && got > 0u) {
        got = 0u;
    }
}

void zb_plat_serial_poll(void)
{
    uint8_t buf[64];
    size_t got = 0u;

    if (g_open == 0u) {
        return;
    }
    if (uart_read(UART_ID_ZNP, buf, sizeof(buf), &got, 0u) == ERR_OK && got > 0u && g_rx != NULL) {
        g_rx(buf, (uint16_t)got);
    }
}

int zb_plat_serial_ok(void)
{
    return (g_open != 0u) ? 1 : 0;
}
