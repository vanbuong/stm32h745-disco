#include "bsp/board.h"

#include "bsp/disp_geom.h"
#include "cube.h"
#include "hal/disp.h"
#include "rk043fn48h.h"

#include <string.h>

/*
 * RK043FN48H timings from third_party/stm32-rk043fn48h. GPIO/PLL3/LTDC numbers
 * from the ST disco BSP (read-only). RGB565 double buffer in FMC SDRAM bank 2.
 * Reload is polled (vector table is the Cortex-M 16; no LTDC IRQ).
 */

#define LCD_HSYNC RK043FN48H_HSYNC
#define LCD_HBP RK043FN48H_HBP
#define LCD_HFP RK043FN48H_HFP
#define LCD_VSYNC RK043FN48H_VSYNC
#define LCD_VBP RK043FN48H_VBP
#define LCD_VFP RK043FN48H_VFP

static LTDC_HandleTypeDef g_ltdc;
static DMA2D_HandleTypeDef g_dma2d;
static uint16_t *g_fb[2];
static uint8_t g_front;
static uint8_t g_ready;

static void lcd_gpio_out(GPIO_TypeDef *port, uint16_t pin, GPIO_PinState level)
{
    GPIO_InitTypeDef g = {0};

    g.Pin = pin;
    g.Mode = GPIO_MODE_OUTPUT_PP;
    g.Pull = GPIO_NOPULL;
    g.Speed = GPIO_SPEED_FREQ_HIGH;
    HAL_GPIO_Init(port, &g);
    HAL_GPIO_WritePin(port, pin, level);
}

void HAL_LTDC_MspInit(LTDC_HandleTypeDef *hltdc)
{
    GPIO_InitTypeDef g = {0};

    (void)hltdc;
    __HAL_RCC_GPIOA_CLK_ENABLE();
    __HAL_RCC_GPIOD_CLK_ENABLE();
    __HAL_RCC_GPIOH_CLK_ENABLE();
    __HAL_RCC_GPIOI_CLK_ENABLE();
    __HAL_RCC_GPIOJ_CLK_ENABLE();
    __HAL_RCC_GPIOK_CLK_ENABLE();
    __HAL_RCC_LTDC_CLK_ENABLE();

    g.Mode = GPIO_MODE_AF_PP;
    g.Pull = GPIO_NOPULL;
    g.Speed = GPIO_SPEED_FREQ_HIGH;
    g.Alternate = GPIO_AF14_LTDC;

    g.Pin = GPIO_PIN_0 | GPIO_PIN_1 | GPIO_PIN_9 | GPIO_PIN_12 | GPIO_PIN_14 | GPIO_PIN_15;
    HAL_GPIO_Init(GPIOI, &g);

    /* PJ2 is the M4 LED — leave it alone. */
    g.Pin = GPIO_PIN_0 | GPIO_PIN_1 | GPIO_PIN_3 | GPIO_PIN_4 | GPIO_PIN_5 | GPIO_PIN_6 |
            GPIO_PIN_7 | GPIO_PIN_8 | GPIO_PIN_9 | GPIO_PIN_10 | GPIO_PIN_11 | GPIO_PIN_12 |
            GPIO_PIN_13 | GPIO_PIN_14 | GPIO_PIN_15;
    HAL_GPIO_Init(GPIOJ, &g);

    g.Pin = GPIO_PIN_0 | GPIO_PIN_1 | GPIO_PIN_2 | GPIO_PIN_3 | GPIO_PIN_4 | GPIO_PIN_5 |
            GPIO_PIN_6 | GPIO_PIN_7;
    HAL_GPIO_Init(GPIOK, &g);

    g.Pin = GPIO_PIN_1 | GPIO_PIN_9;
    HAL_GPIO_Init(GPIOH, &g);

    lcd_gpio_out(GPIOD, GPIO_PIN_7, GPIO_PIN_SET);
    lcd_gpio_out(GPIOK, GPIO_PIN_7, GPIO_PIN_SET);
    lcd_gpio_out(GPIOK, GPIO_PIN_0, GPIO_PIN_SET);
}

void HAL_DMA2D_MspInit(DMA2D_HandleTypeDef *hdma2d)
{
    (void)hdma2d;
    __HAL_RCC_DMA2D_CLK_ENABLE();
}

static err_t lcd_pixel_clock(void)
{
    RCC_PeriphCLKInitTypeDef p = {0};

    /* 25 MHz / 5 * 160 / 83 ≈ 9.63 MHz (ST disco BSP). */
    p.PeriphClockSelection = RCC_PERIPHCLK_LTDC;
    p.PLL3.PLL3M = 5;
    p.PLL3.PLL3N = 160;
    p.PLL3.PLL3P = 2;
    p.PLL3.PLL3Q = 2;
    p.PLL3.PLL3R = 83;
    p.PLL3.PLL3RGE = RCC_PLL3VCIRANGE_2;
    p.PLL3.PLL3VCOSEL = RCC_PLL3VCOWIDE;
    p.PLL3.PLL3FRACN = 0;
    return cube_err(HAL_RCCEx_PeriphCLKConfig(&p));
}

static void fb_clear(uint16_t *fb, uint16_t color)
{
    uint32_t i;
    uint32_t n = (uint32_t)BOARD_LCD_W * (uint32_t)BOARD_LCD_H;

    for (i = 0u; i < n; i++) {
        fb[i] = color;
    }
    SCB_CleanDCache_by_Addr((uint32_t *)(uintptr_t)fb, (int32_t)BOARD_FB_BYTES);
}

static void dma2d_output(uint32_t mode, uint32_t offset)
{
    g_dma2d.Instance = DMA2D;
    g_dma2d.Init.Mode = mode;
    g_dma2d.Init.ColorMode = DMA2D_OUTPUT_RGB565;
    g_dma2d.Init.OutputOffset = offset;
    g_dma2d.Init.AlphaInverted = DMA2D_REGULAR_ALPHA;
    g_dma2d.Init.RedBlueSwap = DMA2D_RB_REGULAR;
    g_dma2d.Init.BytesSwap = DMA2D_BYTES_REGULAR;
    g_dma2d.Init.LineOffsetMode = DMA2D_LOM_PIXELS;
}

static err_t dma2d_fill(uint16_t *dst, uint16_t x, uint16_t y, uint16_t w, uint16_t h,
                        uint16_t color)
{
    uint32_t addr =
        (uint32_t)(uintptr_t)dst + ((uint32_t)y * (uint32_t)BOARD_LCD_W + (uint32_t)x) * 2u;

    dma2d_output(DMA2D_R2M, (uint32_t)BOARD_LCD_W - (uint32_t)w);
    if (HAL_DMA2D_Init(&g_dma2d) != HAL_OK) {
        return ERR_IO;
    }
    if (HAL_DMA2D_Start(&g_dma2d, (uint32_t)color, addr, w, h) != HAL_OK) {
        return ERR_IO;
    }
    return cube_err(HAL_DMA2D_PollForTransfer(&g_dma2d, 100u));
}

static err_t dma2d_copy(const uint16_t *src, uint16_t src_w, uint16_t *dst, uint16_t x, uint16_t y,
                        uint16_t w, uint16_t h)
{
    uint32_t daddr =
        (uint32_t)(uintptr_t)dst + ((uint32_t)y * (uint32_t)BOARD_LCD_W + (uint32_t)x) * 2u;

    dma2d_output(DMA2D_M2M, (uint32_t)BOARD_LCD_W - (uint32_t)w);
    if (HAL_DMA2D_Init(&g_dma2d) != HAL_OK) {
        return ERR_IO;
    }
    g_dma2d.LayerCfg[1].InputOffset = (uint32_t)src_w - (uint32_t)w;
    g_dma2d.LayerCfg[1].InputColorMode = DMA2D_INPUT_RGB565;
    g_dma2d.LayerCfg[1].AlphaMode = DMA2D_NO_MODIF_ALPHA;
    g_dma2d.LayerCfg[1].InputAlpha = 0u;
    g_dma2d.LayerCfg[1].AlphaInverted = DMA2D_REGULAR_ALPHA;
    g_dma2d.LayerCfg[1].RedBlueSwap = DMA2D_RB_REGULAR;
    if (HAL_DMA2D_ConfigLayer(&g_dma2d, 1u) != HAL_OK) {
        return ERR_IO;
    }
    if (HAL_DMA2D_Start(&g_dma2d, (uint32_t)(uintptr_t)src, daddr, w, h) != HAL_OK) {
        return ERR_IO;
    }
    return cube_err(HAL_DMA2D_PollForTransfer(&g_dma2d, 100u));
}

static void cpu_fill(uint16_t *dst, uint16_t x, uint16_t y, uint16_t w, uint16_t h, uint16_t color)
{
    uint16_t row;
    uint16_t col;

    for (row = 0u; row < h; row++) {
        uint16_t *p = dst + ((uint32_t)(y + row) * (uint32_t)BOARD_LCD_W) + x;
        for (col = 0u; col < w; col++) {
            p[col] = color;
        }
    }
    SCB_CleanDCache_by_Addr((uint32_t *)(uintptr_t)dst, (int32_t)BOARD_FB_BYTES);
}

static uint16_t *back_fb(void)
{
    return g_fb[g_front ^ 1u];
}

err_t board_disp_init(void)
{
    LTDC_LayerCfgTypeDef layer = {0};
    err_t e;

    e = lcd_pixel_clock();
    if (e != ERR_OK) {
        return e;
    }

    __HAL_RCC_GPIOA_CLK_ENABLE();
    lcd_gpio_out(GPIOA, GPIO_PIN_2, GPIO_PIN_RESET);
    HAL_Delay(20u);
    HAL_GPIO_WritePin(GPIOA, GPIO_PIN_2, GPIO_PIN_SET);
    HAL_Delay(10u);

    g_fb[0] = (uint16_t *)BOARD_FB0_BASE;
    g_fb[1] = (uint16_t *)BOARD_FB1_BASE;
    g_front = 0u;
    fb_clear(g_fb[0], 0u);
    fb_clear(g_fb[1], 0u);

    g_ltdc.Instance = LTDC;
    g_ltdc.Init.HSPolarity = LTDC_HSPOLARITY_AL;
    g_ltdc.Init.VSPolarity = LTDC_VSPOLARITY_AL;
    g_ltdc.Init.DEPolarity = LTDC_DEPOLARITY_AL;
    g_ltdc.Init.PCPolarity = LTDC_PCPOLARITY_IPC;
    g_ltdc.Init.HorizontalSync = LCD_HSYNC - 1u;
    /* ST BSP quirk: HBP-11 in the porch totals. */
    g_ltdc.Init.AccumulatedHBP = LCD_HSYNC + (LCD_HBP - 11u) - 1u;
    g_ltdc.Init.AccumulatedActiveW = LCD_HSYNC + BOARD_LCD_W + LCD_HBP - 1u;
    g_ltdc.Init.TotalWidth = LCD_HSYNC + BOARD_LCD_W + (LCD_HBP - 11u) + LCD_HFP - 1u;
    g_ltdc.Init.VerticalSync = LCD_VSYNC - 1u;
    g_ltdc.Init.AccumulatedVBP = LCD_VSYNC + LCD_VBP - 1u;
    g_ltdc.Init.AccumulatedActiveH = LCD_VSYNC + BOARD_LCD_H + LCD_VBP - 1u;
    g_ltdc.Init.TotalHeigh = LCD_VSYNC + BOARD_LCD_H + LCD_VBP + LCD_VFP - 1u;
    g_ltdc.Init.Backcolor.Blue = 0xFFu;
    g_ltdc.Init.Backcolor.Green = 0xFFu;
    g_ltdc.Init.Backcolor.Red = 0xFFu;
    if (HAL_LTDC_Init(&g_ltdc) != HAL_OK) {
        return ERR_IO;
    }

    layer.WindowX0 = 0u;
    layer.WindowX1 = BOARD_LCD_W;
    layer.WindowY0 = 0u;
    layer.WindowY1 = BOARD_LCD_H;
    layer.PixelFormat = LTDC_PIXEL_FORMAT_RGB565;
    layer.FBStartAdress = (uint32_t)(uintptr_t)g_fb[0];
    layer.Alpha = 255u;
    layer.Alpha0 = 0u;
    layer.Backcolor.Blue = 0u;
    layer.Backcolor.Green = 0u;
    layer.Backcolor.Red = 0u;
    layer.BlendingFactor1 = LTDC_BLENDING_FACTOR1_PAxCA;
    layer.BlendingFactor2 = LTDC_BLENDING_FACTOR2_PAxCA;
    layer.ImageWidth = BOARD_LCD_W;
    layer.ImageHeight = BOARD_LCD_H;
    if (HAL_LTDC_ConfigLayer(&g_ltdc, &layer, 0u) != HAL_OK) {
        return ERR_IO;
    }

    g_ready = 1u;
    return ERR_OK;
}

void disp_get_info(disp_info_t *info)
{
    if (info == NULL) {
        return;
    }
    info->w = BOARD_LCD_W;
    info->h = BOARD_LCD_H;
    info->stride = BOARD_LCD_W;
    info->fmt = DISP_FMT_RGB565;
}

err_t disp_fill(const disp_rect_t *r, uint16_t rgb565)
{
    disp_rect_t c;
    uint16_t *back;
    err_t e;

    if (!g_ready || r == NULL) {
        return ERR_INVAL;
    }
    c = *r;
    if (disp_clip_rect(&c, BOARD_LCD_W, BOARD_LCD_H) == 0) {
        return ERR_OK;
    }
    back = back_fb();
    e = dma2d_fill(back, c.x, c.y, c.w, c.h, rgb565);
    if (e != ERR_OK) {
        cpu_fill(back, c.x, c.y, c.w, c.h, rgb565);
    }
    return ERR_OK;
}

void disp_flush(const disp_rect_t *r, const void *pixels)
{
    disp_rect_t c;
    const uint16_t *src;
    uint16_t *back;
    err_t e;

    if (!g_ready || r == NULL || pixels == NULL) {
        return;
    }
    c = *r;
    if (disp_clip_rect(&c, BOARD_LCD_W, BOARD_LCD_H) == 0) {
        return;
    }
    src = (const uint16_t *)pixels;
    back = back_fb();
    e = dma2d_copy(src, r->w, back, c.x, c.y, c.w, c.h);
    if (e != ERR_OK) {
        uint16_t row;
        for (row = 0u; row < c.h; row++) {
            memcpy(back + ((uint32_t)(c.y + row) * (uint32_t)BOARD_LCD_W) + c.x,
                   src + ((uint32_t)row * (uint32_t)r->w), (size_t)c.w * 2u);
        }
        SCB_CleanDCache_by_Addr((uint32_t *)(uintptr_t)back, (int32_t)BOARD_FB_BYTES);
    }
}

void disp_swap(void)
{
    uint8_t next;

    if (!g_ready) {
        return;
    }
    next = (uint8_t)(g_front ^ 1u);
    board_disp_show(g_fb[next]);
}

void board_disp_show(const void *fb)
{
    if (!g_ready || fb == NULL) {
        return;
    }
    SCB_CleanDCache_by_Addr((uint32_t *)(uintptr_t)fb, (int32_t)BOARD_FB_BYTES);
    /* Poke SRCR so HAL_LTDC_Reload cannot arm LTDC_IT_RR (no IRQ vector). */
    if (HAL_LTDC_SetAddress_NoReload(&g_ltdc, (uint32_t)(uintptr_t)fb, 0u) != HAL_OK) {
        return;
    }
    g_ltdc.Instance->SRCR = LTDC_SRCR_VBR;
    while (__HAL_LTDC_GET_FLAG(&g_ltdc, LTDC_FLAG_RR) == 0u) {
    }
    __HAL_LTDC_CLEAR_FLAG(&g_ltdc, LTDC_FLAG_RR);
    if (fb == g_fb[0]) {
        g_front = 0u;
    } else if (fb == g_fb[1]) {
        g_front = 1u;
    }
}
