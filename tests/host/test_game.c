#include "unity.h"

#include "app/game.h"
#include "game/game_sim.h"
#include "game/gfx.h"
#include "svc/vfs.h"

#include <string.h>

static const game_module_t *mod(void)
{
    return game_brick_module();
}

static void reset_brick(game_t *g, uint16_t w, uint16_t h)
{
    memset(g, 0, sizeof(*g));
    mod()->reset(g, w, h);
}

static void test_module_registry(void)
{
    const game_module_t *m = game_module_by_id("brick");

    TEST_ASSERT_NOT_NULL(m);
    TEST_ASSERT_EQUAL_STRING("brick", m->id);
    TEST_ASSERT_EQUAL_PTR(m, game_brick_module());
    TEST_ASSERT_EQUAL_PTR(m, game_module_by_id(NULL));
    TEST_ASSERT_EQUAL_PTR(m, game_module_by_id(""));
    TEST_ASSERT_NULL(game_module_by_id("snake"));
    TEST_ASSERT_NOT_NULL(m->reset);
    TEST_ASSERT_NOT_NULL(m->input);
    TEST_ASSERT_NOT_NULL(m->tick);
    TEST_ASSERT_NOT_NULL(m->draw);
}

static void test_brick_ball_moves(void)
{
    game_t g;
    int16_t y0;
    int16_t x0;

    reset_brick(&g, 240, 160);
    TEST_ASSERT_EQUAL_UINT8(GAME_PHASE_PLAY, game_get_phase(&g));
    TEST_ASSERT_EQUAL_UINT8(GAME_LIVES_MAX, game_get_lives(&g));
    TEST_ASSERT_EQUAL_UINT32(0u, game_get_score(&g));
    x0 = game_get_ball_x(&g);
    y0 = game_get_ball_y(&g);
    mod()->tick(&g, 0u);
    TEST_ASSERT_EQUAL_INT16(y0, game_get_ball_y(&g));
    mod()->tick(&g, 16u);
    TEST_ASSERT_TRUE(game_get_ball_x(&g) != x0 || game_get_ball_y(&g) != y0);
    TEST_ASSERT_TRUE(game_get_ball_y(&g) < y0);
}

static void test_brick_hit_scores(void)
{
    game_t g;
    int16_t x;
    int16_t y;
    uint16_t w;
    uint16_t h;
    uint32_t score;

    reset_brick(&g, 240, 160);
    TEST_ASSERT_EQUAL_UINT8(1u, game_brick_alive(&g, 0u));
    game_brick_rect(&g, 0u, &x, &y, &w, &h);
    TEST_ASSERT_TRUE(w > 0u);
    TEST_ASSERT_TRUE(h > 0u);
    game_test_set_ball(&g, (int16_t)(x + 2), (int16_t)(y + 2), 0, 40);
    score = game_get_score(&g);
    mod()->tick(&g, 16u);
    TEST_ASSERT_TRUE(game_get_score(&g) > score);
    TEST_ASSERT_EQUAL_UINT8(0u, game_brick_alive(&g, 0u));
    TEST_ASSERT_TRUE(game_get_high(&g) >= game_get_score(&g));
}

static void test_brick_miss_life(void)
{
    game_t g;
    uint8_t lives;

    reset_brick(&g, 240, 160);
    lives = game_get_lives(&g);
    game_test_set_ball(&g, 20, (int16_t)g.h, 0, 200);
    mod()->tick(&g, 32u);
    TEST_ASSERT_EQUAL_UINT8((uint8_t)(lives - 1u), game_get_lives(&g));
    TEST_ASSERT_EQUAL_UINT8(GAME_PHASE_PLAY, game_get_phase(&g));

    g.lives = 1u;
    game_test_set_ball(&g, 20, (int16_t)g.h, 0, 200);
    mod()->tick(&g, 32u);
    TEST_ASSERT_EQUAL_UINT8(0u, game_get_lives(&g));
    TEST_ASSERT_EQUAL_UINT8(GAME_PHASE_OVER, game_get_phase(&g));
}

static void test_gfx_spy_reset_frame(void)
{
    game_t g;
    uint16_t fb[240 * 160];
    gfx_t fx;
    const gfx_stats_t *st;

    memset(fb, 0, sizeof(fb));
    fx.fb = fb;
    fx.w = 240;
    fx.h = 160;
    fx.stride = 240;
    reset_brick(&g, 240, 160);
    gfx_stats_reset();
    mod()->draw(&g, &fx);
    st = gfx_stats();
    TEST_ASSERT_TRUE(st->clears >= 1u);
    TEST_ASSERT_TRUE(st->fills >= 1u);
    TEST_ASSERT_TRUE(st->blits >= 1u);
    TEST_ASSERT_EQUAL_UINT16(8u, st->last_fill.h);
    TEST_ASSERT_TRUE(st->last_fill.w >= 40u);
}

static void test_gfx_clip(void)
{
    uint16_t fb[16];
    gfx_t fx;
    gfx_rect_t r;
    gfx_sprite_t s;
    uint16_t spr[4] = {0x1111u, 0x0000u, 0x2222u, 0x3333u};

    memset(fb, 0, sizeof(fb));
    fx.fb = fb;
    fx.w = 4;
    fx.h = 4;
    fx.stride = 4;
    gfx_stats_reset();
    gfx_clear(NULL, 0xF800u);
    r.x = 0;
    r.y = 0;
    r.w = 1;
    r.h = 1;
    gfx_fill(NULL, r, 0x07E0u);
    gfx_blit(NULL, 0, 0, NULL);
    gfx_clear(&fx, 0x001Fu);
    TEST_ASSERT_EQUAL_HEX16(0x001Fu, fb[0]);
    r.x = -2;
    r.y = -2;
    r.w = 3;
    r.h = 3;
    gfx_fill(&fx, r, 0xF800u);
    TEST_ASSERT_EQUAL_HEX16(0xF800u, fb[0]);
    s.pixels = spr;
    s.w = 2;
    s.h = 2;
    gfx_blit(&fx, -1, -1, &s);
    TEST_ASSERT_TRUE(gfx_stats()->clears >= 1u);
    TEST_ASSERT_TRUE(gfx_stats()->fills >= 1u);
    TEST_ASSERT_TRUE(gfx_stats()->blits >= 1u);
}

static void test_pause_resume_input(void)
{
    game_t g;
    input_event_t e;
    int16_t y0;
    int16_t px;

    reset_brick(&g, 240, 160);
    y0 = game_get_ball_y(&g);
    game_pause_sim(&g);
    TEST_ASSERT_EQUAL_UINT8(GAME_PHASE_PAUSE, game_get_phase(&g));
    mod()->tick(&g, 16u);
    TEST_ASSERT_EQUAL_INT16(y0, game_get_ball_y(&g));
    e.kind = INPUT_PTR_DOWN;
    e.x = 200;
    e.y = (int16_t)(g.h - 8);
    e.id = 0u;
    e.t_ms = 0u;
    px = game_get_paddle_x(&g);
    mod()->input(&g, &e);
    TEST_ASSERT_EQUAL_INT16(px, game_get_paddle_x(&g));
    game_resume_sim(&g);
    TEST_ASSERT_EQUAL_UINT8(GAME_PHASE_PLAY, game_get_phase(&g));
    mod()->input(&g, &e);
    TEST_ASSERT_TRUE(game_get_paddle_x(&g) != px);
    mod()->tick(&g, 16u);
    TEST_ASSERT_TRUE(game_get_ball_y(&g) != y0);
}

static void test_accessors_null(void)
{
    int16_t x = 9;
    int16_t y = 9;
    uint16_t w = 9u;
    uint16_t h = 9u;

    TEST_ASSERT_EQUAL_UINT8(GAME_PHASE_OVER, game_get_phase(NULL));
    TEST_ASSERT_EQUAL_UINT32(0u, game_get_score(NULL));
    TEST_ASSERT_EQUAL_UINT32(0u, game_get_high(NULL));
    TEST_ASSERT_EQUAL_UINT8(0u, game_get_lives(NULL));
    TEST_ASSERT_EQUAL_INT16(0, game_get_ball_x(NULL));
    TEST_ASSERT_EQUAL_INT16(0, game_get_ball_y(NULL));
    TEST_ASSERT_EQUAL_INT16(0, game_get_paddle_x(NULL));
    TEST_ASSERT_EQUAL_UINT8(0u, game_brick_alive(NULL, 0u));
    TEST_ASSERT_EQUAL_UINT8(0u, game_brick_alive(game_self(), 99u));
    game_set_high(NULL, 1u);
    game_pause_sim(NULL);
    game_resume_sim(NULL);
    game_test_set_ball(NULL, 0, 0, 0, 0);
    game_test_set_brick(NULL, 0u, 1u);
    game_test_clear_bricks(NULL);
    game_resize_layout(NULL, 100, 100);
    game_brick_rect(NULL, 0u, &x, &y, &w, &h);
    TEST_ASSERT_EQUAL_INT16(0, x);
    TEST_ASSERT_EQUAL_UINT16(0u, w);
}

static void test_app_back_and_save(void)
{
    game_t *g;

    TEST_ASSERT_EQUAL_INT(ERR_OK, vfs_mount());
    game_open(240, 160);
    TEST_ASSERT_EQUAL_STRING("brick", game_module()->id);
    TEST_ASSERT_NOT_NULL(game_pixels());
    TEST_ASSERT_EQUAL_UINT16(240u, game_field_w());
    TEST_ASSERT_EQUAL_UINT16(160u, game_field_h());
    TEST_ASSERT_EQUAL_UINT8(GAME_PHASE_PLAY, game_phase());
    TEST_ASSERT_EQUAL_UINT8(1u, game_on_back());
    TEST_ASSERT_EQUAL_UINT8(GAME_PHASE_PAUSE, game_phase());
    TEST_ASSERT_EQUAL_UINT8(0u, game_on_back());
    game_resume();
    TEST_ASSERT_EQUAL_UINT8(GAME_PHASE_PLAY, game_phase());

    g = game_self();
    TEST_ASSERT_NOT_NULL(g);
    g->score = 70u;
    g->lives = 1u;
    game_test_set_ball(g, 20, (int16_t)g->h, 0, 200);
    game_step(32u);
    TEST_ASSERT_EQUAL_UINT8(GAME_PHASE_OVER, game_phase());
    TEST_ASSERT_TRUE(game_high() >= 70u);
    game_close();

    game_open(240, 160);
    TEST_ASSERT_TRUE(game_high() >= 70u);
    TEST_ASSERT_EQUAL_STRING("70", game_high_str());
    game_new();
    TEST_ASSERT_EQUAL_UINT8(GAME_PHASE_PLAY, game_phase());
    TEST_ASSERT_EQUAL_UINT32(0u, game_score());
    TEST_ASSERT_TRUE(game_high() >= 70u);
    game_resize(480, 200);
    TEST_ASSERT_EQUAL_UINT16(480u, game_field_w());
    game_close();
}

static void test_walls_and_small_field(void)
{
    game_t g;
    input_event_t e;

    reset_brick(&g, 140, 120);
    TEST_ASSERT_TRUE(g.rows <= 3u);
    game_test_set_ball(&g, -2, 8, -80, 0);
    mod()->tick(&g, 16u);
    TEST_ASSERT_TRUE(game_get_ball_x(&g) >= 0);
    game_test_set_ball(&g, (int16_t)(g.w - 2), 8, 80, 0);
    mod()->tick(&g, 16u);
    TEST_ASSERT_TRUE(game_get_ball_x(&g) + 6 <= (int16_t)g.w);
    game_test_set_ball(&g, 20, -2, 0, -80);
    mod()->tick(&g, 16u);
    TEST_ASSERT_TRUE(game_get_ball_y(&g) >= 0);

    e.kind = INPUT_PTR_UP;
    e.x = 80;
    e.y = (int16_t)(g.h - 4);
    e.id = 0u;
    e.t_ms = 0u;
    mod()->input(&g, &e);
    e.kind = INPUT_PTR_DOWN;
    e.y = 4;
    mod()->input(&g, &e);
    game_resize_layout(&g, 480, 200);
    TEST_ASSERT_EQUAL_UINT16(480u, g.w);
}

static void test_level_clear_refill(void)
{
    game_t g;
    int16_t x;
    int16_t y;
    uint16_t w;
    uint16_t h;

    reset_brick(&g, 240, 180);
    game_test_clear_bricks(&g);
    game_test_set_brick(&g, 0u, 1u);
    game_brick_rect(&g, 0u, &x, &y, &w, &h);
    game_test_set_ball(&g, (int16_t)(x + 2), (int16_t)(y + 2), 0, 40);
    mod()->tick(&g, 16u);
    TEST_ASSERT_EQUAL_UINT8(1u, game_brick_alive(&g, 1u));
}

void test_game_run(void)
{
    UnitySetTestFile(__FILE__);
    RUN_TEST(test_module_registry);
    RUN_TEST(test_brick_ball_moves);
    RUN_TEST(test_brick_hit_scores);
    RUN_TEST(test_brick_miss_life);
    RUN_TEST(test_gfx_spy_reset_frame);
    RUN_TEST(test_gfx_clip);
    RUN_TEST(test_pause_resume_input);
    RUN_TEST(test_accessors_null);
    RUN_TEST(test_app_back_and_save);
    RUN_TEST(test_walls_and_small_field);
    RUN_TEST(test_level_clear_refill);
}
