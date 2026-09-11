#include "bsp/board.h"
#include "hal/input.h"

#include "lvgl.h"

#if LV_MEM_ADR != BOARD_LVGL_MEM_BASE
#error "lv_conf.h LV_MEM_ADR must match BOARD_LVGL_MEM_BASE"
#endif
#if LV_MEM_SIZE != BOARD_LVGL_MEM_BYTES
#error "lv_conf.h LV_MEM_SIZE must match BOARD_LVGL_MEM_BYTES"
#endif

static uint32_t g_frames;
static uint8_t g_ptr_down;
static int16_t g_ptr_x;
static int16_t g_ptr_y;

static uint32_t tick_ms(void)
{
    return board_millis();
}

static void flush_cb(lv_display_t *disp, const lv_area_t *area, uint8_t *px)
{
    (void)area;
    board_disp_show(px);
    if (lv_display_flush_is_last(disp) != 0) {
        g_frames++;
    }
    lv_display_flush_ready(disp);
}

static void indev_read(lv_indev_t *indev, lv_indev_data_t *data)
{
    input_event_t ev;

    (void)indev;
    while (input_poll(&ev)) {
        if (ev.kind == INPUT_PTR_DOWN || ev.kind == INPUT_PTR_MOVE) {
            g_ptr_down = 1u;
            g_ptr_x = ev.x;
            g_ptr_y = ev.y;
        } else if (ev.kind == INPUT_PTR_UP) {
            g_ptr_down = 0u;
            g_ptr_x = ev.x;
            g_ptr_y = ev.y;
        }
    }
    data->point.x = g_ptr_x;
    data->point.y = g_ptr_y;
    data->state = (g_ptr_down != 0u) ? LV_INDEV_STATE_PRESSED : LV_INDEV_STATE_RELEASED;
}

err_t lv_port_init(void)
{
    lv_display_t *disp;
    lv_indev_t *indev;
    lv_theme_t *th;
    void *fb0 = (void *)BOARD_FB0_BASE;
    void *fb1 = (void *)BOARD_FB1_BASE;

    lv_init();
    lv_tick_set_cb(tick_ms);

    disp = lv_display_create((int32_t)BOARD_LCD_W, (int32_t)BOARD_LCD_H);
    if (disp == NULL) {
        return ERR_NOMEM;
    }
    lv_display_set_color_format(disp, LV_COLOR_FORMAT_RGB565);
    lv_display_set_buffers(disp, fb0, fb1, BOARD_FB_BYTES, LV_DISPLAY_RENDER_MODE_DIRECT);
    lv_display_set_flush_cb(disp, flush_cb);

    th = lv_theme_default_init(disp, lv_color_hex(0x3D8BFFu), lv_color_hex(0x1C212Cu), true,
                               LV_FONT_DEFAULT);
    if (th != NULL) {
        lv_display_set_theme(disp, th);
    }

    indev = lv_indev_create();
    if (indev == NULL) {
        return ERR_NOMEM;
    }
    lv_indev_set_type(indev, LV_INDEV_TYPE_POINTER);
    lv_indev_set_read_cb(indev, indev_read);

    g_frames = 0u;
    g_ptr_down = 0u;
    g_ptr_x = 0;
    g_ptr_y = 0;
    return ERR_OK;
}

uint32_t lv_port_frames(void)
{
    return g_frames;
}
