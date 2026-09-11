#include "ui/backend.h"

#include "app/apps.h"
#include "lv_port.h"
#include "ui/launcher.h"
#include "ui/shell.h"
#include "ui/theme.h"

#include "bsp/board.h"

#include "lvgl.h"

#include <string.h>

static lv_obj_t *s_status;
static lv_obj_t *s_time;
static lv_obj_t *s_stor;
static lv_obj_t *s_content;
static uint32_t s_gen = 0xFFFFFFFFu;
static uint8_t s_last_min = 0xFFu;

static void log_nav(const char *op, const char *id)
{
    board_console_puts(op);
    if (id != NULL) {
        board_console_puts(" ");
        board_console_puts(id);
    }
    board_console_puts("\r\n");
}

static void style_bar(lv_obj_t *obj)
{
    lv_obj_set_style_bg_color(obj, lv_color_hex(THEME_SURFACE), 0);
    lv_obj_set_style_bg_opa(obj, LV_OPA_COVER, 0);
    lv_obj_set_style_border_width(obj, 0, 0);
    lv_obj_set_style_pad_all(obj, 0, 0);
    lv_obj_set_style_radius(obj, 0, 0);
}

static void tile_cb(lv_event_t *e)
{
    const ui_app_t *app;

    if (lv_event_get_code(e) != LV_EVENT_CLICKED) {
        return;
    }
    app = (const ui_app_t *)lv_event_get_user_data(e);
    if (app == NULL) {
        return;
    }
    if (shell_push(app->id, NULL) == ERR_OK) {
        log_nav("shell_push", app->id);
    }
}

static void back_cb(lv_event_t *e)
{
    if (lv_event_get_code(e) != LV_EVENT_CLICKED) {
        return;
    }
    (void)shell_pop();
    log_nav("shell_pop", shell_top_id());
}

static void add_label(lv_obj_t *parent, const char *txt, int32_t x, int32_t y, int32_t w, int32_t h,
                      uint32_t color, const lv_font_t *font)
{
    lv_obj_t *lab = lv_label_create(parent);

    lv_label_set_text(lab, txt);
    lv_obj_set_pos(lab, x, y);
    if (w > 0) {
        lv_obj_set_width(lab, w);
    }
    if (h > 0) {
        lv_obj_set_height(lab, h);
    }
    lv_obj_set_style_text_color(lab, lv_color_hex(color), 0);
    lv_obj_set_style_text_font(lab, font, 0);
}

static void build_launcher(void)
{
    unsigned i;

    for (i = 0u; i < LAUNCHER_COUNT; i++) {
        ui_rect_t r;
        const ui_app_t *app = apps_at(i);
        lv_obj_t *btn;
        lv_obj_t *lab;

        if (app == NULL) {
            continue;
        }
        launcher_tile_rect(i, &r);
        btn = lv_button_create(s_content);
        lv_obj_set_pos(btn, (int32_t)r.x, (int32_t)(r.y - THEME_STATUS_H));
        lv_obj_set_size(btn, (int32_t)r.w, (int32_t)r.h);
        lv_obj_set_style_bg_color(btn, lv_color_hex(THEME_SURFACE_2), 0);
        lv_obj_set_style_radius(btn, THEME_RADIUS_PX, 0);
        lv_obj_add_event_cb(btn, tile_cb, LV_EVENT_CLICKED, (void *)app);
        lab = lv_label_create(btn);
        lv_label_set_text(lab, app->title);
        lv_obj_set_style_text_color(lab, lv_color_hex(THEME_TEXT), 0);
        lv_obj_center(lab);
    }
}

static void build_app(const char *title)
{
    lv_obj_t *bar;
    lv_obj_t *back;
    lv_obj_t *lab;

    bar = lv_obj_create(s_content);
    lv_obj_set_pos(bar, 0, 0);
    lv_obj_set_size(bar, THEME_PANEL_W, THEME_APPBAR_H);
    style_bar(bar);
    lv_obj_set_style_bg_color(bar, lv_color_hex(THEME_SURFACE_2), 0);

    back = lv_button_create(bar);
    lv_obj_set_pos(back, 0, 0);
    lv_obj_set_size(back, THEME_HIT_MIN_PX, THEME_APPBAR_H);
    lv_obj_add_event_cb(back, back_cb, LV_EVENT_CLICKED, NULL);
    lab = lv_label_create(back);
    lv_label_set_text(lab, "<");
    lv_obj_center(lab);

    add_label(bar, (title != NULL) ? title : "", THEME_HIT_MIN_PX + 8, 8, 300, 24, THEME_TEXT,
              LV_FONT_DEFAULT);

    if (shell_top_id() != NULL && strcmp(shell_top_id(), APP_ID_FILES) == 0) {
        unsigned n = app_files_stub_count();
        unsigned i;
        lv_obj_t *list = lv_list_create(s_content);

        lv_obj_set_pos(list, 0, THEME_APPBAR_H);
        lv_obj_set_size(list, THEME_PANEL_W, THEME_CONTENT_H - THEME_APPBAR_H);
        lv_obj_set_style_bg_color(list, lv_color_hex(THEME_BG), 0);
        lv_obj_set_style_border_width(list, 0, 0);
        for (i = 0u; i < n; i++) {
            const char *row = app_files_stub_row(i);
            if (row != NULL) {
                (void)lv_list_add_text(list, row);
            }
        }
    } else {
        add_label(s_content, "stub", 16, THEME_APPBAR_H + 16, 200, 24, THEME_MUTED,
                  LV_FONT_DEFAULT);
    }
}

static void rebuild_content(void)
{
    const char *id;

    lv_obj_clean(s_content);
    id = shell_top_id();
    if (id == NULL) {
        build_launcher();
    } else {
        build_app(shell_top_title());
    }
}

static void refresh_status(void)
{
    const shell_status_t *st = shell_status();
    char clock[6];

    clock[0] = (char)('0' + (st->hour / 10u));
    clock[1] = (char)('0' + (st->hour % 10u));
    clock[2] = ':';
    clock[3] = (char)('0' + (st->min / 10u));
    clock[4] = (char)('0' + (st->min % 10u));
    clock[5] = '\0';
    lv_label_set_text(s_time, clock);
    lv_obj_set_style_text_color(s_stor, lv_color_hex((st->storage_ok != 0u) ? THEME_OK : THEME_ERR),
                                0);
    s_last_min = st->min;
}

err_t ui_backend_init(void)
{
    lv_obj_t *scr;
    err_t e;

    e = lv_port_init();
    if (e != ERR_OK) {
        return e;
    }

    scr = lv_screen_active();
    lv_obj_set_style_bg_color(scr, lv_color_hex(THEME_BG), 0);
    lv_obj_set_style_bg_opa(scr, LV_OPA_COVER, 0);
    lv_obj_clear_flag(scr, LV_OBJ_FLAG_SCROLLABLE);

    s_status = lv_obj_create(scr);
    lv_obj_set_pos(s_status, 0, 0);
    lv_obj_set_size(s_status, THEME_PANEL_W, THEME_STATUS_H);
    style_bar(s_status);

    s_time = lv_label_create(s_status);
    lv_obj_set_pos(s_time, 8, 8);
    lv_obj_set_style_text_font(s_time, &lv_font_montserrat_12, 0);
    lv_obj_set_style_text_color(s_time, lv_color_hex(THEME_TEXT), 0);

    s_stor = lv_label_create(s_status);
    lv_label_set_text(s_stor, "eMMC");
    lv_obj_set_pos(s_stor, 80, 8);
    lv_obj_set_style_text_font(s_stor, &lv_font_montserrat_12, 0);

    s_content = lv_obj_create(scr);
    lv_obj_set_pos(s_content, 0, THEME_STATUS_H);
    lv_obj_set_size(s_content, THEME_PANEL_W, THEME_CONTENT_H);
    lv_obj_set_style_bg_color(s_content, lv_color_hex(THEME_BG), 0);
    lv_obj_set_style_bg_opa(s_content, LV_OPA_COVER, 0);
    lv_obj_set_style_border_width(s_content, 0, 0);
    lv_obj_set_style_pad_all(s_content, 0, 0);
    lv_obj_set_style_radius(s_content, 0, 0);
    lv_obj_clear_flag(s_content, LV_OBJ_FLAG_SCROLLABLE);

    s_gen = 0xFFFFFFFFu;
    refresh_status();
    rebuild_content();
    s_gen = shell_nav_gen();
    return ERR_OK;
}

void ui_backend_handler(void)
{
    uint32_t gen = shell_nav_gen();

    if (gen != s_gen) {
        s_gen = gen;
        rebuild_content();
    }
    if (shell_status()->min != s_last_min) {
        refresh_status();
    }
    lv_timer_handler();
}

uint32_t ui_backend_frames(void)
{
    return lv_port_frames();
}
