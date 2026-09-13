#include "hal/uart.h"

#include "bsp/board.h"

#include "cube.h"

/*
 * TI ZNP on CN2 STMod+: USART2 PD5/PD6 (STMOD#2 TX / #3 RX), 921600 8N1.
 * PH10 = RESET (STMOD#12, active low). PA4 = BOOT (STMOD#13): high = ZNP
 * app, low during reset = SBL. Polled only; 16-exception vector table.
 */
#define ZNP_USART USART2
#define ZNP_RESET_PORT GPIOH
#define ZNP_RESET_PIN GPIO_PIN_10
#define ZNP_BOOT_PORT GPIOA
#define ZNP_BOOT_PIN GPIO_PIN_4

static uint8_t g_open;

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

static void drain_rx(uint32_t ms)
{
    uint32_t t0 = HAL_GetTick();

    while ((HAL_GetTick() - t0) < ms) {
        if (LL_USART_IsActiveFlag_ORE(ZNP_USART) != 0u) {
            LL_USART_ClearFlag_ORE(ZNP_USART);
            (void)LL_USART_ReceiveData8(ZNP_USART);
        }
        if (LL_USART_IsActiveFlag_FE(ZNP_USART) != 0u) {
            LL_USART_ClearFlag_FE(ZNP_USART);
        }
        if (LL_USART_IsActiveFlag_RXNE(ZNP_USART) != 0u) {
            (void)LL_USART_ReceiveData8(ZNP_USART);
        }
    }
}

static void znp_reset_app(void)
{
    /* BOOT high during reset runs the ZNP image; low enters the SBL. */
    HAL_GPIO_WritePin(ZNP_BOOT_PORT, ZNP_BOOT_PIN, GPIO_PIN_SET);
    HAL_GPIO_WritePin(ZNP_RESET_PORT, ZNP_RESET_PIN, GPIO_PIN_RESET);
    HAL_Delay(10u);
    HAL_GPIO_WritePin(ZNP_RESET_PORT, ZNP_RESET_PIN, GPIO_PIN_SET);
    /* App start can take up to UART_ZNP_RESET_MS; zb_host retries SYS_PING. */
    drain_rx(50u);
}

err_t uart_open(uart_id_t id, const uart_cfg_t *cfg)
{
    uint32_t baud = BOARD_ZNP_UART_BAUD;
    uint32_t pclk1;

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
    LL_USART_Enable(ZNP_USART);

    znp_reset_app();
    g_open = 1u;
    return ERR_OK;
}

err_t uart_write(uart_id_t id, const void *data, size_t n)
{
    const uint8_t *p = data;
    size_t i;

    if (id != UART_ID_ZNP || g_open == 0u || (n > 0u && data == NULL)) {
        return ERR_IO;
    }
    for (i = 0u; i < n; i++) {
        while (LL_USART_IsActiveFlag_TXE(ZNP_USART) == 0u) {
        }
        LL_USART_TransmitData8(ZNP_USART, p[i]);
    }
    return ERR_OK;
}

err_t uart_read(uart_id_t id, void *data, size_t n, size_t *got, uint32_t timeout_ms)
{
    uint8_t *p = data;
    uint32_t t0;
    size_t n_got = 0u;

    if (got != NULL) {
        *got = 0u;
    }
    if (id != UART_ID_ZNP || g_open == 0u || (n > 0u && data == NULL)) {
        return ERR_IO;
    }

    t0 = HAL_GetTick();
    while (n_got < n) {
        if (LL_USART_IsActiveFlag_ORE(ZNP_USART) != 0u) {
            LL_USART_ClearFlag_ORE(ZNP_USART);
        }
        if (LL_USART_IsActiveFlag_RXNE(ZNP_USART) != 0u) {
            p[n_got++] = LL_USART_ReceiveData8(ZNP_USART);
            continue;
        }
        if ((HAL_GetTick() - t0) >= timeout_ms) {
            break;
        }
    }
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
