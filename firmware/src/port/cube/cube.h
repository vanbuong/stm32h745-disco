#ifndef CUBE_H
#define CUBE_H

#include "err.h"

/* ST stm32h7xx-hal-driver: HAL + LL (USE_HAL_DRIVER, USE_FULL_LL_DRIVER). */
#include "stm32h7xx_hal.h"
#include "stm32h7xx_ll_bus.h"
#include "stm32h7xx_ll_gpio.h"
#include "stm32h7xx_ll_rcc.h"
#include "stm32h7xx_ll_usart.h"

#ifdef __cplusplus
extern "C" {
#endif

err_t cube_err(HAL_StatusTypeDef s);
void cube_gpio_af(GPIO_TypeDef *port, uint32_t pin, uint32_t af, uint32_t pull);

#ifdef __cplusplus
}
#endif

#endif /* CUBE_H */
