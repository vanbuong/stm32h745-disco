#include "platform/iotdev/zb_iotdev_peripheral.h"
#include "platform/iotdev/zb_plat_serial.h"

#include "zb_osal.h"
#include "zb_port.h"

#include "bsp/board.h"
#include "hal/uart.h"

#include <string.h>

#define ZNP_SREQ_TYPE 0x20u
#define ZNP_SREQ_COLLECT_MS 40u
#define ZNP_SREQ_GAP_MS 2u
#define ZNP_RX_BURST 256u
#define ZNP_FLUSH_MS 8u

static zb_serial_rx_cb_t g_rx;
static uint8_t g_open;

void zb_plat_busy_wait_us(uint32_t us)
{
    uint32_t ms;
    uint32_t t0;

    if (us == 0u) {
        return;
    }
    /* Busy-wait so the RESET pulse is not pre-empted. vTaskDelay here let
     * the UI task run ETH/console mid-pulse and HardFault. */
    ms = (us + 999u) / 1000u;
    if (ms == 0u) {
        ms = 1u;
    }
    t0 = board_millis();
    while ((uint32_t)(board_millis() - t0) < ms) {
    }
}

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

static void rx_to_cb(void)
{
    uint8_t buf[64];
    size_t got = 0u;
    uint32_t n = 0u;

    if (g_open == 0u || g_rx == NULL) {
        return;
    }
    while (n < ZNP_RX_BURST && uart_read(UART_ID_ZNP, buf, sizeof(buf), &got, 0u) == ERR_OK &&
           got > 0u) {
        g_rx(buf, (uint16_t)got);
        n += (uint32_t)got;
        got = 0u;
    }
}

static void collect_sreq_reply(const uint8_t *data, uint16_t len)
{
    uint32_t t0;
    uint32_t last;
    uint8_t any = 0u;

    if (data == NULL || len < 3u || data[0] != 0xFEu || (data[2] & 0xE0u) != ZNP_SREQ_TYPE) {
        rx_to_cb();
        return;
    }
    t0 = board_millis();
    last = t0;
    for (;;) {
        uint32_t now = board_millis();
        size_t got = 0u;
        uint8_t buf[64];

        if (g_rx == NULL) {
            break;
        }
        if (uart_read(UART_ID_ZNP, buf, sizeof(buf), &got, 0u) == ERR_OK && got > 0u) {
            g_rx(buf, (uint16_t)got);
            any = 1u;
            last = now;
            continue;
        }
        if (any != 0u && (uint32_t)(now - last) >= ZNP_SREQ_GAP_MS) {
            break;
        }
        if ((uint32_t)(now - t0) >= ZNP_SREQ_COLLECT_MS) {
            break;
        }
        zb_os_delay_ms(1u);
    }
}

int zb_plat_serial_write(const uint8_t *data, uint16_t len)
{
    if (g_open == 0u) {
        return -1;
    }
    if (uart_write(UART_ID_ZNP, data, len) != ERR_OK) {
        return -1;
    }
    collect_sreq_reply(data, len);
    return (int)len;
}

void zb_plat_serial_flush_rx(void)
{
    uint8_t buf[32];
    size_t got = 0u;
    uint32_t t0;
    uint32_t n = 0u;

    if (g_open == 0u) {
        return;
    }
    t0 = board_millis();
    while (uart_read(UART_ID_ZNP, buf, sizeof(buf), &got, 0u) == ERR_OK && got > 0u) {
        n += (uint32_t)got;
        got = 0u;
        if (n >= ZNP_RX_BURST || (uint32_t)(board_millis() - t0) >= ZNP_FLUSH_MS) {
            break;
        }
    }
}

void zb_plat_serial_poll(void)
{
    rx_to_cb();
}

int zb_plat_serial_ok(void)
{
    return (g_open != 0u) ? 1 : 0;
}
