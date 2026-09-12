#include "ui/backend.h"

#include "app/apps.h"
#include "app/calendar.h"
#include "app/files.h"
#include "app/game.h"
#include "app/home.h"
#include "app/image_view.h"
#include "app/network.h"
#include "app/player.h"
#include "lv_port.h"
#include "svc/audio.h"
#include "svc/text_view.h"
#include "svc/time.h"
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
static lv_obj_t *s_eth;
static lv_obj_t *s_zb;
static lv_obj_t *s_nowplay;
static lv_obj_t *s_np_title;
static lv_obj_t *s_np_btn;
static lv_obj_t *s_content;
static lv_obj_t *s_list;
static uint32_t s_gen = 0xFFFFFFFFu;
static uint32_t s_files_gen = 0xFFFFFFFFu;
static uint32_t s_text_gen = 0xFFFFFFFFu;
static uint32_t s_img_gen = 0xFFFFFFFFu;
static uint8_t s_last_min = 0xFFu;
static uint8_t s_last_m4 = 0xFFu;
static uint8_t s_last_stor = 0xFFu;
static uint8_t s_last_net = 0xFFu;
static uint8_t s_last_audio = 0xFFu;
static uint32_t s_net_gen = 0xFFFFFFFFu;
static lv_obj_t *s_pl_title;
static lv_obj_t *s_pl_elapsed;
static lv_obj_t *s_pl_dur;
static lv_obj_t *s_pl_state;
static lv_obj_t *s_pl_vol;
static lv_obj_t *s_pl_toggle;
static lv_obj_t *s_pl_fill;
static lv_obj_t *s_cal_clock;
static lv_obj_t *s_cal_date;
static lv_obj_t *s_cal_ntp;
static uint32_t s_cal_gen = 0xFFFFFFFFu;
static lv_obj_t *s_game_img;
static lv_obj_t *s_game_score;
static lv_obj_t *s_game_high;
static lv_obj_t *s_game_lives;
static uint32_t s_game_gen = 0xFFFFFFFFu;
static uint32_t s_home_gen = 0xFFFFFFFFu;
static lv_obj_t *s_home_banner;
static uint8_t s_last_zb = 0xFFu;
static lv_image_dsc_t s_img_dsc;

static lv_obj_t *add_label(lv_obj_t *parent, const char *txt, int32_t x, int32_t y, int32_t w,
                           int32_t h, uint32_t color, const lv_font_t *font);

static int32_t content_h(void)
{
    const char *id = shell_top_id();
    int32_t h = (int32_t)THEME_CONTENT_H;

    if (audio_active() != 0u && (id == NULL || strcmp(id, APP_ID_PLAYER) != 0)) {
        h -= (int32_t)THEME_NOWPLAYING_H;
    }
    return h;
}

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
    lv_obj_set_style_shadow_width(obj, 0, 0);
}

static void style_card(lv_obj_t *obj)
{
    lv_obj_set_style_bg_color(obj, lv_color_hex(THEME_SURFACE), 0);
    lv_obj_set_style_bg_opa(obj, LV_OPA_COVER, 0);
    lv_obj_set_style_border_width(obj, 1, 0);
    lv_obj_set_style_border_color(obj, lv_color_hex(THEME_HAIRLINE), 0);
    lv_obj_set_style_radius(obj, 12, 0);
    lv_obj_set_style_pad_all(obj, 0, 0);
    lv_obj_set_style_shadow_width(obj, 0, 0);
    lv_obj_clear_flag(obj, LV_OBJ_FLAG_SCROLLABLE);
}

static void style_round_btn(lv_obj_t *btn, uint32_t color)
{
    lv_obj_set_style_bg_color(btn, lv_color_hex(color), 0);
    lv_obj_set_style_bg_opa(btn, LV_OPA_COVER, 0);
    lv_obj_set_style_radius(btn, LV_RADIUS_CIRCLE, 0);
    lv_obj_set_style_shadow_width(btn, 0, 0);
    lv_obj_set_style_border_width(btn, 0, 0);
    lv_obj_set_style_pad_all(btn, 0, 0);
    lv_obj_clear_flag(btn, LV_OBJ_FLAG_SCROLLABLE);
}

static void style_pill(lv_obj_t *btn, uint32_t color)
{
    style_round_btn(btn, color);
    lv_obj_set_style_radius(btn, 16, 0);
}

static void style_tile_btn(lv_obj_t *btn, uint32_t color)
{
    lv_obj_set_style_bg_color(btn, lv_color_hex(color), 0);
    lv_obj_set_style_bg_opa(btn, LV_OPA_COVER, 0);
    lv_obj_set_style_radius(btn, THEME_TILE_RADIUS_PX, 0);
    lv_obj_set_style_shadow_width(btn, 0, 0);
    lv_obj_set_style_border_width(btn, 0, 0);
    lv_obj_set_style_pad_all(btn, 0, 0);
    lv_obj_clear_flag(btn, LV_OBJ_FLAG_SCROLLABLE);
}

static lv_obj_t *add_card(int32_t x, int32_t y, int32_t w, int32_t h)
{
    lv_obj_t *card = lv_obj_create(s_content);

    lv_obj_set_pos(card, x, y);
    lv_obj_set_size(card, w, h);
    style_card(card);
    return card;
}

static lv_obj_t *add_icon_circle(lv_obj_t *parent, int32_t x, int32_t y, int32_t d, uint32_t color,
                                 const char *symbol)
{
    lv_obj_t *c = lv_obj_create(parent);
    lv_obj_t *lab;

    lv_obj_set_pos(c, x, y);
    lv_obj_set_size(c, d, d);
    style_round_btn(c, color);
    lab = lv_label_create(c);
    lv_label_set_text(lab, symbol);
    lv_obj_set_style_text_color(lab, lv_color_hex(0xFFFFFFu), 0);
    lv_obj_center(lab);
    return c;
}

static lv_obj_t *add_pill_btn(lv_obj_t *parent, int32_t x, int32_t y, int32_t w, int32_t h,
                              const char *txt, uint32_t bg, uint32_t fg, lv_event_cb_t cb)
{
    lv_obj_t *btn = lv_button_create(parent);
    lv_obj_t *lab;

    lv_obj_set_pos(btn, x, y);
    lv_obj_set_size(btn, w, h);
    style_pill(btn, bg);
    if (cb != NULL) {
        lv_obj_add_event_cb(btn, cb, LV_EVENT_CLICKED, NULL);
    }
    lab = lv_label_create(btn);
    lv_label_set_text(lab, txt);
    lv_obj_set_style_text_color(lab, lv_color_hex(fg), 0);
    lv_obj_center(lab);
    return btn;
}

static void add_message_card(int32_t y, const char *title, uint32_t title_color, const char *body)
{
    lv_obj_t *card = add_card(16, y, 448, 80);

    add_label(card, title, 16, 14, 416, 24, title_color, LV_FONT_DEFAULT);
    add_label(card, body, 16, 44, 416, 24, THEME_MUTED, &lv_font_montserrat_12);
}

static uint32_t tile_color(const char *id)
{
    if (id == NULL) {
        return THEME_ACCENT;
    }
    if (strcmp(id, APP_ID_FILES) == 0) {
        return THEME_TILE_FILES;
    }
    if (strcmp(id, APP_ID_HOME) == 0) {
        return THEME_TILE_HOME;
    }
    if (strcmp(id, APP_ID_GAME) == 0) {
        return THEME_TILE_GAME;
    }
    if (strcmp(id, APP_ID_PLAYER) == 0) {
        return THEME_TILE_MUSIC;
    }
    if (strcmp(id, APP_ID_NETWORK) == 0) {
        return THEME_TILE_NET;
    }
    if (strcmp(id, APP_ID_CALENDAR) == 0) {
        return THEME_TILE_CAL;
    }
    if (strcmp(id, APP_ID_SETTINGS) == 0) {
        return THEME_TILE_SET;
    }
    return THEME_ACCENT;
}

static const char *tile_symbol(const char *id)
{
    if (id == NULL) {
        return LV_SYMBOL_FILE;
    }
    if (strcmp(id, APP_ID_FILES) == 0) {
        return LV_SYMBOL_DIRECTORY;
    }
    if (strcmp(id, APP_ID_HOME) == 0) {
        return LV_SYMBOL_HOME;
    }
    if (strcmp(id, APP_ID_GAME) == 0) {
        return LV_SYMBOL_PLAY;
    }
    if (strcmp(id, APP_ID_PLAYER) == 0) {
        return LV_SYMBOL_AUDIO;
    }
    if (strcmp(id, APP_ID_NETWORK) == 0) {
        return LV_SYMBOL_WIFI;
    }
    if (strcmp(id, APP_ID_CALENDAR) == 0) {
        return LV_SYMBOL_BARS;
    }
    if (strcmp(id, APP_ID_SETTINGS) == 0) {
        return LV_SYMBOL_SETTINGS;
    }
    return LV_SYMBOL_FILE;
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
    if (id != NULL && strcmp(id, APP_ID_GAME) == 0) {
        if (game_on_back() != 0u) {
            log_nav("game", game_in_library() ? "library" : "pause");
            return;
        }
    }
    if (id != NULL && strcmp(id, APP_ID_HOME) == 0) {
        if (home_app_on_back() != 0u) {
            log_nav("home", home_app_title());
            return;
        }
    }
    (void)shell_pop();
    log_nav("shell_pop", shell_top_id());
}

static void label_set(lv_obj_t *lab, const char *txt)
{
    const char *cur;

    if (lab == NULL || txt == NULL) {
        return;
    }
    cur = lv_label_get_text(lab);
    if (cur != NULL && strcmp(cur, txt) == 0) {
        return;
    }
    lv_label_set_text(lab, txt);
}

static lv_obj_t *add_label(lv_obj_t *parent, const char *txt, int32_t x, int32_t y, int32_t w,
                           int32_t h, uint32_t color, const lv_font_t *font)
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
    return lab;
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
    lv_obj_set_style_bg_color(bar, lv_color_hex(THEME_SURFACE), 0);
    lv_obj_set_style_border_width(bar, 1, 0);
    lv_obj_set_style_border_side(bar, LV_BORDER_SIDE_BOTTOM, 0);
    lv_obj_set_style_border_color(bar, lv_color_hex(THEME_HAIRLINE), 0);

    back = lv_button_create(bar);
    lv_obj_set_pos(back, 8, 4);
    lv_obj_set_size(back, 32, 32);
    style_round_btn(back, THEME_SURFACE_2);
    lv_obj_add_event_cb(back, back_cb, LV_EVENT_CLICKED, NULL);
    lab = lv_label_create(back);
    lv_label_set_text(lab, LV_SYMBOL_LEFT);
    lv_obj_set_style_text_color(lab, lv_color_hex(THEME_TEXT), 0);
    lv_obj_center(lab);

    add_label(bar, (title != NULL) ? title : "", 48, 10, 360, 24, THEME_TEXT, LV_FONT_DEFAULT);
    return bar;
}

static void build_launcher(void)
{
    unsigned i;
    const shell_status_t *st = shell_status();
    char when[20];

    s_list = NULL;
    add_label(s_content, "What do you want to do today?", 16, 8, 340, 22, THEME_TEXT,
              LV_FONT_DEFAULT);
    when[0] = (char)('0' + (st->hour / 10u));
    when[1] = (char)('0' + (st->hour % 10u));
    when[2] = ':';
    when[3] = (char)('0' + (st->min / 10u));
    when[4] = (char)('0' + (st->min % 10u));
    when[5] = '\0';
    add_label(s_content, when, 380, 8, 84, 18, THEME_MUTED, &lv_font_montserrat_12);

    for (i = 0u; i < LAUNCHER_COUNT; i++) {
        ui_rect_t r;
        const ui_app_t *app = apps_at(i);
        lv_obj_t *cell;
        lv_obj_t *btn;
        lv_obj_t *icon;
        lv_obj_t *lab;
        int32_t x;
        int32_t y;

        if (app == NULL) {
            continue;
        }
        launcher_tile_rect_in(i, (uint16_t)content_h(), &r);
        x = (int32_t)r.x;
        y = (int32_t)(r.y - THEME_STATUS_H);
        cell = lv_obj_create(s_content);
        lv_obj_set_pos(cell, x, y);
        lv_obj_set_size(cell, (int32_t)r.w, (int32_t)r.h);
        lv_obj_set_style_bg_opa(cell, LV_OPA_TRANSP, 0);
        lv_obj_set_style_border_width(cell, 0, 0);
        lv_obj_set_style_pad_all(cell, 0, 0);
        lv_obj_set_style_shadow_width(cell, 0, 0);
        lv_obj_clear_flag(cell, LV_OBJ_FLAG_SCROLLABLE);
        lv_obj_add_flag(cell, LV_OBJ_FLAG_CLICKABLE);
        lv_obj_add_event_cb(cell, tile_cb, LV_EVENT_CLICKED, (void *)app);

        btn = lv_button_create(cell);
        lv_obj_set_pos(btn, 0, 0);
        lv_obj_set_size(btn, (int32_t)r.w, (int32_t)r.h);
        style_tile_btn(btn, tile_color(app->id));
        lv_obj_add_flag(btn, LV_OBJ_FLAG_EVENT_BUBBLE);
        icon = lv_label_create(btn);
        if (strcmp(app->id, APP_ID_CALENDAR) == 0) {
            time_civil_t now;
            char day[4];

            if (time_now(&now) == ERR_OK && now.day >= 1u) {
                if (now.day >= 10u) {
                    day[0] = (char)('0' + ((now.day / 10u) % 10u));
                    day[1] = (char)('0' + (now.day % 10u));
                    day[2] = '\0';
                } else {
                    day[0] = (char)('0' + now.day);
                    day[1] = '\0';
                }
                lv_label_set_text(icon, day);
            } else {
                lv_label_set_text(icon, tile_symbol(app->id));
            }
        } else {
            lv_label_set_text(icon, tile_symbol(app->id));
        }
        lv_obj_set_style_text_font(icon, &lv_font_montserrat_20, 0);
        lv_obj_set_style_text_color(icon, lv_color_hex(0xFFFFFFu), 0);
        lv_obj_align(icon, LV_ALIGN_TOP_MID, 0, 18);

        lab = lv_label_create(btn);
        lv_label_set_text(lab, app->title);
        lv_obj_set_style_text_color(lab, lv_color_hex(0xFFFFFFu), 0);
        lv_obj_set_style_text_font(lab, &lv_font_montserrat_12, 0);
        lv_obj_set_width(lab, (int32_t)r.w);
        lv_obj_set_style_text_align(lab, LV_TEXT_ALIGN_CENTER, 0);
        lv_obj_align(lab, LV_ALIGN_BOTTOM_MID, 0, -10);
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

static void player_nav_cb(lv_event_t *e)
{
    int dir;

    if (lv_event_get_code(e) != LV_EVENT_CLICKED) {
        return;
    }
    dir = (int)(intptr_t)lv_event_get_user_data(e);
    (void)player_next(dir);
}

static void player_toggle_cb(lv_event_t *e)
{
    if (lv_event_get_code(e) != LV_EVENT_CLICKED) {
        return;
    }
    player_toggle();
}

static void player_vol_cb(lv_event_t *e)
{
    int d;
    int v;

    if (lv_event_get_code(e) != LV_EVENT_CLICKED) {
        return;
    }
    d = (int)(intptr_t)lv_event_get_user_data(e);
    v = (int)player_volume() + d;
    if (v < 0) {
        v = 0;
    }
    if (v > 100) {
        v = 100;
    }
    player_set_volume((uint8_t)v);
}

static void mini_open_cb(lv_event_t *e)
{
    const char *path;

    if (lv_event_get_code(e) != LV_EVENT_CLICKED) {
        return;
    }
    path = audio_path();
    if (path == NULL || path[0] == '\0') {
        path = NULL;
    }
    if (shell_push(APP_ID_PLAYER, (void *)path) == ERR_OK) {
        log_nav("shell_push", APP_ID_PLAYER);
    }
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

static lv_obj_t *add_nav_btn(lv_obj_t *bar, int32_t x, const char *txt, lv_event_cb_t cb, int dir)
{
    lv_obj_t *btn = lv_button_create(bar);
    lv_obj_t *lab;

    lv_obj_set_pos(btn, x, 4);
    lv_obj_set_size(btn, 32, 32);
    style_round_btn(btn, THEME_SURFACE_2);
    lv_obj_add_event_cb(btn, cb, LV_EVENT_CLICKED, (void *)(intptr_t)dir);
    lab = lv_label_create(btn);
    lv_label_set_text(lab, txt);
    lv_obj_set_style_text_color(lab, lv_color_hex(THEME_TEXT), 0);
    lv_obj_center(lab);
    return lab;
}

static void add_hit_btn(lv_obj_t *parent, int32_t x, int32_t y, const char *txt, lv_event_cb_t cb,
                        int dir)
{
    lv_obj_t *btn = lv_button_create(parent);
    lv_obj_t *lab;

    lv_obj_set_pos(btn, x, y);
    lv_obj_set_size(btn, 40, 40);
    style_round_btn(btn, THEME_SURFACE_2);
    lv_obj_add_event_cb(btn, cb, LV_EVENT_CLICKED, (void *)(intptr_t)dir);
    lab = lv_label_create(btn);
    lv_label_set_text(lab, txt);
    lv_obj_set_style_text_color(lab, lv_color_hex(THEME_TEXT), 0);
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
        lv_obj_t *card = add_card(16, THEME_APPBAR_H + 12, 448, 120);

        add_icon_circle(card, 16, 20, 48, THEME_ERR, LV_SYMBOL_WARNING);
        add_label(card, "Storage not ready", 80, 18, 340, 24, THEME_TEXT, LV_FONT_DEFAULT);
        add_label(card, "Check the eMMC user volume, then retry.", 80, 44, 340, 20, THEME_MUTED,
                  &lv_font_montserrat_12);
        add_pill_btn(card, 80, 72, 100, 36, "Retry", THEME_ACCENT, 0xFFFFFFu, files_retry_cb);
        return;
    }
    if (st == FILES_ST_PROMPT) {
        lv_obj_t *card = add_card(16, THEME_APPBAR_H + 12, 448, 128);

        add_icon_circle(card, 16, 20, 48, THEME_WARN, LV_SYMBOL_FILE);
        add_label(card, files_prompt_name(), 80, 16, 340, 24, THEME_TEXT, LV_FONT_DEFAULT);
        add_label(card, "No app for this file", 80, 42, 340, 20, THEME_MUTED,
                  &lv_font_montserrat_12);
        add_pill_btn(card, 80, 76, 150, 40, "Open as text", THEME_ACCENT, 0xFFFFFFu,
                     files_open_text_cb);
        add_pill_btn(card, 242, 76, 100, 40, "Cancel", THEME_SURFACE_2, THEME_TEXT,
                     files_cancel_cb);
        return;
    }
    if (st == FILES_ST_EMPTY) {
        lv_obj_t *card = add_card(16, THEME_APPBAR_H + 12, 448, 88);

        add_icon_circle(card, 16, 20, 48, THEME_TILE_FILES, LV_SYMBOL_DIRECTORY);
        add_label(card, "No files yet", 80, 18, 340, 24, THEME_TEXT, LV_FONT_DEFAULT);
        add_label(card, "Copy media onto the eMMC user volume.", 80, 46, 340, 20, THEME_MUTED,
                  &lv_font_montserrat_12);
        return;
    }

    s_list = lv_list_create(s_content);
    lv_obj_set_pos(s_list, 8, THEME_APPBAR_H + 4);
    lv_obj_set_size(s_list, THEME_PANEL_W - 16, content_h() - THEME_APPBAR_H - 8);
    lv_obj_set_style_bg_color(s_list, lv_color_hex(THEME_BG), 0);
    lv_obj_set_style_border_width(s_list, 0, 0);
    lv_obj_set_style_pad_all(s_list, 0, 0);
    lv_obj_set_style_pad_row(s_list, 6, 0);
    {
        unsigned i;
        for (i = 0u; i < files_count(); i++) {
            const files_row_t *r = files_row(i);
            char line[96];
            lv_obj_t *btn;

            row_label(line, sizeof(line), r);
            btn = lv_list_add_button(
                s_list, (r != NULL && r->is_dir != 0u) ? LV_SYMBOL_DIRECTORY : LV_SYMBOL_FILE,
                line);
            lv_obj_set_height(btn, THEME_ROW_H);
            lv_obj_set_style_bg_color(btn, lv_color_hex(THEME_SURFACE), 0);
            lv_obj_set_style_radius(btn, 10, 0);
            lv_obj_set_style_text_color(btn, lv_color_hex(THEME_TEXT), 0);
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
        add_nav_btn(bar, THEME_PANEL_W - 2 * THEME_HIT_MIN_PX, LV_SYMBOL_LEFT, text_page_cb, -1);
        add_nav_btn(bar, THEME_PANEL_W - THEME_HIT_MIN_PX, LV_SYMBOL_RIGHT, text_page_cb, 1);
    }
    box = add_card(12, THEME_APPBAR_H + 8, THEME_PANEL_W - 24, content_h() - THEME_APPBAR_H - 16);
    lv_obj_set_style_pad_all(box, 12, 0);
    lv_obj_add_flag(box, LV_OBJ_FLAG_SCROLLABLE);
    lab = lv_label_create(box);
    lv_label_set_long_mode(lab, LV_LABEL_LONG_WRAP);
    lv_obj_set_width(lab, THEME_PANEL_W - 48);
    lv_label_set_text(lab, text_view_text());
    lv_obj_set_style_text_color(lab, lv_color_hex(THEME_TEXT), 0);
}

static void build_image(void)
{
    lv_obj_t *bar;
    const char *name = image_view_name();

    bar = make_bar((name != NULL && name[0] != '\0') ? name : "Image");
    add_nav_btn(bar, THEME_PANEL_W - 2 * THEME_HIT_MIN_PX, LV_SYMBOL_LEFT, image_nav_cb, -1);
    add_nav_btn(bar, THEME_PANEL_W - THEME_HIT_MIN_PX, LV_SYMBOL_RIGHT, image_nav_cb, 1);
    if (image_view_status() != ERR_OK || image_view_pixels() == NULL || image_view_w() == 0u) {
        lv_obj_t *card = add_card(16, THEME_APPBAR_H + 12, 448, 88);

        add_icon_circle(card, 16, 20, 48, THEME_ERR, LV_SYMBOL_IMAGE);
        add_label(card, "Can't open image", 80, 18, 340, 24, THEME_TEXT, LV_FONT_DEFAULT);
        add_label(card,
                  (image_view_err_str()[0] != '\0') ? image_view_err_str() : "truncated or corrupt",
                  80, 46, 340, 24, THEME_MUTED, &lv_font_montserrat_12);
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

static void fmt_time(char *out, uint32_t ms)
{
    uint32_t sec = ms / 1000u;
    uint32_t m = sec / 60u;
    uint32_t s = sec % 60u;

    out[0] = (char)('0' + ((m / 10u) % 10u));
    out[1] = (char)('0' + (m % 10u));
    out[2] = ':';
    out[3] = (char)('0' + (s / 10u));
    out[4] = (char)('0' + (s % 10u));
    out[5] = '\0';
}

static void fmt_vol(char *out, uint8_t pct)
{
    unsigned n = (unsigned)pct;
    size_t o = 0u;

    if (n >= 100u) {
        out[o++] = '1';
        out[o++] = '0';
        out[o++] = '0';
    } else {
        if (n >= 10u) {
            out[o++] = (char)('0' + ((n / 10u) % 10u));
        }
        out[o++] = (char)('0' + (n % 10u));
    }
    out[o++] = '%';
    out[o] = '\0';
}

static void refresh_player_live(void)
{
    char t0[6];
    char t1[6];
    char vol[12];
    const char *title = player_title();

    if (title == NULL) {
        title = "Music";
    }
    label_set(s_pl_title, title);
    fmt_time(t0, player_elapsed_ms());
    fmt_time(t1, player_duration_ms());
    label_set(s_pl_elapsed, t0);
    label_set(s_pl_dur, t1);
    label_set(s_pl_state, player_playing() ? "Now playing" : "Paused");
    fmt_vol(vol, player_volume());
    label_set(s_pl_vol, vol);
    label_set(s_pl_toggle, player_playing() ? LV_SYMBOL_PAUSE : LV_SYMBOL_PLAY);
    if (s_pl_fill != NULL) {
        uint32_t dur = player_duration_ms();
        uint32_t el = player_elapsed_ms();
        int32_t w = 0;

        if (dur > 0u) {
            w = (int32_t)((el * 320u) / dur);
            if (w > 320) {
                w = 320;
            }
        }
        lv_obj_set_width(s_pl_fill, w);
    }
}

static void build_player(void)
{
    lv_obj_t *card;
    lv_obj_t *ring;
    lv_obj_t *track;
    lv_obj_t *play;
    const char *title = player_title();
    char t0[6];
    char t1[6];
    char vol[12];

    make_bar("Music");
    if (player_status() != ERR_OK && audio_active() == 0u) {
        lv_obj_t *err = add_card(16, THEME_APPBAR_H + 12, 448, 88);

        add_icon_circle(err, 16, 20, 48, THEME_ERR, LV_SYMBOL_AUDIO);
        add_label(err, "Can't open audio", 80, 18, 340, 24, THEME_TEXT, LV_FONT_DEFAULT);
        add_label(err, (player_err_str()[0] != '\0') ? player_err_str() : "unsupported", 80, 46,
                  340, 24, THEME_MUTED, &lv_font_montserrat_12);
        return;
    }

    card = add_card(12, THEME_APPBAR_H + 8, 456, 184);
    ring = lv_obj_create(card);
    lv_obj_set_pos(ring, 12, 16);
    lv_obj_set_size(ring, 80, 80);
    style_round_btn(ring, THEME_SURFACE_2);
    add_icon_circle(ring, 4, 4, 72, THEME_TILE_MUSIC, LV_SYMBOL_AUDIO);

    s_pl_state = add_label(card, player_playing() ? "Now playing" : "Paused", 108, 16, 240, 16,
                           THEME_TILE_MUSIC, &lv_font_montserrat_12);
    s_pl_title = add_label(card, (title != NULL) ? title : "Music", 108, 36, 330, 22, THEME_TEXT,
                           LV_FONT_DEFAULT);
    lv_label_set_long_mode(s_pl_title, LV_LABEL_LONG_DOT);

    track = lv_obj_create(card);
    lv_obj_set_pos(track, 108, 68);
    lv_obj_set_size(track, 320, 6);
    lv_obj_set_style_bg_color(track, lv_color_hex(THEME_SURFACE_2), 0);
    lv_obj_set_style_bg_opa(track, LV_OPA_COVER, 0);
    lv_obj_set_style_radius(track, 3, 0);
    lv_obj_set_style_border_width(track, 0, 0);
    lv_obj_set_style_pad_all(track, 0, 0);
    lv_obj_clear_flag(track, LV_OBJ_FLAG_SCROLLABLE);
    s_pl_fill = lv_obj_create(track);
    lv_obj_set_pos(s_pl_fill, 0, 0);
    lv_obj_set_size(s_pl_fill, 0, 6);
    lv_obj_set_style_bg_color(s_pl_fill, lv_color_hex(THEME_TILE_MUSIC), 0);
    lv_obj_set_style_bg_opa(s_pl_fill, LV_OPA_COVER, 0);
    lv_obj_set_style_radius(s_pl_fill, 3, 0);
    lv_obj_set_style_border_width(s_pl_fill, 0, 0);
    lv_obj_set_style_pad_all(s_pl_fill, 0, 0);

    fmt_time(t0, player_elapsed_ms());
    fmt_time(t1, player_duration_ms());
    s_pl_elapsed = add_label(card, t0, 108, 78, 60, 16, THEME_MUTED, &lv_font_montserrat_12);
    s_pl_dur = add_label(card, t1, 368, 78, 60, 16, THEME_MUTED, &lv_font_montserrat_12);
    lv_obj_set_style_text_align(s_pl_dur, LV_TEXT_ALIGN_RIGHT, 0);

    add_hit_btn(card, 16, 124, LV_SYMBOL_MINUS, player_vol_cb, -10);
    add_hit_btn(card, 84, 124, LV_SYMBOL_PREV, player_nav_cb, -1);
    play = lv_button_create(card);
    lv_obj_set_pos(play, 200, 116);
    lv_obj_set_size(play, 56, 56);
    style_round_btn(play, THEME_TILE_MUSIC);
    lv_obj_add_event_cb(play, player_toggle_cb, LV_EVENT_CLICKED, NULL);
    s_pl_toggle = lv_label_create(play);
    lv_label_set_text(s_pl_toggle, player_playing() ? LV_SYMBOL_PAUSE : LV_SYMBOL_PLAY);
    lv_obj_set_style_text_color(s_pl_toggle, lv_color_hex(0xFFFFFFu), 0);
    lv_obj_center(s_pl_toggle);
    add_hit_btn(card, 332, 124, LV_SYMBOL_NEXT, player_nav_cb, 1);
    add_hit_btn(card, 400, 124, LV_SYMBOL_PLUS, player_vol_cb, 10);
    fmt_vol(vol, player_volume());
    s_pl_vol = add_label(card, vol, 12, 100, 80, 16, THEME_MUTED, &lv_font_montserrat_12);
    lv_obj_set_style_text_align(s_pl_vol, LV_TEXT_ALIGN_CENTER, 0);
}

static const char *net_path_pretty(void)
{
    const char *p = network_path_str();

    if (p != NULL && strcmp(p, "eth") == 0) {
        return "Ethernet";
    }
    if (p != NULL && strcmp(p, "wifi") == 0) {
        return "Wi-Fi";
    }
    return "None";
}

static const char *net_link_pretty(void)
{
    const char *p = network_link_str();

    if (p != NULL && strcmp(p, "up full") == 0) {
        return "Up · full";
    }
    if (p != NULL && strcmp(p, "up half") == 0) {
        return "Up · half";
    }
    return "Down";
}

static void add_stat_card(int32_t x, int32_t y, int32_t w, const char *kicker, const char *value,
                          uint32_t accent)
{
    lv_obj_t *card = add_card(x, y, w, 48);
    lv_obj_t *dot = lv_obj_create(card);

    lv_obj_set_pos(dot, 12, 19);
    lv_obj_set_size(dot, 10, 10);
    style_round_btn(dot, accent);
    add_label(card, kicker, 32, 4, w - 44, 16, THEME_MUTED, &lv_font_montserrat_12);
    add_label(card, value, 32, 22, w - 44, 22, THEME_TEXT, LV_FONT_DEFAULT);
}

static const char *home_symbol(home_kind_t kind)
{
    if (kind == HOME_LIGHT) {
        return LV_SYMBOL_CHARGE;
    }
    if (kind == HOME_SWITCH) {
        return LV_SYMBOL_POWER;
    }
    if (kind == HOME_BINARY_SENSOR) {
        return LV_SYMBOL_EYE_OPEN;
    }
    return LV_SYMBOL_REFRESH;
}

static void home_pair_cb(lv_event_t *e)
{
    if (lv_event_get_code(e) != LV_EVENT_CLICKED) {
        return;
    }
    home_app_pair();
    log_nav("home", "pair");
}

static void home_net_cb(lv_event_t *e)
{
    if (lv_event_get_code(e) != LV_EVENT_CLICKED) {
        return;
    }
    home_app_open_network();
    log_nav("home", "network");
}

static void home_row_cb(lv_event_t *e)
{
    unsigned idx;

    if (lv_event_get_code(e) != LV_EVENT_CLICKED) {
        return;
    }
    idx = (unsigned)(uintptr_t)lv_event_get_user_data(e);
    home_app_open_device(idx);
    log_nav("home", "device");
}

static void home_toggle_cb(lv_event_t *e)
{
    unsigned idx;

    if (lv_event_get_code(e) != LV_EVENT_CLICKED) {
        return;
    }
    idx = (unsigned)(uintptr_t)lv_event_get_user_data(e);
    home_app_toggle(idx);
}

static void home_ieee_text(char *out, size_t n, const home_device_t *d)
{
    size_t i;
    size_t o = 0u;

    if (out == NULL || n == 0u || d == NULL) {
        return;
    }
    out[0] = '\0';
    for (i = 0u; i < 8u && o + 3u < n; i++) {
        static const char hex[] = "0123456789abcdef";
        if (i > 0u) {
            out[o++] = ':';
        }
        out[o++] = hex[(d->ieee[i] >> 4) & 0x0Fu];
        out[o++] = hex[d->ieee[i] & 0x0Fu];
    }
    out[o] = '\0';
}

static void home_u16_text(char *out, size_t n, const char *prefix, uint16_t v)
{
    char tmp[6];
    int i = 6;
    size_t o = 0u;

    if (out == NULL || n == 0u) {
        return;
    }
    if (prefix != NULL) {
        while (*prefix != '\0' && o + 1u < n) {
            out[o++] = *prefix++;
        }
    }
    tmp[5] = '\0';
    if (v == 0u) {
        tmp[--i] = '0';
    }
    while (v > 0u && i > 0) {
        tmp[--i] = (char)('0' + (v % 10u));
        v = (uint16_t)(v / 10u);
    }
    while (tmp[i] != '\0' && o + 1u < n) {
        out[o++] = tmp[i++];
    }
    out[o] = '\0';
}

static void build_home_list(lv_obj_t *bar)
{
    home_device_t d[HOME_DEV_MAX];
    size_t n;
    size_t i;
    int32_t y;
    home_net_t net;
    uint32_t banner_bg = THEME_SURFACE;

    add_pill_btn(bar, THEME_PANEL_W - 84, 4, 72, 32, "Pair", THEME_TILE_HOME, 0xFFFFFFu,
                 home_pair_cb);
    lv_obj_add_flag(s_content, LV_OBJ_FLAG_SCROLLABLE);
    home_net(&net);
    if (net.radio_ok == 0u) {
        banner_bg = 0xFFF3D6u;
    } else if (net.permit_left > 0u) {
        banner_bg = 0xE8F8ECu;
    }
    {
        lv_obj_t *banner = add_card(12, THEME_APPBAR_H + 6, 456, 36);
        lv_obj_set_style_bg_color(banner, lv_color_hex(banner_bg), 0);
        lv_obj_add_flag(banner, LV_OBJ_FLAG_CLICKABLE);
        lv_obj_add_event_cb(banner, home_net_cb, LV_EVENT_CLICKED, NULL);
        s_home_banner = add_label(banner, home_app_banner(), 12, 8, 432, 20, THEME_TEXT,
                                  &lv_font_montserrat_12);
    }
    n = home_devices(NULL, d, HOME_DEV_MAX);
    if (n == 0u) {
        add_message_card(THEME_APPBAR_H + 50, "No devices", THEME_TEXT,
                         "Tap Pair to open the network.");
        return;
    }
    y = THEME_APPBAR_H + 48;
    for (i = 0u; i < n; i++) {
        lv_obj_t *row = add_card(12, y, 456, THEME_ROW_H);
        uint8_t can_tog = (d[i].kind == HOME_LIGHT || d[i].kind == HOME_SWITCH) ? 1u : 0u;
        uint32_t ic = (d[i].on != 0u) ? THEME_TILE_HOME : THEME_MUTED;

        lv_obj_add_flag(row, LV_OBJ_FLAG_CLICKABLE);
        lv_obj_add_event_cb(row, home_row_cb, LV_EVENT_CLICKED, (void *)(uintptr_t)i);
        add_icon_circle(row, 8, 6, 32, ic, home_symbol(d[i].kind));
        add_label(row, d[i].name, 48, 4, 220, 20, THEME_TEXT, &lv_font_montserrat_12);
        add_label(row, d[i].room_id, 48, 22, 160, 16, THEME_MUTED, &lv_font_montserrat_12);
        add_label(row, home_app_state_text(&d[i]), 270, 12, 80, 20, THEME_TEXT,
                  &lv_font_montserrat_12);
        if (can_tog != 0u) {
            lv_obj_t *tog = add_pill_btn(row, 360, 2, 88, 40, (d[i].on != 0u) ? "On" : "Off",
                                         (d[i].on != 0u) ? THEME_OK : THEME_SURFACE_2,
                                         (d[i].on != 0u) ? 0xFFFFFFu : THEME_TEXT, NULL);
            lv_obj_add_event_cb(tog, home_toggle_cb, LV_EVENT_CLICKED, (void *)(uintptr_t)i);
        }
        y += THEME_ROW_H + 6;
    }
}

static void build_home_device(lv_obj_t *bar)
{
    home_device_t d;
    char ieee[28];
    char nwk[16];
    char lqi[16];
    lv_obj_t *card;
    unsigned idx = home_app_sel();

    add_pill_btn(bar, THEME_PANEL_W - 84, 4, 72, 32, "Pair", THEME_TILE_HOME, 0xFFFFFFu,
                 home_pair_cb);
    if (home_device_at((size_t)idx, &d) != ERR_OK) {
        add_message_card(THEME_APPBAR_H + 12, "Missing device", THEME_ERR, "Go back to the list.");
        return;
    }
    home_ieee_text(ieee, sizeof(ieee), &d);
    home_u16_text(nwk, sizeof(nwk), "NWK ", d.nwk);
    home_u16_text(lqi, sizeof(lqi), "LQI ", d.lqi);
    card = add_card(12, THEME_APPBAR_H + 8, 456, 168);
    add_icon_circle(card, 16, 16, 40, THEME_TILE_HOME, home_symbol(d.kind));
    add_label(card, d.name, 68, 12, 360, 22, THEME_TEXT, LV_FONT_DEFAULT);
    add_label(card, d.room_id, 68, 36, 200, 18, THEME_MUTED, &lv_font_montserrat_12);
    add_label(card, home_app_kind_text(d.kind), 280, 36, 160, 18, THEME_MUTED,
              &lv_font_montserrat_12);
    add_label(card, ieee, 16, 68, 420, 18, THEME_TEXT, &lv_font_montserrat_12);
    add_label(card, nwk, 16, 90, 200, 18, THEME_MUTED, &lv_font_montserrat_12);
    add_label(card, lqi, 220, 90, 200, 18, THEME_MUTED, &lv_font_montserrat_12);
    if (d.kind == HOME_LIGHT || d.kind == HOME_SWITCH) {
        lv_obj_t *tog = add_pill_btn(card, 16, 118, 120, 40, (d.on != 0u) ? "On" : "Off",
                                     (d.on != 0u) ? THEME_OK : THEME_SURFACE_2,
                                     (d.on != 0u) ? 0xFFFFFFu : THEME_TEXT, NULL);
        lv_obj_add_event_cb(tog, home_toggle_cb, LV_EVENT_CLICKED, (void *)(uintptr_t)idx);
    } else {
        add_label(card, home_app_state_text(&d), 16, 124, 200, 20, THEME_TEXT, LV_FONT_DEFAULT);
    }
}

static void build_home_network(lv_obj_t *bar)
{
    home_net_t n;
    char ch[16];
    char pan[16];
    char cnt[16];
    lv_obj_t *card;

    add_pill_btn(bar, THEME_PANEL_W - 84, 4, 72, 32, "Pair", THEME_TILE_HOME, 0xFFFFFFu,
                 home_pair_cb);
    home_net(&n);
    home_u16_text(ch, sizeof(ch), "ch ", n.channel);
    home_u16_text(pan, sizeof(pan), "PAN ", n.pan);
    home_u16_text(cnt, sizeof(cnt), "", (uint16_t)home_device_count());
    card = add_card(12, THEME_APPBAR_H + 8, 456, 168);
    add_label(card, (n.radio_ok != 0u) ? "Radio up" : "Radio not ready", 16, 12, 420, 22,
              (n.radio_ok != 0u) ? THEME_OK : THEME_WARN, LV_FONT_DEFAULT);
    add_label(card, n.znp_ver, 16, 38, 200, 18, THEME_MUTED, &lv_font_montserrat_12);
    add_label(card, ch, 16, 60, 140, 18, THEME_TEXT, &lv_font_montserrat_12);
    add_label(card, pan, 160, 60, 160, 18, THEME_TEXT, &lv_font_montserrat_12);
    add_label(card, (n.formed != 0u) ? "Formed" : "Not formed", 16, 82, 200, 18, THEME_TEXT,
              &lv_font_montserrat_12);
    add_label(card, cnt, 220, 82, 80, 18, THEME_TEXT, &lv_font_montserrat_12);
    add_label(card, "devices", 300, 82, 120, 18, THEME_MUTED, &lv_font_montserrat_12);
    s_home_banner =
        add_label(card, home_app_banner(), 16, 110, 420, 20, THEME_TEXT, &lv_font_montserrat_12);
    add_label(card, (n.persist_ok != 0u) ? "Saved on eMMC" : "Not saved", 16, 134, 200, 18,
              THEME_MUTED, &lv_font_montserrat_12);
}

static void build_home(void)
{
    lv_obj_t *bar = make_bar(home_app_title());

    s_home_banner = NULL;
    if (home_app_page() == HOME_PAGE_DEVICE) {
        build_home_device(bar);
        return;
    }
    if (home_app_page() == HOME_PAGE_NETWORK) {
        build_home_network(bar);
        return;
    }
    build_home_list(bar);
}

static void refresh_home_live(void)
{
    if (shell_top_id() == NULL || strcmp(shell_top_id(), APP_ID_HOME) != 0) {
        return;
    }
    label_set(s_home_banner, home_app_banner());
}

static void game_pause_cb(lv_event_t *e)
{
    if (lv_event_get_code(e) != LV_EVENT_CLICKED) {
        return;
    }
    if (game_phase() == GAME_PHASE_PLAY) {
        game_pause();
    } else if (game_phase() == GAME_PHASE_PAUSE) {
        game_resume();
    }
}

static void game_resume_cb(lv_event_t *e)
{
    if (lv_event_get_code(e) != LV_EVENT_CLICKED) {
        return;
    }
    game_resume();
}

static void game_new_cb(lv_event_t *e)
{
    if (lv_event_get_code(e) != LV_EVENT_CLICKED) {
        return;
    }
    game_new();
}

static void game_quit_cb(lv_event_t *e)
{
    if (lv_event_get_code(e) != LV_EVENT_CLICKED) {
        return;
    }
    game_to_library();
    log_nav("game", "library");
}

static void game_row_cb(lv_event_t *e)
{
    unsigned idx;

    if (lv_event_get_code(e) != LV_EVENT_CLICKED) {
        return;
    }
    idx = (unsigned)(uintptr_t)lv_event_get_user_data(e);
    game_pick(idx);
    log_nav("game", game_title());
}

static void game_ptr_cb(lv_event_t *e)
{
    lv_event_code_t code = lv_event_get_code(e);
    lv_obj_t *obj = (lv_obj_t *)lv_event_get_target(e);
    lv_indev_t *indev;
    lv_point_t p;
    lv_area_t a;
    input_kind_t kind;
    int16_t x;
    int16_t y;

    if (code == LV_EVENT_PRESSED) {
        kind = INPUT_PTR_DOWN;
    } else if (code == LV_EVENT_PRESSING) {
        kind = INPUT_PTR_MOVE;
    } else if (code == LV_EVENT_RELEASED) {
        kind = INPUT_PTR_UP;
    } else {
        return;
    }
    indev = lv_event_get_indev(e);
    if (indev == NULL) {
        indev = lv_indev_active();
    }
    if (indev == NULL || obj == NULL) {
        return;
    }
    lv_indev_get_point(indev, &p);
    lv_obj_get_coords(obj, &a);
    x = (int16_t)(p.x - a.x1);
    y = (int16_t)(p.y - a.y1);
    game_pointer(kind, x, y);
}

static void bind_game_image(void)
{
    memset(&s_img_dsc, 0, sizeof(s_img_dsc));
    s_img_dsc.header.magic = LV_IMAGE_HEADER_MAGIC;
    s_img_dsc.header.cf = LV_COLOR_FORMAT_RGB565;
    s_img_dsc.header.w = game_field_w();
    s_img_dsc.header.h = game_field_h();
    s_img_dsc.header.stride = (uint32_t)game_field_stride() * 2u;
    s_img_dsc.data_size = (uint32_t)game_field_stride() * (uint32_t)game_field_h() * 2u;
    s_img_dsc.data = (const uint8_t *)game_pixels();
}

static void refresh_game_live(void)
{
    uint16_t want_h;

    if (shell_top_id() == NULL || strcmp(shell_top_id(), APP_ID_GAME) != 0) {
        return;
    }
    if (game_in_library() != 0u) {
        return;
    }
    want_h = (uint16_t)(content_h() - THEME_APPBAR_H);
    if (want_h < 80u) {
        want_h = 80u;
    }
    if (game_field_w() != THEME_PANEL_W || game_field_h() != want_h) {
        game_resize(THEME_PANEL_W, want_h);
        bind_game_image();
        if (s_game_img != NULL) {
            lv_obj_set_size(s_game_img, THEME_PANEL_W, (int32_t)want_h);
            lv_image_set_src(s_game_img, &s_img_dsc);
        }
    }
    label_set(s_game_score, game_score_str());
    label_set(s_game_high, game_high_str());
    label_set(s_game_lives, game_lives_str());
    if (s_game_img != NULL) {
        lv_obj_invalidate(s_game_img);
    }
}

static void build_game_overlay(game_phase_t phase)
{
    lv_obj_t *card;
    int32_t y = THEME_APPBAR_H + 36;

    if (phase == GAME_PHASE_PAUSE) {
        card = add_card(80, y, 320, 96);
        add_label(card, "Paused", 16, 10, 288, 22, THEME_TEXT, LV_FONT_DEFAULT);
        add_pill_btn(card, 16, 44, 136, 40, "Resume", THEME_TILE_GAME, 0xFFFFFFu, game_resume_cb);
        add_pill_btn(card, 168, 44, 136, 40, "Games", THEME_SURFACE_2, THEME_TEXT, game_quit_cb);
        return;
    }
    card = add_card(80, y, 320, 110);
    add_label(card, "Game over", 16, 10, 288, 22, THEME_TEXT, LV_FONT_DEFAULT);
    add_label(card, game_score_str(), 16, 34, 288, 18, THEME_MUTED, &lv_font_montserrat_12);
    add_pill_btn(card, 16, 58, 136, 40, "New game", THEME_TILE_GAME, 0xFFFFFFu, game_new_cb);
    add_pill_btn(card, 168, 58, 136, 40, "Games", THEME_SURFACE_2, THEME_TEXT, game_quit_cb);
}

static void build_game_library(void)
{
    unsigned i;

    make_bar("Games");
    s_list = lv_list_create(s_content);
    lv_obj_set_pos(s_list, 8, THEME_APPBAR_H + 4);
    lv_obj_set_size(s_list, THEME_PANEL_W - 16, content_h() - THEME_APPBAR_H - 8);
    lv_obj_set_style_bg_color(s_list, lv_color_hex(THEME_BG), 0);
    lv_obj_set_style_border_width(s_list, 0, 0);
    lv_obj_set_style_pad_all(s_list, 0, 0);
    lv_obj_set_style_pad_row(s_list, 6, 0);
    for (i = 0u; i < game_title_count(); i++) {
        const game_title_t *t = game_title_at(i);
        lv_obj_t *btn;
        const char *lab = (t != NULL) ? t->name : "Game";
        const char *sym =
            (t != NULL && strcmp(t->core, "chip8") == 0) ? LV_SYMBOL_VIDEO : LV_SYMBOL_PLAY;

        btn = lv_list_add_button(s_list, sym, lab);
        lv_obj_set_height(btn, THEME_ROW_H);
        lv_obj_set_style_bg_color(btn, lv_color_hex(THEME_SURFACE), 0);
        lv_obj_set_style_radius(btn, 10, 0);
        lv_obj_set_style_text_color(btn, lv_color_hex(THEME_TEXT), 0);
        lv_obj_add_event_cb(btn, game_row_cb, LV_EVENT_CLICKED, (void *)(uintptr_t)i);
    }
}

static void build_game(void)
{
    lv_obj_t *bar;
    uint16_t field_h;
    game_phase_t phase;
    const char *title;

    if (game_in_library() != 0u) {
        build_game_library();
        return;
    }

    field_h = (uint16_t)(content_h() - THEME_APPBAR_H);
    if (field_h < 80u) {
        field_h = 80u;
    }
    game_resize(THEME_PANEL_W, field_h);
    phase = game_phase();
    title = game_title();
    bar = make_bar((title != NULL && title[0] != '\0') ? title : "Game");
    add_label(bar, "SCORE", 96, 4, 56, 14, THEME_MUTED, &lv_font_montserrat_12);
    s_game_score =
        add_label(bar, game_score_str(), 96, 18, 56, 18, THEME_TEXT, &lv_font_montserrat_12);
    add_label(bar, "LIVES", 168, 4, 48, 14, THEME_MUTED, &lv_font_montserrat_12);
    s_game_lives =
        add_label(bar, game_lives_str(), 168, 18, 48, 18, THEME_TEXT, &lv_font_montserrat_12);
    add_label(bar, "HIGH", 228, 4, 56, 14, THEME_MUTED, &lv_font_montserrat_12);
    s_game_high =
        add_label(bar, game_high_str(), 228, 18, 80, 18, THEME_TILE_GAME, &lv_font_montserrat_12);
    add_nav_btn(bar, THEME_PANEL_W - THEME_HIT_MIN_PX,
                (phase == GAME_PHASE_PLAY) ? LV_SYMBOL_PAUSE : LV_SYMBOL_PLAY, game_pause_cb, 0);

    bind_game_image();
    s_game_img = lv_image_create(s_content);
    lv_obj_set_pos(s_game_img, 0, THEME_APPBAR_H);
    lv_obj_set_size(s_game_img, THEME_PANEL_W, (int32_t)field_h);
    lv_image_set_src(s_game_img, &s_img_dsc);
    lv_obj_add_flag(s_game_img, LV_OBJ_FLAG_CLICKABLE);
    lv_obj_clear_flag(s_game_img, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_add_event_cb(s_game_img, game_ptr_cb, LV_EVENT_PRESSED, NULL);
    lv_obj_add_event_cb(s_game_img, game_ptr_cb, LV_EVENT_PRESSING, NULL);
    lv_obj_add_event_cb(s_game_img, game_ptr_cb, LV_EVENT_RELEASED, NULL);

    if (phase != GAME_PHASE_PLAY) {
        build_game_overlay(phase);
    }
}

static void build_settings(void)
{
    lv_obj_t *card = add_card(16, THEME_APPBAR_H + 6, 448, 56);
    char vol[12];
    const char *link;

    make_bar("Settings");
    network_refresh();
    add_icon_circle(card, 12, 8, 40, THEME_TILE_SET, LV_SYMBOL_SETTINGS);
    add_label(card, "STM32H745 Disco", 64, 8, 200, 20, THEME_TEXT, LV_FONT_DEFAULT);
    add_label(card, "CN10 headphone", 64, 30, 160, 16, THEME_MUTED, &lv_font_montserrat_12);
    add_hit_btn(card, 280, 8, LV_SYMBOL_MINUS, player_vol_cb, -10);
    fmt_vol(vol, player_volume());
    s_pl_vol = add_label(card, vol, 328, 16, 48, 20, THEME_TEXT, LV_FONT_DEFAULT);
    lv_obj_set_style_text_align(s_pl_vol, LV_TEXT_ALIGN_CENTER, 0);
    add_hit_btn(card, 384, 8, LV_SYMBOL_PLUS, player_vol_cb, 10);

    link = net_link_pretty();
    add_stat_card(16, THEME_APPBAR_H + 68, 220, "Link", link,
                  (strcmp(link, "Down") == 0) ? THEME_ERR : THEME_TILE_NET);
    add_stat_card(244, THEME_APPBAR_H + 68, 220, "IPv4", network_ip_str(), THEME_ACCENT);
    add_stat_card(16, THEME_APPBAR_H + 122, 220, "Path", net_path_pretty(), THEME_TILE_NET);
    add_stat_card(244, THEME_APPBAR_H + 122, 220, "Time", time_ntp_str(),
                  (time_ntp_state() == TIME_NTP_OK) ? THEME_OK : THEME_WARN);
}

static void calendar_nav_cb(lv_event_t *e)
{
    int dir;

    if (lv_event_get_code(e) != LV_EVENT_CLICKED) {
        return;
    }
    dir = (int)(intptr_t)lv_event_get_user_data(e);
    if (dir < 0) {
        calendar_prev_month();
    } else if (dir > 0) {
        calendar_next_month();
    } else {
        calendar_go_today();
    }
}

static void refresh_calendar_live(void)
{
    label_set(s_cal_clock, calendar_clock());
    label_set(s_cal_date, calendar_date_line());
    label_set(s_cal_ntp, calendar_ntp_line());
}

static void build_calendar(void)
{
    lv_obj_t *bar;
    unsigned i;
    int32_t grid_y;
    int32_t cell_w;
    int32_t cell_h;
    int32_t grid_h;
    static const char *const wd[7] = {"S", "M", "T", "W", "T", "F", "S"};

    bar = make_bar(calendar_title());
    add_nav_btn(bar, THEME_PANEL_W - 2 * THEME_HIT_MIN_PX, LV_SYMBOL_LEFT, calendar_nav_cb, -1);
    add_nav_btn(bar, THEME_PANEL_W - THEME_HIT_MIN_PX, LV_SYMBOL_RIGHT, calendar_nav_cb, 1);

    s_cal_clock = add_label(s_content, calendar_clock(), 16, THEME_APPBAR_H + 4, 160, 22,
                            THEME_TEXT, LV_FONT_DEFAULT);
    s_cal_date = add_label(s_content, calendar_date_line(), 180, THEME_APPBAR_H + 6, 180, 18,
                           THEME_MUTED, &lv_font_montserrat_12);
    s_cal_ntp = add_label(s_content, calendar_ntp_line(), 360, THEME_APPBAR_H + 6, 110, 18,
                          THEME_TILE_CAL, &lv_font_montserrat_12);

    grid_y = THEME_APPBAR_H + 28;
    for (i = 0u; i < 7u; i++) {
        add_label(s_content, wd[i], (int32_t)(8 + i * 67), grid_y, 64, 14, THEME_MUTED,
                  &lv_font_montserrat_12);
    }
    grid_y += 16;
    grid_h = content_h() - grid_y - 4;
    if (grid_h < 96) {
        grid_h = 96;
    }
    cell_w = 66;
    cell_h = grid_h / 6;
    if (cell_h < 16) {
        cell_h = 16;
    }
    for (i = 0u; i < CALENDAR_CELLS; i++) {
        uint8_t day = calendar_cell_day(i);
        uint8_t today = calendar_cell_today(i);
        int32_t x = (int32_t)(8 + (i % 7u) * 67);
        int32_t y = grid_y + (int32_t)((i / 7u) * (unsigned)cell_h);
        char num[3];
        lv_obj_t *cell;
        uint32_t bg = THEME_SURFACE;
        uint32_t fg = THEME_TEXT;

        if (day == 0u) {
            continue;
        }
        if (today != 0u) {
            bg = THEME_TILE_CAL;
            fg = 0xFFFFFFu;
        }
        cell = lv_obj_create(s_content);
        lv_obj_set_pos(cell, x, y);
        lv_obj_set_size(cell, cell_w, cell_h - 2);
        lv_obj_set_style_bg_color(cell, lv_color_hex(bg), 0);
        lv_obj_set_style_bg_opa(cell, LV_OPA_COVER, 0);
        lv_obj_set_style_radius(cell, 8, 0);
        lv_obj_set_style_border_width(cell, 0, 0);
        lv_obj_set_style_pad_all(cell, 0, 0);
        lv_obj_clear_flag(cell, LV_OBJ_FLAG_SCROLLABLE);
        if (day >= 10u) {
            num[0] = (char)('0' + ((day / 10u) % 10u));
            num[1] = (char)('0' + (day % 10u));
            num[2] = '\0';
        } else {
            num[0] = (char)('0' + day);
            num[1] = '\0';
        }
        add_label(cell, num, 0, (cell_h - 18) / 2, cell_w, 16, fg, &lv_font_montserrat_12);
        lv_obj_set_style_text_align(lv_obj_get_child(cell, 0), LV_TEXT_ALIGN_CENTER, 0);
    }
}

static void build_app(const char *id, const char *title)
{
    s_list = NULL;
    if (id != NULL && strcmp(id, APP_ID_TEXT) == 0) {
        build_text();
        return;
    }
    if (id != NULL && strcmp(id, APP_ID_IMAGE) == 0) {
        build_image();
        return;
    }
    if (id != NULL && strcmp(id, APP_ID_PLAYER) == 0) {
        build_player();
        return;
    }
    if (id != NULL && strcmp(id, APP_ID_NETWORK) == 0) {
        build_settings();
        return;
    }
    if (id != NULL && strcmp(id, APP_ID_CALENDAR) == 0) {
        build_calendar();
        return;
    }
    if (id != NULL && strcmp(id, APP_ID_FILES) == 0) {
        build_files();
        return;
    }
    if (id != NULL && strcmp(id, APP_ID_HOME) == 0) {
        build_home();
        return;
    }
    if (id != NULL && strcmp(id, APP_ID_GAME) == 0) {
        build_game();
        return;
    }
    if (id != NULL && strcmp(id, APP_ID_SETTINGS) == 0) {
        build_settings();
        return;
    }
    make_bar((title != NULL) ? title : "");
    add_message_card(THEME_APPBAR_H + 12, "Nothing here yet", THEME_TEXT,
                     "This screen is reserved.");
}

static void rebuild_content(void)
{
    const char *id;

    s_list = NULL;
    lv_obj_clear_flag(s_content, LV_OBJ_FLAG_SCROLLABLE);
    s_pl_title = NULL;
    s_pl_elapsed = NULL;
    s_pl_dur = NULL;
    s_pl_state = NULL;
    s_pl_vol = NULL;
    s_pl_toggle = NULL;
    s_pl_fill = NULL;
    s_cal_clock = NULL;
    s_cal_date = NULL;
    s_cal_ntp = NULL;
    s_home_banner = NULL;
    s_game_img = NULL;
    s_game_score = NULL;
    s_game_high = NULL;
    s_game_lives = NULL;
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

static uint32_t net_color(uint8_t level)
{
    if (level == 3u) {
        return THEME_OK;
    }
    if (level == 2u) {
        return THEME_WARN;
    }
    return THEME_ERR;
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
    if (st->net == 0u) {
        lv_obj_add_flag(s_eth, LV_OBJ_FLAG_HIDDEN);
    } else {
        lv_obj_remove_flag(s_eth, LV_OBJ_FLAG_HIDDEN);
        lv_obj_set_style_text_color(s_eth, lv_color_hex(net_color(st->net)), 0);
    }
    lv_obj_set_style_text_color(s_zb, lv_color_hex(net_color((st->zb != 0u) ? st->zb : 1u)), 0);
    s_last_min = st->min;
    s_last_m4 = st->m4;
    s_last_stor = st->storage_ok;
    s_last_net = st->net;
    s_last_zb = st->zb;
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
    lv_obj_set_style_border_width(s_status, 1, 0);
    lv_obj_set_style_border_side(s_status, LV_BORDER_SIDE_BOTTOM, 0);
    lv_obj_set_style_border_color(s_status, lv_color_hex(THEME_HAIRLINE), 0);

    s_time = lv_label_create(s_status);
    lv_obj_set_pos(s_time, 12, 8);
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

    s_eth = lv_label_create(s_status);
    lv_label_set_text(s_eth, "ETH");
    lv_obj_set_pos(s_eth, 180, 8);
    lv_obj_set_style_text_font(s_eth, &lv_font_montserrat_12, 0);

    s_zb = lv_label_create(s_status);
    lv_label_set_text(s_zb, "ZB");
    lv_obj_set_pos(s_zb, 220, 8);
    lv_obj_set_style_text_font(s_zb, &lv_font_montserrat_12, 0);

    s_nowplay = lv_obj_create(scr);
    lv_obj_set_pos(s_nowplay, 0, (int32_t)(THEME_PANEL_H - THEME_NOWPLAYING_H));
    lv_obj_set_size(s_nowplay, THEME_PANEL_W, THEME_NOWPLAYING_H);
    style_bar(s_nowplay);
    lv_obj_set_style_bg_color(s_nowplay, lv_color_hex(THEME_SURFACE), 0);
    lv_obj_set_style_border_width(s_nowplay, 1, 0);
    lv_obj_set_style_border_side(s_nowplay, LV_BORDER_SIDE_TOP, 0);
    lv_obj_set_style_border_color(s_nowplay, lv_color_hex(THEME_HAIRLINE), 0);
    add_icon_circle(s_nowplay, 8, 4, 28, THEME_TILE_MUSIC, LV_SYMBOL_AUDIO);
    s_np_title = lv_label_create(s_nowplay);
    lv_obj_set_pos(s_np_title, 44, 8);
    lv_obj_set_width(s_np_title, 360);
    lv_label_set_long_mode(s_np_title, LV_LABEL_LONG_DOT);
    lv_obj_set_style_text_color(s_np_title, lv_color_hex(THEME_TILE_MUSIC), 0);
    lv_obj_add_flag(s_np_title, LV_OBJ_FLAG_CLICKABLE);
    lv_obj_add_event_cb(s_np_title, mini_open_cb, LV_EVENT_CLICKED, NULL);
    s_np_btn = lv_button_create(s_nowplay);
    lv_obj_set_pos(s_np_btn, THEME_PANEL_W - 40, 0);
    lv_obj_set_size(s_np_btn, 36, 36);
    style_round_btn(s_np_btn, THEME_TILE_MUSIC);
    lv_obj_add_event_cb(s_np_btn, player_toggle_cb, LV_EVENT_CLICKED, NULL);
    {
        lv_obj_t *lab = lv_label_create(s_np_btn);
        lv_label_set_text(lab, LV_SYMBOL_PAUSE);
        lv_obj_set_style_text_color(lab, lv_color_hex(0xFFFFFFu), 0);
        lv_obj_center(lab);
    }
    lv_obj_add_flag(s_nowplay, LV_OBJ_FLAG_HIDDEN);

    s_content = lv_obj_create(scr);
    lv_obj_set_pos(s_content, 0, THEME_STATUS_H);
    lv_obj_set_size(s_content, THEME_PANEL_W, content_h());
    lv_obj_set_style_bg_color(s_content, lv_color_hex(THEME_BG), 0);
    lv_obj_set_style_bg_opa(s_content, LV_OPA_COVER, 0);
    lv_obj_set_style_border_width(s_content, 0, 0);
    lv_obj_set_style_pad_all(s_content, 0, 0);
    lv_obj_set_style_radius(s_content, 0, 0);
    lv_obj_clear_flag(s_content, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_remove_flag(s_content, LV_OBJ_FLAG_OVERFLOW_VISIBLE);

    s_gen = 0xFFFFFFFFu;
    s_files_gen = 0xFFFFFFFFu;
    s_text_gen = 0xFFFFFFFFu;
    s_img_gen = 0xFFFFFFFFu;
    s_game_gen = 0xFFFFFFFFu;
    s_home_gen = 0xFFFFFFFFu;
    refresh_status();
    rebuild_content();
    s_gen = shell_nav_gen();
    s_files_gen = files_view_gen();
    s_text_gen = text_view_gen();
    s_img_gen = image_view_gen();
    s_net_gen = network_gen();
    s_cal_gen = calendar_gen();
    s_game_gen = game_gen();
    s_home_gen = home_app_gen();
    return ERR_OK;
}

void ui_backend_handler(void)
{
    uint32_t gen = shell_nav_gen();
    uint32_t fgen = files_view_gen();
    uint32_t tgen = text_view_gen();
    uint32_t igen = image_view_gen();
    uint32_t ngen = network_gen();
    uint32_t cgen = calendar_gen();
    uint32_t ggen = game_gen();
    uint32_t hgen = home_app_gen();
    uint8_t audio = audio_active();
    const char *id = shell_top_id();
    uint8_t show_mini = (audio != 0u && (id == NULL || strcmp(id, APP_ID_PLAYER) != 0)) ? 1u : 0u;
    int32_t h = content_h();

    if (lv_obj_get_height(s_content) != h) {
        lv_obj_set_size(s_content, THEME_PANEL_W, h);
    }
    if (show_mini != 0u) {
        if (lv_obj_has_flag(s_nowplay, LV_OBJ_FLAG_HIDDEN)) {
            lv_obj_remove_flag(s_nowplay, LV_OBJ_FLAG_HIDDEN);
        }
        label_set(s_np_title, audio_title());
        {
            lv_obj_t *lab = lv_obj_get_child(s_np_btn, 0);
            if (lab != NULL) {
                label_set(lab, (audio_state() == AUDIO_ST_PLAY) ? LV_SYMBOL_PAUSE : LV_SYMBOL_PLAY);
            }
        }
    } else if (lv_obj_has_flag(s_nowplay, LV_OBJ_FLAG_HIDDEN) == 0) {
        lv_obj_add_flag(s_nowplay, LV_OBJ_FLAG_HIDDEN);
    }

    /* Do not rebuild on player_gen: elapsed time used to recreate the whole
     * tree every second, which flickered and ate taps. */
    if (gen != s_gen || fgen != s_files_gen || tgen != s_text_gen || igen != s_img_gen ||
        ngen != s_net_gen || cgen != s_cal_gen || ggen != s_game_gen || hgen != s_home_gen ||
        audio != s_last_audio) {
        s_gen = gen;
        s_files_gen = fgen;
        s_text_gen = tgen;
        s_img_gen = igen;
        s_net_gen = ngen;
        s_cal_gen = cgen;
        s_game_gen = ggen;
        s_home_gen = hgen;
        s_last_audio = audio;
        rebuild_content();
    }
    refresh_player_live();
    refresh_calendar_live();
    refresh_game_live();
    refresh_home_live();
    if (shell_status()->min != s_last_min || shell_status()->m4 != s_last_m4 ||
        shell_status()->storage_ok != s_last_stor || shell_status()->net != s_last_net ||
        shell_status()->zb != s_last_zb) {
        refresh_status();
    }
    lv_timer_handler();
}

void ui_backend_invalidate(void)
{
    lv_obj_t *scr = lv_screen_active();

    if (scr != NULL) {
        lv_obj_invalidate(scr);
    }
}

uint32_t ui_backend_frames(void)
{
    return lv_port_frames();
}
