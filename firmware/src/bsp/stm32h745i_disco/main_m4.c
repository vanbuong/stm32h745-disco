#include "stm32h7xx_ll_bus.h"
#include "stm32h7xx_ll_gpio.h"

static void delay(volatile uint32_t n)
{
    while (n > 0u) {
        n--;
    }
}

int main(void)
{
    LL_AHB4_GRP1_EnableClock(LL_AHB4_GRP1_PERIPH_GPIOJ);
    LL_GPIO_SetPinMode(GPIOJ, LL_GPIO_PIN_2, LL_GPIO_MODE_OUTPUT);
    for (;;) {
        LL_GPIO_SetOutputPin(GPIOJ, LL_GPIO_PIN_2);
        delay(400000u);
        LL_GPIO_ResetOutputPin(GPIOJ, LL_GPIO_PIN_2);
        delay(400000u);
    }
}
