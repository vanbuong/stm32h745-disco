#include "hal/uart.h"
#include "svc/znp_mt.h"

#include <string.h>

#define UART_RX_MAX 64u

#define MT_SYS_SREQ 0x21u
#define MT_SYS_SRSP 0x61u
#define MT_SYS_PING 0x01u

static uint8_t g_open_znp;
static uint8_t g_rx[UART_RX_MAX];
static size_t g_rx_n;

static void rx_push(const uint8_t *p, size_t n)
{
    size_t i;

    if (p == NULL) {
        return;
    }
    for (i = 0u; i < n && g_rx_n < UART_RX_MAX; i++) {
        g_rx[g_rx_n++] = p[i];
    }
}

err_t uart_open(uart_id_t id, const uart_cfg_t *cfg)
{
    (void)cfg;
    if (id != UART_ID_ZNP) {
        return ERR_UNSUPPORTED;
    }
    g_open_znp = 1u;
    g_rx_n = 0u;
    return ERR_OK;
}

err_t uart_write(uart_id_t id, const void *data, size_t n)
{
    uint8_t cmd0 = 0u;
    uint8_t cmd1 = 0u;
    uint8_t plen = 0u;
    uint8_t pl[8];
    uint8_t reply[16];
    uint8_t caps[2] = {0x01u, 0x00u};
    size_t rn = 0u;

    if (id != UART_ID_ZNP || g_open_znp == 0u || data == NULL) {
        return ERR_IO;
    }
    if (znp_mt_decode((const uint8_t *)data, n, &cmd0, &cmd1, pl, (uint8_t)sizeof(pl), &plen) !=
        ERR_OK) {
        return ERR_OK;
    }
    if (cmd0 == MT_SYS_SREQ && cmd1 == MT_SYS_PING) {
        if (znp_mt_encode(MT_SYS_SRSP, MT_SYS_PING, caps, 2u, reply, sizeof(reply), &rn) ==
            ERR_OK) {
            rx_push(reply, rn);
        }
    }
    return ERR_OK;
}

err_t uart_read(uart_id_t id, void *data, size_t n, size_t *got, uint32_t timeout_ms)
{
    size_t take;

    (void)timeout_ms;
    if (got != NULL) {
        *got = 0u;
    }
    if (id != UART_ID_ZNP || g_open_znp == 0u || data == NULL) {
        return ERR_IO;
    }
    take = n;
    if (take > g_rx_n) {
        take = g_rx_n;
    }
    if (take > 0u) {
        memcpy(data, g_rx, take);
        if (g_rx_n > take) {
            memmove(g_rx, g_rx + take, g_rx_n - take);
        }
        g_rx_n = (size_t)(g_rx_n - take);
    }
    if (got != NULL) {
        *got = take;
    }
    return ERR_OK;
}

void uart_rx_pump(void)
{
}

err_t uart_set_gpio(uart_id_t id, uint8_t pin_id, int level)
{
    (void)pin_id;
    (void)level;
    if (id != UART_ID_ZNP) {
        return ERR_UNSUPPORTED;
    }
    return ERR_OK;
}
