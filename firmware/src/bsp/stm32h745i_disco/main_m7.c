#include "stm32h745_regs.h"

static void delay(volatile uint32_t n)
{
    while (n > 0u) {
        n--;
    }
}

static void usart3_init(void)
{
    RCC_AHB4ENR |= RCC_AHB4ENR_GPIOBEN;
    RCC_APB1LENR |= RCC_APB1LENR_USART3EN;

    /* PB10/PB11 AF7 USART3 */
    GPIO_MODER(GPIOB_BASE) &= ~((3u << 20) | (3u << 22));
    GPIO_MODER(GPIOB_BASE) |= (2u << 20) | (2u << 22);
    GPIO_OSPEEDR(GPIOB_BASE) |= (3u << 20) | (3u << 22);
    GPIO_AFR(GPIOB_BASE, 1) &= ~((0xFu << 8) | (0xFu << 12));
    GPIO_AFR(GPIOB_BASE, 1) |= (7u << 8) | (7u << 12);

    /* HSI 64 MHz default, 115200 baud */
    USART_BRR(USART3_BASE) = 64u * 1000u * 1000u / 115200u;
    USART_CR1(USART3_BASE) = USART_CR1_UE | USART_CR1_TE;
}

static void usart3_write(const char *s)
{
    while (*s != '\0') {
        while ((USART_ISR(USART3_BASE) & USART_ISR_TXE) == 0u) {
        }
        USART_TDR(USART3_BASE) = (uint32_t)(uint8_t)*s++;
    }
}

static void led_init(void)
{
    RCC_AHB4ENR |= RCC_AHB4ENR_GPIOIEN;
    GPIO_MODER(GPIOI_BASE) &= ~(3u << 26);
    GPIO_MODER(GPIOI_BASE) |= (1u << 26);
}

int main(void)
{
    led_init();
    usart3_init();
    usart3_write("M7 stm32h745-disco\r\n");
    for (;;) {
        GPIO_BSRR(GPIOI_BASE) = (1u << 13);
        delay(800000u);
        GPIO_BSRR(GPIOI_BASE) = (1u << 29);
        delay(800000u);
    }
}
