#include "bsp/board.h"

#include "cube.h"
#include "ft5336.h"
#include "hal/input.h"

/*
 * H745I-DISCO touch: newer panels are GT911 (I2C 0xBA / 0x28, 16-bit
 * regs). Older ones are FT5336 (0x70, 8-bit). Probe GT911 first, matching
 * the current ST disco BSP. INT on PG2 is polled (no EXTI).
 */

#define GT911_REG_CMD 0x8040u
#define GT911_REG_XMAX 0x8048u
#define GT911_REG_ID 0x8140u
#define GT911_REG_STAT 0x814Eu
#define GT911_REG_PT1 0x8150u
#define GT911_STAT_READY 0x80u
#define GT911_STAT_COUNT 0x0Fu

#define TS_NONE 0u
#define TS_FT5336 1u
#define TS_GT911 2u

static FT5336_Object_t g_ft;
static uint8_t g_kind;
static uint16_t g_gt_addr;
static uint16_t g_gt_max_x;
static uint16_t g_gt_max_y;
static uint8_t g_gt_swap_xy;
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

static void gt_start(void)
{
    uint8_t cmd;
    uint8_t xy[4];
    uint8_t clr = 0u;

    cmd = 0x02u;
    (void)board_i2c4_write16(g_gt_addr, GT911_REG_CMD, &cmd, 1u);
    HAL_Delay(10u);
    cmd = 0x00u;
    (void)board_i2c4_write16(g_gt_addr, GT911_REG_CMD, &cmd, 1u);
    HAL_Delay(10u);
    (void)board_i2c4_write16(g_gt_addr, GT911_REG_STAT, &clr, 1u);

    g_gt_max_x = BOARD_LCD_W;
    g_gt_max_y = BOARD_LCD_H;
    g_gt_swap_xy = 0u;
    if (board_i2c4_read16(g_gt_addr, GT911_REG_XMAX, xy, 4u) == 0) {
        uint16_t mx = (uint16_t)xy[0] | ((uint16_t)xy[1] << 8);
        uint16_t my = (uint16_t)xy[2] | ((uint16_t)xy[3] << 8);
        if (mx >= 100u && my >= 100u) {
            g_gt_max_x = mx;
            g_gt_max_y = my;
        }
    }
    if (g_gt_max_x < g_gt_max_y) {
        g_gt_swap_xy = 1u;
    }
}

static uint8_t probe_gt911(void)
{
    static const uint16_t addrs[2] = {BOARD_GT911_ADDR, BOARD_GT911_ADDR_ALT};
    unsigned i;

    for (i = 0u; i < 2u; i++) {
        if (gt_id_ok(addrs[i]) != 0u) {
            g_gt_addr = addrs[i];
            g_kind = TS_GT911;
            gt_start();
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

static int16_t gt_map(uint16_t raw, uint16_t max, uint16_t out)
{
    if (max == 0u) {
        max = out;
    }
    if (raw >= max) {
        raw = (uint16_t)(max - 1u);
    }
    return (int16_t)(((uint32_t)raw * (uint32_t)out) / (uint32_t)max);
}

static uint8_t gt_sample(int16_t *x, int16_t *y, uint8_t *down)
{
    uint8_t st;
    uint8_t xy[8];
    uint8_t clr = 0u;
    uint16_t rx;
    uint16_t ry;

    if (board_i2c4_read16(g_gt_addr, GT911_REG_STAT, &st, 1u) != 0) {
        return 0u;
    }
    if ((st & GT911_STAT_READY) == 0u) {
        *down = 0u;
        return 1u;
    }
    *down = ((st & GT911_STAT_COUNT) != 0u) ? 1u : 0u;
    if (*down != 0u) {
        if (board_i2c4_read16(g_gt_addr, GT911_REG_PT1, xy, 8u) != 0) {
            (void)board_i2c4_write16(g_gt_addr, GT911_REG_STAT, &clr, 1u);
            return 0u;
        }
        rx = (uint16_t)xy[0] | ((uint16_t)xy[1] << 8);
        ry = (uint16_t)xy[2] | ((uint16_t)xy[3] << 8);
        if (g_gt_swap_xy != 0u) {
            uint16_t t = rx;
            rx = ry;
            ry = t;
            *x = gt_map(rx, g_gt_max_y, BOARD_LCD_W);
            *y = gt_map(ry, g_gt_max_x, BOARD_LCD_H);
        } else {
            *x = gt_map(rx, g_gt_max_x, BOARD_LCD_W);
            *y = gt_map(ry, g_gt_max_y, BOARD_LCD_H);
        }
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
    HAL_Delay(50u);
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
