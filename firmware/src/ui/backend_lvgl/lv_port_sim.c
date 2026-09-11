#include "lv_port.h"

#include "ui/theme.h"

#include "lvgl.h"
#include "src/drivers/sdl/lv_sdl_mouse.h"
#include "src/drivers/sdl/lv_sdl_window.h"

static uint32_t g_frames;

err_t lv_port_init(void)
{
    lv_display_t *disp;
    lv_indev_t *mouse;
    lv_theme_t *th;

    lv_init();

    disp = lv_sdl_window_create((int32_t)THEME_PANEL_W, (int32_t)THEME_PANEL_H);
    if (disp == NULL) {
        return ERR_NOMEM;
    }
    lv_sdl_window_set_title(disp, "stm32h745-disco");
    lv_sdl_window_set_resizeable(disp, false);
    lv_sdl_window_set_zoom(disp, 2.0f);

    th = lv_theme_default_init(disp, lv_color_hex(0x3D8BFFu), lv_color_hex(0x1C212Cu), true,
                               LV_FONT_DEFAULT);
    if (th != NULL) {
        lv_display_set_theme(disp, th);
    }

    mouse = lv_sdl_mouse_create();
    if (mouse == NULL) {
        return ERR_NOMEM;
    }

    g_frames = 0u;
    return ERR_OK;
}

uint32_t lv_port_frames(void)
{
    return g_frames;
}
