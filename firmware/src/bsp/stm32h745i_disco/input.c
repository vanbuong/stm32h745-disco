#include "bsp/board.h"

#include "cube.h"
#include "ft5336.h"
#include "hal/input.h"

/*
 * H745I-DISCO touch: newer panels are GT911 (I2C 0xBA / 0x28, 16-bit
 * regs). Older ones are FT5336 (0x70, 8-bit). Probe GT911 first, matching
 * the current ST disco BSP. INT on PG2 is polled (no EXTI).
 */

#define GT911_REG_ID 0x8140u
#define GT911_REG_STAT 0x814Eu
#define GT911_REG_PT1 0x8150u

#define TS_NONE 0u
#define TS_FT5336 1u
#define TS_GT911 2u

static FT5336_Object_t g_ft;
static uint8_t g_kind;
static uint16_t g_gt_addr;
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

static uint8_t gt_id_ok(uint16_t addr)
{
    uint8_t id[4];

    if (board_i2c4_read16(addr, GT911_REG_ID, id, 4u) != 0) {
        return 0u;
    }
    return (id[0] == (uint8_t)'9' && id[1] == (uint8_t)'1' && id[2] == (uint8_t)'1') ? 1u : 0u;
}

static uint8_t probe_gt911(void)
{
    static const uint16_t addrs[2] = {BOARD_GT911_ADDR, BOARD_GT911_ADDR_ALT};
    unsigned i;

    for (i = 0u; i < 2u; i++) {
        if (gt_id_ok(addrs[i]) != 0u) {
            g_gt_addr = addrs[i];
            g_kind = TS_GT911;
            return 1u;
        }
    }
    return 0u;
}

static uint8_t probe_ft5336(void)
{
    FT5336_IO_t io = {0};
    uint32_t id = 0u;

    io.Init = ft_io_init;
    io.DeInit = ft_io_deinit;
    io.Address = BOARD_FT5336_ADDR;
    io.WriteReg = board_i2c4_write_reg;
    io.ReadReg = board_i2c4_read_reg;
    io.GetTick = ft_io_get_tick;

    if (FT5336_RegisterBusIO(&g_ft, &io) != FT5336_OK) {
        return 0u;
    }
    if (FT5336_ReadID(&g_ft, &id) != FT5336_OK || id != FT5336_ID) {
        return 0u;
    }
    if (FT5336_Init(&g_ft) != FT5336_OK) {
        return 0u;
    }
    g_kind = TS_FT5336;
    return 1u;
}

static uint8_t gt_sample(int16_t *x, int16_t *y, uint8_t *down)
{
    uint8_t st;
    uint8_t xy[4];
    uint8_t clr = 0u;

    if (board_i2c4_read16(g_gt_addr, GT911_REG_STAT, &st, 1u) != 0) {
        return 0u;
    }
    *down = (((st & 0x80u) != 0u) && ((st & 0x0Fu) != 0u)) ? 1u : 0u;
    if (*down != 0u) {
        if (board_i2c4_read16(g_gt_addr, GT911_REG_PT1, xy, 4u) != 0) {
            (void)board_i2c4_write16(g_gt_addr, GT911_REG_STAT, &clr, 1u);
            return 0u;
        }
        *x = (int16_t)((uint16_t)xy[0] | ((uint16_t)xy[1] << 8));
        *y = (int16_t)((uint16_t)xy[2] | ((uint16_t)xy[3] << 8));
    }
    (void)board_i2c4_write16(g_gt_addr, GT911_REG_STAT, &clr, 1u);
    return 1u;
}

err_t board_input_init(void)
{
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

    g_kind = TS_NONE;
    g_was_down = 0u;
    if (probe_gt911() == 0u) {
        (void)probe_ft5336();
    }
    return ERR_OK;
}

uint8_t board_touch_present(void)
{
    return (g_kind != TS_NONE) ? 1u : 0u;
}

const char *board_touch_name(void)
{
    if (g_kind == TS_GT911) {
        return "gt911";
    }
    if (g_kind == TS_FT5336) {
        return "ft5336";
    }
    return "none";
}

bool input_poll(input_event_t *out)
{
    uint8_t down = 0u;
    int16_t x = g_last_x;
    int16_t y = g_last_y;

    if (out == NULL || g_kind == TS_NONE) {
        return false;
    }
    if (g_kind == TS_GT911) {
        if (gt_sample(&x, &y, &down) == 0u) {
            return false;
        }
    } else {
        FT5336_State_t st;

        if (FT5336_GetState(&g_ft, &st) != FT5336_OK) {
            return false;
        }
        down = (st.TouchDetected != 0u) ? 1u : 0u;
        if (down != 0u) {
            x = (int16_t)st.TouchX;
            y = (int16_t)st.TouchY;
        }
    }
    if (down != 0u) {
        if (x < 0) {
            x = 0;
        }
        if (y < 0) {
            y = 0;
        }
        if (x >= (int16_t)BOARD_LCD_W) {
            x = (int16_t)(BOARD_LCD_W - 1u);
        }
        if (y >= (int16_t)BOARD_LCD_H) {
            y = (int16_t)(BOARD_LCD_H - 1u);
        }
        out->kind = g_was_down ? INPUT_PTR_MOVE : INPUT_PTR_DOWN;
        out->x = x;
        out->y = y;
        out->id = 0u;
        out->t_ms = HAL_GetTick();
        g_last_x = x;
        g_last_y = y;
        g_was_down = 1u;
        return true;
    }
    if (g_was_down != 0u) {
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
