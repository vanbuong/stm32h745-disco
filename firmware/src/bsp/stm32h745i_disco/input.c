#include "bsp/board.h"

#include "cube.h"
#include "ft5336.h"
#include "hal/input.h"

/*
 * FT5336 on I2C4, 8-bit address 0x70. INT on PG2 is polled (no EXTI this
 * sprint — vector table is the Cortex-M 16). Some disco revs ship GT911;
 * probe FT5336 only and continue without touch if the ID is wrong.
 */

static FT5336_Object_t g_ft;
static uint8_t g_have_touch;
static uint8_t g_was_down;
static int16_t g_last_x;
static int16_t g_last_y;

static int32_t ft_io_init(void)
{
    return 0;
}

static int32_t ft_io_deinit(void)
{
    return 0;
}

static int32_t ft_io_get_tick(void)
{
    return (int32_t)HAL_GetTick();
}

err_t board_input_init(void)
{
    FT5336_IO_t io = {0};
    uint32_t id = 0u;
    GPIO_InitTypeDef g = {0};
    err_t e;

    e = board_i2c4_init();
    if (e != ERR_OK) {
        return e;
    }

    __HAL_RCC_GPIOG_CLK_ENABLE();
    g.Pin = GPIO_PIN_2;
    g.Mode = GPIO_MODE_INPUT;
    g.Pull = GPIO_PULLUP;
    g.Speed = GPIO_SPEED_FREQ_LOW;
    HAL_GPIO_Init(GPIOG, &g);

    io.Init = ft_io_init;
    io.DeInit = ft_io_deinit;
    io.Address = BOARD_FT5336_ADDR;
    io.WriteReg = board_i2c4_write_reg;
    io.ReadReg = board_i2c4_read_reg;
    io.GetTick = ft_io_get_tick;

    g_have_touch = 0u;
    g_was_down = 0u;
    if (FT5336_RegisterBusIO(&g_ft, &io) != FT5336_OK) {
        return ERR_OK;
    }
    if (FT5336_Init(&g_ft) != FT5336_OK) {
        return ERR_OK;
    }
    if (FT5336_ReadID(&g_ft, &id) != FT5336_OK || id != FT5336_ID) {
        return ERR_OK;
    }
    g_have_touch = 1u;
    return ERR_OK;
}

uint8_t board_touch_present(void)
{
    return g_have_touch;
}

bool input_poll(input_event_t *out)
{
    FT5336_State_t st;
    uint8_t down;

    if (out == NULL || !g_have_touch) {
        return false;
    }
    if (FT5336_GetState(&g_ft, &st) != FT5336_OK) {
        return false;
    }

    down = (st.TouchDetected != 0u) ? 1u : 0u;
    if (down) {
        uint16_t x = (uint16_t)st.TouchX;
        uint16_t y = (uint16_t)st.TouchY;
        if (x >= BOARD_LCD_W) {
            x = (uint16_t)(BOARD_LCD_W - 1u);
        }
        if (y >= BOARD_LCD_H) {
            y = (uint16_t)(BOARD_LCD_H - 1u);
        }
        out->kind = g_was_down ? INPUT_PTR_MOVE : INPUT_PTR_DOWN;
        out->x = (int16_t)x;
        out->y = (int16_t)y;
        out->id = 0u;
        out->t_ms = HAL_GetTick();
        g_last_x = out->x;
        g_last_y = out->y;
        g_was_down = 1u;
        return true;
    }
    if (g_was_down) {
        out->kind = INPUT_PTR_UP;
        out->x = g_last_x;
        out->y = g_last_y;
        out->id = 0u;
        out->t_ms = HAL_GetTick();
        g_was_down = 0u;
        return true;
    }
    return false;
}
