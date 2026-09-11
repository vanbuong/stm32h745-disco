#include "stm32h745_regs.h"

static void delay(volatile uint32_t n)
{
    while (n > 0u) {
        n--;
    }
}

int main(void)
{
    RCC_AHB4ENR |= RCC_AHB4ENR_GPIOJEN;
    GPIO_MODER(GPIOJ_BASE) &= ~(3u << 4);
    GPIO_MODER(GPIOJ_BASE) |= (1u << 4);
    for (;;) {
        GPIO_BSRR(GPIOJ_BASE) = (1u << 2);
        delay(400000u);
        GPIO_BSRR(GPIOJ_BASE) = (1u << 18);
        delay(400000u);
    }
}
