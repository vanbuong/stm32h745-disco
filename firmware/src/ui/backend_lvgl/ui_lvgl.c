#include "ui/backend.h"

#include "app/apps.h"
#include "app/files.h"
#include "app/image_view.h"
#include "lv_port.h"
#include "svc/text_view.h"
#include "ui/launcher.h"
#include "ui/shell.h"
#include "ui/theme.h"

#include "bsp/board.h"

#include "lvgl.h"

#include <stdint.h>
#include <string.h>

static lv_obj_t *s_status;
static lv_obj_t *s_time;
static lv_obj_t *s_stor;
static lv_obj_t *s_m4;
static lv_obj_t *s_content;
static lv_obj_t *s_list;
static uint32_t s_gen = 0xFFFFFFFFu;
static uint32_t s_files_gen = 0xFFFFFFFFu;
static uint32_t s_text_gen = 0xFFFFFFFFu;
static uint32_t s_img_gen = 0xFFFFFFFFu;
static uint8_t s_last_min = 0xFFu;
static uint8_t s_last_m4 = 0xFFu;
static uint8_t s_last_stor = 0xFFu;
static lv_image_dsc_t s_img_dsc;

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
    const char *id;

    if (lv_event_get_code(e) != LV_EVENT_CLICKED) {
        return;
    }
    id = shell_top_id();
    if (id != NULL && strcmp(id, APP_ID_FILES) == 0) {
        if (s_list != NULL) {
            files_set_scroll(lv_obj_get_scroll_y(s_list));
        }
        if (files_state() == FILES_ST_PROMPT) {
            files_prompt_cancel();
            return;
        }
        if (files_back() != 0) {
            log_nav("files", files_cwd());
            return;
        }
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

static lv_obj_t *make_bar(const char *title)
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

    add_label(bar, (title != NULL) ? title : "", THEME_HIT_MIN_PX + 8, 8, 360, 24, THEME_TEXT,
              LV_FONT_DEFAULT);
    return bar;
}

static void build_launcher(void)
{
    unsigned i;

    s_list = NULL;
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

static void files_row_cb(lv_event_t *e)
{
    unsigned idx;

    if (lv_event_get_code(e) != LV_EVENT_CLICKED) {
        return;
    }
    if (s_list != NULL) {
        files_set_scroll(lv_obj_get_scroll_y(s_list));
    }
    idx = (unsigned)(uintptr_t)lv_event_get_user_data(e);
    (void)files_on_row(idx);
    if (shell_top_id() != NULL && strcmp(shell_top_id(), APP_ID_FILES) != 0) {
        log_nav("shell_push", shell_top_id());
    } else {
        log_nav("files", files_cwd());
    }
}

static void files_retry_cb(lv_event_t *e)
{
    if (lv_event_get_code(e) != LV_EVENT_CLICKED) {
        return;
    }
    files_reload();
}

static void files_open_text_cb(lv_event_t *e)
{
    if (lv_event_get_code(e) != LV_EVENT_CLICKED) {
        return;
    }
    files_prompt_open_text();
    log_nav("shell_push", shell_top_id());
}

static void files_cancel_cb(lv_event_t *e)
{
    if (lv_event_get_code(e) != LV_EVENT_CLICKED) {
        return;
    }
    files_prompt_cancel();
}

static void text_page_cb(lv_event_t *e)
{
    int dir;

    if (lv_event_get_code(e) != LV_EVENT_CLICKED) {
        return;
    }
    dir = (int)(intptr_t)lv_event_get_user_data(e);
    (void)text_view_page(dir);
}

static void image_nav_cb(lv_event_t *e)
{
    int dir;

    if (lv_event_get_code(e) != LV_EVENT_CLICKED) {
        return;
    }
    dir = (int)(intptr_t)lv_event_get_user_data(e);
    (void)image_view_next(dir);
}

static void put_u32(char *out, size_t n, uint32_t v)
{
    char tmp[11];
    int i = 10;
    size_t o = 0u;

    if (out == NULL || n == 0u) {
        return;
    }
    tmp[10] = '\0';
    if (v == 0u) {
        tmp[--i] = '0';
    }
    while (v > 0u && i > 0) {
        tmp[--i] = (char)('0' + (v % 10u));
        v /= 10u;
    }
    while (tmp[i] != '\0' && o + 1u < n) {
        out[o++] = tmp[i++];
    }
    out[o] = '\0';
}

static void add_nav_btn(lv_obj_t *bar, int32_t x, const char *txt, lv_event_cb_t cb, int dir)
{
    lv_obj_t *btn = lv_button_create(bar);
    lv_obj_t *lab;

    lv_obj_set_pos(btn, x, 0);
    lv_obj_set_size(btn, THEME_HIT_MIN_PX, THEME_APPBAR_H);
    lv_obj_add_event_cb(btn, cb, LV_EVENT_CLICKED, (void *)(intptr_t)dir);
    lab = lv_label_create(btn);
    lv_label_set_text(lab, txt);
    lv_obj_center(lab);
}

static void row_label(char *out, size_t n, const files_row_t *r)
{
    char sz[12];
    const char *tag;
    size_t o = 0u;
    const char *p;

    if (out == NULL || n == 0u || r == NULL) {
        return;
    }
    tag = files_kind_tag(r);
    while (*tag != '\0' && o + 1u < n) {
        out[o++] = *tag++;
    }
    if (o + 2u < n) {
        out[o++] = ' ';
        out[o++] = ' ';
    }
    p = r->name;
    while (*p != '\0' && o + 1u < n) {
        out[o++] = *p++;
    }
    if (r->is_dir != 0u && o + 1u < n) {
        out[o++] = '/';
    }
    if (r->is_dir == 0u) {
        files_format_size(r->size, sz, sizeof(sz));
        if (o + 2u < n) {
            out[o++] = ' ';
            out[o++] = ' ';
        }
        p = sz;
        while (*p != '\0' && o + 1u < n) {
            out[o++] = *p++;
        }
    }
    out[o] = '\0';
}

static void build_files(void)
{
    files_state_t st = files_state();
    const char *cwd = files_cwd();

    make_bar((cwd != NULL && cwd[0] != '\0') ? cwd : "Files");
    if (st == FILES_ST_UNMOUNTED || st == FILES_ST_IO) {
        add_label(s_content, "Storage not ready", 16, THEME_APPBAR_H + 16, 440, 24, THEME_ERR,
                  LV_FONT_DEFAULT);
        {
            lv_obj_t *btn = lv_button_create(s_content);
            lv_obj_t *lab;
            lv_obj_set_pos(btn, 16, THEME_APPBAR_H + 56);
            lv_obj_set_size(btn, 120, THEME_HIT_MIN_PX);
            lv_obj_add_event_cb(btn, files_retry_cb, LV_EVENT_CLICKED, NULL);
            lab = lv_label_create(btn);
            lv_label_set_text(lab, "Retry");
            lv_obj_center(lab);
        }
        return;
    }
    if (st == FILES_ST_PROMPT) {
        add_label(s_content, files_prompt_name(), 16, THEME_APPBAR_H + 12, 440, 24, THEME_TEXT,
                  LV_FONT_DEFAULT);
        add_label(s_content, "No app for this file", 16, THEME_APPBAR_H + 40, 440, 24, THEME_MUTED,
                  &lv_font_montserrat_12);
        {
            lv_obj_t *open = lv_button_create(s_content);
            lv_obj_t *cancel = lv_button_create(s_content);
            lv_obj_t *lab;
            lv_obj_set_pos(open, 16, THEME_APPBAR_H + 80);
            lv_obj_set_size(open, 160, THEME_HIT_MIN_PX);
            lv_obj_add_event_cb(open, files_open_text_cb, LV_EVENT_CLICKED, NULL);
            lab = lv_label_create(open);
            lv_label_set_text(lab, "Open as text");
            lv_obj_center(lab);
            lv_obj_set_pos(cancel, 192, THEME_APPBAR_H + 80);
            lv_obj_set_size(cancel, 100, THEME_HIT_MIN_PX);
            lv_obj_add_event_cb(cancel, files_cancel_cb, LV_EVENT_CLICKED, NULL);
            lab = lv_label_create(cancel);
            lv_label_set_text(lab, "Cancel");
            lv_obj_center(lab);
        }
        return;
    }
    if (st == FILES_ST_EMPTY) {
        add_label(s_content, "No files", 16, THEME_APPBAR_H + 16, 440, 24, THEME_MUTED,
                  LV_FONT_DEFAULT);
        return;
    }

    s_list = lv_list_create(s_content);
    lv_obj_set_pos(s_list, 0, THEME_APPBAR_H);
    lv_obj_set_size(s_list, THEME_PANEL_W, THEME_CONTENT_H - THEME_APPBAR_H);
    lv_obj_set_style_bg_color(s_list, lv_color_hex(THEME_BG), 0);
    lv_obj_set_style_border_width(s_list, 0, 0);
    lv_obj_set_style_pad_all(s_list, 0, 0);
    {
        unsigned i;
        for (i = 0u; i < files_count(); i++) {
            const files_row_t *r = files_row(i);
            char line[96];
            lv_obj_t *btn;

            row_label(line, sizeof(line), r);
            btn = lv_list_add_button(s_list, NULL, line);
            lv_obj_set_height(btn, THEME_ROW_H);
            lv_obj_add_event_cb(btn, files_row_cb, LV_EVENT_CLICKED, (void *)(uintptr_t)i);
        }
    }
    lv_obj_scroll_to_y(s_list, files_scroll(), LV_ANIM_OFF);
}

static void build_text(void)
{
    lv_obj_t *bar;
    lv_obj_t *box;
    lv_obj_t *lab;
    char title[48];
    const char *name = text_view_name();
    uint32_t pct = text_view_progress();
    size_t o = 0u;
    const char *p;

    title[0] = '\0';
    p = (name != NULL && name[0] != '\0') ? name : "Text";
    while (*p != '\0' && o + 1u < sizeof(title)) {
        title[o++] = *p++;
    }
    if (o + 2u < sizeof(title)) {
        title[o++] = ' ';
    }
    {
        char num[12];
        put_u32(num, sizeof(num), pct);
        p = num;
        while (*p != '\0' && o + 1u < sizeof(title)) {
            title[o++] = *p++;
        }
        if (o + 1u < sizeof(title)) {
            title[o++] = '%';
        }
    }
    title[o] = '\0';
    bar = make_bar(title);
    if (text_view_size() > TEXT_WIN_MAX) {
        add_nav_btn(bar, THEME_PANEL_W - 2 * THEME_HIT_MIN_PX, "<", text_page_cb, -1);
        add_nav_btn(bar, THEME_PANEL_W - THEME_HIT_MIN_PX, ">", text_page_cb, 1);
    }
    box = lv_obj_create(s_content);
    lv_obj_set_pos(box, 0, THEME_APPBAR_H);
    lv_obj_set_size(box, THEME_PANEL_W, THEME_CONTENT_H - THEME_APPBAR_H);
    lv_obj_set_style_bg_color(box, lv_color_hex(THEME_BG), 0);
    lv_obj_set_style_border_width(box, 0, 0);
    lv_obj_set_style_pad_all(box, 8, 0);
    lab = lv_label_create(box);
    lv_label_set_long_mode(lab, LV_LABEL_LONG_WRAP);
    lv_obj_set_width(lab, THEME_PANEL_W - 16);
    lv_label_set_text(lab, text_view_text());
    lv_obj_set_style_text_color(lab, lv_color_hex(THEME_TEXT), 0);
}

static void build_image(void)
{
    lv_obj_t *bar;
    const char *name = image_view_name();

    bar = make_bar((name != NULL && name[0] != '\0') ? name : "Image");
    add_nav_btn(bar, THEME_PANEL_W - 2 * THEME_HIT_MIN_PX, "<", image_nav_cb, -1);
    add_nav_btn(bar, THEME_PANEL_W - THEME_HIT_MIN_PX, ">", image_nav_cb, 1);
    if (image_view_status() != ERR_OK || image_view_pixels() == NULL || image_view_w() == 0u) {
        add_label(s_content, "Can't open image", 16, THEME_APPBAR_H + 16, 440, 24, THEME_ERR,
                  LV_FONT_DEFAULT);
        add_label(s_content,
                  (image_view_err_str()[0] != '\0') ? image_view_err_str() : "truncated or corrupt",
                  16, THEME_APPBAR_H + 48, 440, 24, THEME_MUTED, &lv_font_montserrat_12);
        return;
    }
    memset(&s_img_dsc, 0, sizeof(s_img_dsc));
    s_img_dsc.header.magic = LV_IMAGE_HEADER_MAGIC;
    s_img_dsc.header.cf = LV_COLOR_FORMAT_RGB565;
    s_img_dsc.header.w = image_view_w();
    s_img_dsc.header.h = image_view_h();
    s_img_dsc.header.stride = (uint32_t)image_view_stride() * 2u;
    s_img_dsc.data_size = (uint32_t)image_view_stride() * (uint32_t)image_view_h() * 2u;
    s_img_dsc.data = (const uint8_t *)image_view_pixels();
    {
        lv_obj_t *img = lv_image_create(s_content);
        lv_obj_set_pos(img, 0, THEME_APPBAR_H);
        lv_image_set_src(img, &s_img_dsc);
    }
}

static void build_app(const char *id, const char *title)
{
    const char *path;

    s_list = NULL;
    if (id != NULL && strcmp(id, APP_ID_TEXT) == 0) {
        build_text();
        return;
    }
    if (id != NULL && strcmp(id, APP_ID_IMAGE) == 0) {
        build_image();
        return;
    }
    make_bar((title != NULL) ? title : "");
    if (id != NULL && strcmp(id, APP_ID_FILES) == 0) {
        build_files();
        return;
    }
    path = (const char *)shell_top_args();
    if (path != NULL && path[0] != '\0') {
        add_label(s_content, path, 16, THEME_APPBAR_H + 16, 448, 24, THEME_TEXT, LV_FONT_DEFAULT);
        add_label(s_content, "stub viewer", 16, THEME_APPBAR_H + 48, 200, 24, THEME_MUTED,
                  &lv_font_montserrat_12);
    } else {
        add_label(s_content, "stub", 16, THEME_APPBAR_H + 16, 200, 24, THEME_MUTED,
                  LV_FONT_DEFAULT);
    }
}

static void rebuild_content(void)
{
    const char *id;

    s_list = NULL;
    lv_obj_clean(s_content);
    id = shell_top_id();
    if (id == NULL) {
        build_launcher();
    } else if (strcmp(id, APP_ID_FILES) == 0) {
        build_files();
    } else {
        build_app(id, shell_top_title());
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
    lv_obj_set_style_text_color(s_m4, lv_color_hex((st->m4 != 0u) ? THEME_OK : THEME_ERR), 0);
    s_last_min = st->min;
    s_last_m4 = st->m4;
    s_last_stor = st->storage_ok;
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

    s_m4 = lv_label_create(s_status);
    lv_label_set_text(s_m4, "M4");
    lv_obj_set_pos(s_m4, 140, 8);
    lv_obj_set_style_text_font(s_m4, &lv_font_montserrat_12, 0);

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
    s_files_gen = 0xFFFFFFFFu;
    s_text_gen = 0xFFFFFFFFu;
    s_img_gen = 0xFFFFFFFFu;
    refresh_status();
    rebuild_content();
    s_gen = shell_nav_gen();
    s_files_gen = files_view_gen();
    s_text_gen = text_view_gen();
    s_img_gen = image_view_gen();
    return ERR_OK;
}

void ui_backend_handler(void)
{
    uint32_t gen = shell_nav_gen();
    uint32_t fgen = files_view_gen();
    uint32_t tgen = text_view_gen();
    uint32_t igen = image_view_gen();

    if (gen != s_gen || fgen != s_files_gen || tgen != s_text_gen || igen != s_img_gen) {
        s_gen = gen;
        s_files_gen = fgen;
        s_text_gen = tgen;
        s_img_gen = igen;
        rebuild_content();
    }
    if (shell_status()->min != s_last_min || shell_status()->m4 != s_last_m4 ||
        shell_status()->storage_ok != s_last_stor) {
        refresh_status();
    }
    lv_timer_handler();
}

uint32_t ui_backend_frames(void)
{
    return lv_port_frames();
}
