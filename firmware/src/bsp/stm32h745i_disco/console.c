#include "bsp/board.h"

#include "stm32h745_regs.h"

void board_console_init(uint32_t pclk1_hz)
{
    if (pclk1_hz == 0u) {
        pclk1_hz = BOARD_HSI_HZ;
    }

    RCC_AHB4ENR |= RCC_AHB4ENR_GPIOBEN;
    RCC_APB1LENR |= RCC_APB1LENR_USART3EN;
    (void)RCC_APB1LENR;

    /* PB10/PB11 AF7 USART3 */
    gpio_af(GPIOB_BASE, 10u, 7u, 0u);
    gpio_af(GPIOB_BASE, 11u, 7u, 1u);

    USART_CR1(USART3_BASE) = 0;
    USART_BRR(USART3_BASE) = pclk1_hz / BOARD_UART_BAUD;
    USART_CR1(USART3_BASE) = USART_CR1_UE | USART_CR1_TE;
}

void board_console_puts(const char *s)
{
    if (s == NULL) {
        return;
    }
    while (*s != '\0') {
        while ((USART_ISR(USART3_BASE) & USART_ISR_TXE) == 0u) {
        }
        USART_TDR(USART3_BASE) = (uint32_t)(uint8_t)*s++;
    }
}

void board_console_put_hex32(uint32_t v)
{
    static const char hex[] = "0123456789ABCDEF";
    char buf[9];
    int i;

    for (i = 7; i >= 0; i--) {
        buf[i] = hex[v & 0xFu];
        v >>= 4;
    }
    buf[8] = '\0';
    board_console_puts(buf);
}
