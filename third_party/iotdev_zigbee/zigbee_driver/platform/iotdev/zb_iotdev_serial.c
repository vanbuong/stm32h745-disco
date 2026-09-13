/*
 * zb_iotdev_serial.c
 *
 * iotdev/ESP backend for the board-neutral zb_plat_serial interface. Wraps the
 * iotdev_uart HAL so the protocol driver (zb_znp.c) carries no direct UART
 * dependency. Confines the iotdev_uart taxonomy (port setup struct, connection
 * struct, the HAL-shaped RX callback) to this single file. Selected for the
 * firmware build via ZB_PLATFORM_IOTDEV; host unit tests stub the interface.
 *
 * Author: Vo Van Buong (BRT-SG)
 */

#include "platform/iotdev/zb_plat_serial.h"

#if defined(ZB_PLATFORM_IOTDEV)

#include <stddef.h>

#include "platform/iotdev/zb_iotdev_peripheral.h"
#include "iotdev_uart/include/iotdev_uart.h"

/* Protocol-level framing for ZNP MT: 8N1, no flow control. Board-variable
 * wiring (port / baud / pins) is applied per zb_plat_serial_open(). */
#define ZB_SERIAL_PARITY            IOTDEV_UART_PARITY_NONE
#define ZB_SERIAL_STOPBIT           IOTDEV_UART_STOPBIT_1
#define ZB_SERIAL_DATABIT           IOTDEV_UART_DATABIT_8
#define ZB_SERIAL_FLOW_CTRL         IOTDEV_UART_NO_FLOW_CTRL
#define ZB_SERIAL_UART_BUFFER_SIZE  256

/* Pre-allocated UART RX buffer handed to the HAL. */
static uint8_t g_uart_buf[ZB_SERIAL_UART_BUFFER_SIZE] = { 0 };

static s_iotdev_uart_setup_t g_uart_setup =
{
    .parity = ZB_SERIAL_PARITY,
    .stopbit = ZB_SERIAL_STOPBIT,
    .databit = ZB_SERIAL_DATABIT,
    .flow_ctrl = ZB_SERIAL_FLOW_CTRL,

    .rx_max_buffer = ZB_SERIAL_UART_BUFFER_SIZE,
    .tx_max_buffer = ZB_SERIAL_UART_BUFFER_SIZE,
    .mode = IOTDEV_UART_DEFAULT_MODE,
    .rs485_echo_tchar_timeout = 0,
};

static zb_serial_rx_cb_t g_on_rx = NULL;

/* iotdev_uart delivers (uart_setup, data, len); adapt to the neutral sink. */
static int16_t
serial_rx_trampoline(s_iotdev_uart_setup_t *uart_setup, uint8_t *data, uint16_t len)
{
    (void)uart_setup;
    if (g_on_rx)
    {
        g_on_rx(data, len);
    }
    return 0;
}

void
zb_plat_serial_open(const s_zb_serial_cfg_t *cfg, zb_serial_rx_cb_t on_rx)
{
    if (!cfg)
    {
        return;
    }
    g_on_rx = on_rx;

    g_uart_setup.port_num = cfg->port;
    g_uart_setup.baud = cfg->baud;
    g_uart_setup.tx_pin = cfg->tx_pin;
    g_uart_setup.rx_pin = cfg->rx_pin;
    g_uart_setup.rts_pin = cfg->rts_pin;
    g_uart_setup.cts_pin = cfg->cts_pin;

    s_iotdev_uart_connxn_t uart_connxn =
    {
        .uart_setup = &g_uart_setup,
        .uart_buffer = g_uart_buf,
        .uart_buffer_max_len = sizeof(g_uart_buf),
        .uart_buffer_len = 0,
        .rx_sub_cb = serial_rx_trampoline,
    };

    iotdev_uart_setup_connxn(&uart_connxn, true);
    // zb_plat_uart_rx_pullup_en(cfg->rx_pin);
}

void
zb_plat_serial_close(void)
{
    s_iotdev_uart_connxn_t uart_connxn = { .uart_setup = &g_uart_setup };
    iotdev_uart_disconnxn(&uart_connxn);
    g_on_rx = NULL;
}

int
zb_plat_serial_write(const uint8_t *data, uint16_t len)
{
    return iotdev_uart_write(&g_uart_setup, (uint8_t *)data, len);
}

void
zb_plat_serial_flush_rx(void)
{
    iotdev_uart_flush_rx(&g_uart_setup);
}

#endif /* ZB_PLATFORM_IOTDEV */
