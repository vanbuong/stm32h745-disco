#include "bsp/board.h"
#include "bsp/disp_geom.h"
#include "hal/disp.h"
#include "hal/input.h"
#include "svc/memtest.h"
#include "svc/vfs.h"

#include "cube.h"

#include <string.h>

#define FPS_FRAMES 60u
#define CROSS_ARM 8
#define VFS_CHUNK 16384u
#define VFS_FILE_BYTES (1024u * 1024u)
#define VFS_READ_GOAL (8u * 1024u * 1024u)

static void led_init(void)
{
    LL_AHB4_GRP1_EnableClock(LL_AHB4_GRP1_PERIPH_GPIOI);
    LL_GPIO_SetPinMode(GPIOI, LL_GPIO_PIN_13, LL_GPIO_MODE_OUTPUT);
}

static void log_kv(const char *k, uint32_t v)
{
    board_console_puts(k);
    board_console_puts(" ");
    board_console_put_hex32(v);
    board_console_puts("\r\n");
}

static void log_err(const char *what, err_t e)
{
    board_console_puts(what);
    if (e == ERR_OK) {
        board_console_puts(" ok\r\n");
    } else {
        board_console_puts(" fail ");
        board_console_put_hex32((uint32_t)(int32_t)e);
        board_console_puts("\r\n");
    }
}

static void put_u32(uint32_t v)
{
    char buf[11];
    int i = 10;

    buf[10] = '\0';
    if (v == 0u) {
        board_console_puts("0");
        return;
    }
    while (v > 0u && i > 0) {
        buf[--i] = (char)('0' + (v % 10u));
        v /= 10u;
    }
    board_console_puts(&buf[i]);
}

static void draw_bars(void)
{
    static const uint8_t rgb[8][3] = {
        {255, 255, 255}, {255, 255, 0}, {0, 255, 255}, {0, 255, 0},
        {255, 0, 255},   {255, 0, 0},   {0, 0, 255},   {0, 0, 0},
    };
    disp_rect_t r;
    unsigned i;

    r.y = 0u;
    r.h = BOARD_LCD_H;
    r.w = (uint16_t)(BOARD_LCD_W / 8u);
    for (i = 0u; i < 8u; i++) {
        r.x = (uint16_t)(i * r.w);
        (void)disp_fill(&r, disp_rgb565(rgb[i][0], rgb[i][1], rgb[i][2]));
    }
}

static void draw_cross(int16_t x, int16_t y)
{
    disp_rect_t h;
    disp_rect_t v;
    int16_t x0 = (int16_t)(x - CROSS_ARM);
    int16_t y0 = (int16_t)(y - CROSS_ARM);

    if (x0 < 0) {
        x0 = 0;
    }
    if (y0 < 0) {
        y0 = 0;
    }
    h.x = (uint16_t)x0;
    h.y = (uint16_t)y;
    h.w = (uint16_t)(CROSS_ARM * 2 + 1);
    h.h = 1u;
    v.x = (uint16_t)x;
    v.y = (uint16_t)y0;
    v.w = 1u;
    v.h = (uint16_t)(CROSS_ARM * 2 + 1);
    (void)disp_fill(&h, disp_rgb565(255u, 255u, 255u));
    (void)disp_fill(&v, disp_rgb565(255u, 255u, 255u));
}

static void fps_fill(void)
{
    disp_rect_t full = {0u, 0u, BOARD_LCD_W, BOARD_LCD_H};
    uint32_t t0;
    uint32_t dt;
    uint32_t fps;
    uint32_t i;
    uint16_t red = disp_rgb565(255u, 0u, 0u);
    uint16_t blue = disp_rgb565(0u, 0u, 255u);

    t0 = HAL_GetTick();
    for (i = 0u; i < FPS_FRAMES; i++) {
        (void)disp_fill(&full, ((i & 1u) != 0u) ? blue : red);
        disp_swap();
    }
    dt = HAL_GetTick() - t0;
    fps = (dt == 0u) ? 0u : ((FPS_FRAMES * 1000u) / dt);

    board_console_puts("fps_ms ");
    put_u32(dt);
    board_console_puts("\r\n");
    board_console_puts("fps ");
    put_u32(fps);
    board_console_puts("\r\n");
    if (fps >= 30u) {
        board_console_puts("fps ok\r\n");
    } else {
        board_console_puts("fps fail\r\n");
    }
}

static uint8_t g_chunk[VFS_CHUNK];

static void vfs_list_user(void)
{
    vfs_dir_t d = -1;
    vfs_dirent_t ent;
    unsigned n = 0;
    err_t e;

    e = vfs_opendir("/user", &d);
    log_err("vfs_dir", e);
    if (e != ERR_OK) {
        return;
    }
    for (;;) {
        e = vfs_readdir(d, &ent);
        if (e == ERR_NOENT) {
            break;
        }
        if (e != ERR_OK) {
            log_err("vfs_readent", e);
            break;
        }
        n++;
        if (n <= 8u) {
            board_console_puts("vfs_ent ");
            board_console_puts(ent.name);
            board_console_puts("\r\n");
        }
    }
    (void)vfs_closedir(d);
    board_console_puts("vfs_ents ");
    put_u32(n);
    board_console_puts("\r\n");
}

static err_t vfs_ensure_bench(void)
{
    vfs_stat_t st;
    vfs_file_t fd = -1;
    uint32_t left;
    err_t e;

    if (vfs_stat("/user/s3.bin", &st) == ERR_OK && st.size >= VFS_FILE_BYTES) {
        return ERR_OK;
    }
    memset(g_chunk, 0xA5, sizeof(g_chunk));
    e = vfs_open("/user/s3.bin", VFS_O_WR | VFS_O_CREAT | VFS_O_TRUNC, &fd);
    if (e != ERR_OK) {
        return e;
    }
    left = VFS_FILE_BYTES;
    while (left > 0u) {
        size_t n = (left > VFS_CHUNK) ? VFS_CHUNK : left;
        size_t put = 0u;
        e = vfs_write(fd, g_chunk, n, &put);
        if (e != ERR_OK || put != n) {
            (void)vfs_close(fd);
            return (e != ERR_OK) ? e : ERR_IO;
        }
        left -= (uint32_t)n;
    }
    return vfs_close(fd);
}

static void vfs_bench(void)
{
    vfs_file_t fd = -1;
    uint32_t t0;
    uint32_t dt;
    uint32_t total = 0u;
    uint32_t kBps;
    err_t e;

    e = vfs_ensure_bench();
    log_err("vfs_mk", e);
    if (e != ERR_OK) {
        return;
    }
    e = vfs_open("/user/s3.bin", VFS_O_RD, &fd);
    log_err("vfs_open", e);
    if (e != ERR_OK) {
        return;
    }
    t0 = HAL_GetTick();
    while (total < VFS_READ_GOAL) {
        size_t got = 0u;
        e = vfs_read(fd, g_chunk, VFS_CHUNK, &got);
        if (e != ERR_OK) {
            log_err("vfs_rd", e);
            break;
        }
        if (got == 0u) {
            if (vfs_seek(fd, 0u) != ERR_OK) {
                break;
            }
            continue;
        }
        total += (uint32_t)got;
    }
    dt = HAL_GetTick() - t0;
    (void)vfs_close(fd);
    board_console_puts("vfs_bytes ");
    put_u32(total);
    board_console_puts("\r\n");
    board_console_puts("vfs_ms ");
    put_u32(dt);
    board_console_puts("\r\n");
    kBps = (dt == 0u) ? 0u : ((total / 1024u) * 1000u) / dt;
    board_console_puts("vfs_kBps ");
    put_u32(kBps);
    board_console_puts("\r\n");
    if (kBps >= 15360u) {
        board_console_puts("vfs_mbps ok\r\n");
    } else {
        board_console_puts("vfs_mbps fail\r\n");
    }
}

static void vfs_bringup(void)
{
    err_t e;

    e = board_emmc_init();
    log_err("emmc", e);
    if (e != ERR_OK) {
        return;
    }
    log_kv("emmc_blocks", board_emmc_block_count());
    e = vfs_mount();
    log_err("vfs", e);
    if (e != ERR_OK) {
        return;
    }
    vfs_list_user();
    vfs_bench();
}

int main(void)
{
    uint8_t id[3] = {0, 0, 0};
    uint32_t word = 0;
    uint32_t fail_off = 0;
    uint32_t blink_at = 0;
    uint8_t led_on = 0;
    uint8_t have_ptr = 0;
    int16_t px = 0;
    int16_t py = 0;
    err_t e;

    HAL_Init();
    led_init();
    board_console_init(0);
    board_console_puts("M7 stm32h745-disco s3\r\n");

    e = board_clock_init();
    board_console_init(0);
    log_err("clk", e);
    log_kv("sysclk", board_sysclk_hz());

    board_mpu_init();
    board_console_puts("mpu on\r\n");
    board_cache_init();
    board_console_puts("cache on\r\n");

    e = board_sdram_init();
    log_err("sdram", e);
    if (e == ERR_OK) {
        board_cache_d_disable();
        e = memtest_walking((volatile uint32_t *)BOARD_SDRAM_BASE, BOARD_SDRAM_BYTES / 4u,
                            &fail_off);
        board_cache_d_enable();
        log_err("walk", e);
        if (e != ERR_OK) {
            log_kv("walk_off", fail_off);
        }
    }

    e = board_qspi_init();
    log_err("qspi", e);
    if (e == ERR_OK) {
        e = board_qspi_read_id(id);
        board_console_puts("qspi id ");
        board_console_put_hex32(((uint32_t)id[0] << 16) | ((uint32_t)id[1] << 8) | id[2]);
        board_console_puts("\r\n");
        log_err("qspi_id", e);
        e = board_qspi_mmap_probe(&word);
        log_err("qspi_mmap", e);
        log_kv("qspi_word", word);
    }

    e = board_mpu_selftest();
    log_err("mpu_test", e);
    log_kv("mpu_faults", board_mpu_faults());
    log_kv("mpu_mmfar", board_mpu_last_mmfar());

    vfs_bringup();

    e = board_disp_init();
    log_err("disp", e);
    e = board_input_init();
    log_err("input", e);
    if (board_touch_present() != 0u) {
        board_console_puts("touch ok\r\n");
    } else {
        board_console_puts("touch none\r\n");
    }

    draw_bars();
    disp_swap();
    board_console_puts("bars ok\r\n");
    fps_fill();
    draw_bars();
    disp_swap();

    blink_at = HAL_GetTick() + 250u;
    for (;;) {
        input_event_t ev;

        if (input_poll(&ev)) {
            if (ev.kind == INPUT_PTR_UP) {
                have_ptr = 0u;
            } else if (ev.kind == INPUT_PTR_DOWN || ev.kind == INPUT_PTR_MOVE) {
                have_ptr = 1u;
                px = ev.x;
                py = ev.y;
            }
            draw_bars();
            if (have_ptr != 0u) {
                draw_cross(px, py);
            }
            disp_swap();
        }

        if ((int32_t)(HAL_GetTick() - blink_at) >= 0) {
            blink_at = HAL_GetTick() + 250u;
            if (led_on != 0u) {
                LL_GPIO_ResetOutputPin(GPIOI, LL_GPIO_PIN_13);
                led_on = 0u;
            } else {
                LL_GPIO_SetOutputPin(GPIOI, LL_GPIO_PIN_13);
                led_on = 1u;
            }
        }
    }
}
