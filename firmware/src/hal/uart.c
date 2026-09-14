#include "hal/uart.h"

#include "bsp/board.h"

#include "cube.h"

/*
 * TI ZNP on CN2 STMod+: USART2 PD5/PD6 (STMOD#2 TX / #3 RX), 921600 8N1.
 * PH10 = RESET (STMOD#12, active low). PA4 = BOOT (STMOD#13): high = ZNP
 * app, low during reset = SBL. Polled only; 16-exception vector table.
 *
 * At 921600 the HW RDR/FIFO overruns if RX is not drained while USART3
 * console logs (~3 ms/line) or LVGL runs. Bytes are copied into a software
 * ring from every UART touch and from board_console_puts().
 */
#define ZNP_USART USART2
#define ZNP_RESET_PORT GPIOH
#define ZNP_RESET_PIN GPIO_PIN_10
#define ZNP_BOOT_PORT GPIOA
#define ZNP_BOOT_PIN GPIO_PIN_4
#define RX_RING 256u
#define ACK_SPINS 10000u

static uint8_t g_open;
static uint8_t g_rx[RX_RING];
static uint16_t g_rx_head;
static uint16_t g_rx_n;

static void gpio_out(GPIO_TypeDef *port, uint32_t pin, GPIO_PinState level)
{
    GPIO_InitTypeDef g = {0};

    g.Pin = pin;
    g.Mode = GPIO_MODE_OUTPUT_PP;
    g.Pull = GPIO_NOPULL;
    g.Speed = GPIO_SPEED_FREQ_LOW;
    HAL_GPIO_Init(port, &g);
    HAL_GPIO_WritePin(port, pin, level);
}

static void rx_put(uint8_t b)
{
    if (g_rx_n >= RX_RING) {
        return;
    }
    g_rx[(g_rx_head + g_rx_n) % RX_RING] = b;
    g_rx_n++;
}

static uint32_t uart_lock(void)
{
    uint32_t primask = __get_PRIMASK();

    __disable_irq();
    return primask;
}

static void uart_unlock(uint32_t primask)
{
    if (primask == 0u) {
        __enable_irq();
    }
}

static void uart_rx_pump_body(void)
{
    if (g_open == 0u) {
        return;
    }
    for (;;) {
        if (LL_USART_IsActiveFlag_ORE(ZNP_USART) != 0u) {
            if (LL_USART_IsActiveFlag_RXNE(ZNP_USART) != 0u) {
                rx_put(LL_USART_ReceiveData8(ZNP_USART));
            }
            LL_USART_ClearFlag_ORE(ZNP_USART);
            continue;
        }
        if (LL_USART_IsActiveFlag_FE(ZNP_USART) != 0u) {
            LL_USART_ClearFlag_FE(ZNP_USART);
        }
        if (LL_USART_IsActiveFlag_RXNE(ZNP_USART) != 0u) {
            rx_put(LL_USART_ReceiveData8(ZNP_USART));
            continue;
        }
        break;
    }
}

void uart_rx_pump(void)
{
    uint32_t lock = uart_lock();

    uart_rx_pump_body();
    uart_unlock(lock);
}

err_t uart_open(uart_id_t id, const uart_cfg_t *cfg)
{
    uint32_t baud = BOARD_ZNP_UART_BAUD;
    uint32_t pclk1;
    uint32_t spins;

    if (id != UART_ID_ZNP) {
        return ERR_UNSUPPORTED;
    }
    if (cfg != NULL && cfg->baud != 0u) {
        baud = cfg->baud;
    }

    __HAL_RCC_GPIOA_CLK_ENABLE();
    __HAL_RCC_GPIOD_CLK_ENABLE();
    __HAL_RCC_GPIOH_CLK_ENABLE();
    LL_APB1_GRP1_EnableClock(LL_APB1_GRP1_PERIPH_USART2);

    gpio_out(ZNP_BOOT_PORT, ZNP_BOOT_PIN, GPIO_PIN_SET);
    gpio_out(ZNP_RESET_PORT, ZNP_RESET_PIN, GPIO_PIN_RESET);

    cube_gpio_af(GPIOD, GPIO_PIN_5, GPIO_AF7_USART2, GPIO_NOPULL);
    cube_gpio_af(GPIOD, GPIO_PIN_6, GPIO_AF7_USART2, GPIO_PULLUP);

    pclk1 = HAL_RCC_GetPCLK1Freq();
    if (pclk1 == 0u) {
        pclk1 = BOARD_PCLK1_HZ;
    }

    LL_USART_Disable(ZNP_USART);
    LL_USART_SetTransferDirection(ZNP_USART, LL_USART_DIRECTION_TX_RX);
    LL_USART_SetDataWidth(ZNP_USART, LL_USART_DATAWIDTH_8B);
    LL_USART_SetParity(ZNP_USART, LL_USART_PARITY_NONE);
    LL_USART_SetStopBitsLength(ZNP_USART, LL_USART_STOPBITS_1);
    LL_USART_SetOverSampling(ZNP_USART, LL_USART_OVERSAMPLING_16);
    LL_USART_SetBaudRate(ZNP_USART, pclk1, LL_USART_PRESCALER_DIV1, LL_USART_OVERSAMPLING_16, baud);
    LL_USART_EnableFIFO(ZNP_USART);
    LL_USART_SetRXFIFOThreshold(ZNP_USART, LL_USART_FIFOTHRESHOLD_1_8);
    LL_USART_SetTXFIFOThreshold(ZNP_USART, LL_USART_FIFOTHRESHOLD_1_8);
    LL_USART_Enable(ZNP_USART);
    spins = 0u;
    while (LL_USART_IsActiveFlag_TEACK(ZNP_USART) == 0u && spins < ACK_SPINS) {
        spins++;
    }
    spins = 0u;
    while (LL_USART_IsActiveFlag_REACK(ZNP_USART) == 0u && spins < ACK_SPINS) {
        spins++;
    }

    g_rx_head = 0u;
    g_rx_n = 0u;
    g_open = 1u;
    /* Keep RESET asserted. zb_znp set-mode owns the pulse (needs a real
     * delay_us) so SYS_RESET_IND lands in the parser instead of drain_rx. */
    return ERR_OK;
}

err_t uart_write(uart_id_t id, const void *data, size_t n)
{
    const uint8_t *p = data;
    size_t i;

    if (id != UART_ID_ZNP || g_open == 0u || (n > 0u && data == NULL)) {
        return ERR_IO;
    }
    uart_rx_pump();
    for (i = 0u; i < n; i++) {
        while (LL_USART_IsActiveFlag_TXE(ZNP_USART) == 0u) {
            uart_rx_pump();
        }
        LL_USART_TransmitData8(ZNP_USART, p[i]);
        uart_rx_pump();
    }
    while (LL_USART_IsActiveFlag_TC(ZNP_USART) == 0u) {
        uart_rx_pump();
    }
    uart_rx_pump();
    return ERR_OK;
}

err_t uart_read(uart_id_t id, void *data, size_t n, size_t *got, uint32_t timeout_ms)
{
    uint8_t *p = data;
    uint32_t t0;
    uint32_t lock;
    size_t n_got = 0u;

    if (got != NULL) {
        *got = 0u;
    }
    if (id != UART_ID_ZNP || g_open == 0u || (n > 0u && data == NULL)) {
        return ERR_IO;
    }

    t0 = HAL_GetTick();
    lock = uart_lock();
    while (n_got < n) {
        uart_rx_pump_body();
        if (g_rx_n > 0u) {
            p[n_got++] = g_rx[g_rx_head];
            g_rx_head = (uint16_t)((g_rx_head + 1u) % RX_RING);
            g_rx_n--;
            continue;
        }
        if (timeout_ms == 0u || (HAL_GetTick() - t0) >= timeout_ms) {
            break;
        }
        uart_unlock(lock);
        lock = uart_lock();
    }
    uart_unlock(lock);
    if (got != NULL) {
        *got = n_got;
    }
    return (n_got > 0u) ? ERR_OK : ERR_IO;
}

err_t uart_set_gpio(uart_id_t id, uint8_t pin_id, int level)
{
    GPIO_PinState s = (level != 0) ? GPIO_PIN_SET : GPIO_PIN_RESET;

    if (id != UART_ID_ZNP) {
        return ERR_UNSUPPORTED;
    }
    __HAL_RCC_GPIOA_CLK_ENABLE();
    __HAL_RCC_GPIOH_CLK_ENABLE();
    if (pin_id == UART_PIN_RESET) {
        gpio_out(ZNP_RESET_PORT, ZNP_RESET_PIN, s);
        return ERR_OK;
    }
    if (pin_id == UART_PIN_BOOT) {
        gpio_out(ZNP_BOOT_PORT, ZNP_BOOT_PIN, s);
        return ERR_OK;
    }
    return ERR_INVAL;
}
