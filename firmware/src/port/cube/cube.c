#include "cube.h"

err_t cube_err(HAL_StatusTypeDef s)
{
    if (s == HAL_OK) {
        return ERR_OK;
    }
    if (s == HAL_TIMEOUT) {
        return ERR_TIMEOUT;
    }
    if (s == HAL_BUSY) {
        return ERR_BUSY;
    }
    return ERR_IO;
}

void cube_gpio_af(GPIO_TypeDef *port, uint32_t pin, uint32_t af, uint32_t pull)
{
    GPIO_InitTypeDef g = {0};

    g.Pin = pin;
    g.Mode = GPIO_MODE_AF_PP;
    g.Pull = pull;
    g.Speed = GPIO_SPEED_FREQ_VERY_HIGH;
    g.Alternate = af;
    HAL_GPIO_Init(port, &g);
}

void HAL_MspInit(void)
{
    __HAL_RCC_SYSCFG_CLK_ENABLE();
    __HAL_RCC_HSEM_CLK_ENABLE();
}
