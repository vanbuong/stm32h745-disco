#include "bsp/board.h"

#include "cube.h"

/*
 * I2C4 on PD12/PD13 (AF4). Shared later by FT5336 + WM8994; this sprint only
 * the touch controller uses it. Bare-metal lock: one outstanding transfer.
 */

#define I2C4_TIMING 0x307075B1u /* 100 kHz at 120 MHz D3PCLK1 */

static I2C_HandleTypeDef g_i2c4;
static uint8_t g_taken;
static uint8_t g_ready;

void HAL_I2C_MspInit(I2C_HandleTypeDef *hi2c)
{
    GPIO_InitTypeDef g = {0};

    if (hi2c->Instance != I2C4) {
        return;
    }
    __HAL_RCC_GPIOD_CLK_ENABLE();
    __HAL_RCC_I2C4_CLK_ENABLE();

    g.Pin = GPIO_PIN_12 | GPIO_PIN_13;
    g.Mode = GPIO_MODE_AF_OD;
    g.Pull = GPIO_PULLUP;
    g.Speed = GPIO_SPEED_FREQ_HIGH;
    g.Alternate = GPIO_AF4_I2C4;
    HAL_GPIO_Init(GPIOD, &g);
}

err_t board_i2c4_init(void)
{
    RCC_PeriphCLKInitTypeDef p = {0};

    if (g_ready) {
        return ERR_OK;
    }

    p.PeriphClockSelection = RCC_PERIPHCLK_I2C4;
    p.I2c4ClockSelection = RCC_I2C4CLKSOURCE_D3PCLK1;
    if (HAL_RCCEx_PeriphCLKConfig(&p) != HAL_OK) {
        return ERR_IO;
    }

    g_i2c4.Instance = I2C4;
    g_i2c4.Init.Timing = I2C4_TIMING;
    g_i2c4.Init.OwnAddress1 = 0u;
    g_i2c4.Init.AddressingMode = I2C_ADDRESSINGMODE_7BIT;
    g_i2c4.Init.DualAddressMode = I2C_DUALADDRESS_DISABLE;
    g_i2c4.Init.OwnAddress2 = 0u;
    g_i2c4.Init.OwnAddress2Masks = I2C_OA2_NOMASK;
    g_i2c4.Init.GeneralCallMode = I2C_GENERALCALL_DISABLE;
    g_i2c4.Init.NoStretchMode = I2C_NOSTRETCH_DISABLE;
    if (HAL_I2C_Init(&g_i2c4) != HAL_OK) {
        return ERR_IO;
    }
    if (HAL_I2CEx_ConfigAnalogFilter(&g_i2c4, I2C_ANALOGFILTER_ENABLE) != HAL_OK) {
        return ERR_IO;
    }
    g_ready = 1u;
    g_taken = 0u;
    return ERR_OK;
}

err_t board_i2c4_lock(void)
{
    if (!g_ready) {
        return ERR_INVAL;
    }
    if (g_taken) {
        return ERR_BUSY;
    }
    g_taken = 1u;
    return ERR_OK;
}

void board_i2c4_unlock(void)
{
    g_taken = 0u;
}

int32_t board_i2c4_read_reg(uint16_t addr, uint16_t reg, uint8_t *data, uint16_t len)
{
    HAL_StatusTypeDef s;

    if (data == NULL || len == 0u) {
        return -1;
    }
    if (board_i2c4_lock() != ERR_OK) {
        return -1;
    }
    s = HAL_I2C_Mem_Read(&g_i2c4, addr, reg, I2C_MEMADD_SIZE_8BIT, data, len, 100u);
    board_i2c4_unlock();
    return (s == HAL_OK) ? 0 : -1;
}

int32_t board_i2c4_write_reg(uint16_t addr, uint16_t reg, uint8_t *data, uint16_t len)
{
    HAL_StatusTypeDef s;

    if (data == NULL || len == 0u) {
        return -1;
    }
    if (board_i2c4_lock() != ERR_OK) {
        return -1;
    }
    s = HAL_I2C_Mem_Write(&g_i2c4, addr, reg, I2C_MEMADD_SIZE_8BIT, data, len, 100u);
    board_i2c4_unlock();
    return (s == HAL_OK) ? 0 : -1;
}
