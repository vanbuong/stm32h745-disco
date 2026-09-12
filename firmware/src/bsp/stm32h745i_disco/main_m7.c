#include "bsp/board.h"
#include "hal/disp.h"
#include "hal/input.h"
#include "svc/memtest.h"
#include "svc/net.h"
#include "svc/time.h"
#include "svc/vfs.h"
#include "ui/backend.h"
#include "ui/shell.h"

#include "cube.h"

#include <string.h>

#define VFS_CHUNK 16384u
#define VFS_FILE_BYTES (1024u * 1024u)
#define VFS_READ_GOAL (8u * 1024u * 1024u)
#define UI_FPS_MS 2000u

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
        if (n > 64u) {
            break;
        }
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

static uint8_t vfs_bringup(void)
{
    err_t e;

    board_console_puts("emmc\r\n");
    e = board_emmc_init();
    log_err("emmc", e);
    if (e != ERR_OK) {
        return 0u;
    }
    log_kv("emmc_blocks", board_emmc_block_count());
    log_kv("emmc_hz", board_emmc_clock_hz());
    board_console_puts("vfs mount\r\n");
    e = vfs_mount();
    if (e == ERR_CORRUPT) {
        board_console_puts("vfs fmt\r\n");
        e = vfs_format();
    }
    log_err("vfs", e);
    if (e != ERR_OK) {
        log_kv("emmc_err", board_emmc_last_error());
    }
    if (e != ERR_OK) {
        return 0u;
    }
    if (vfs_formatted_on_mount() != 0u) {
        board_console_puts("vfs fmt done\r\n");
    }
    log_err("vfs_rw", vfs_selftest());
    vfs_list_user();
    vfs_bench();
    return 1u;
}

static void ui_fps_probe(void)
{
    uint32_t t0;
    uint32_t dt;
    uint32_t f0;
    uint32_t n;
    uint32_t fps;

    f0 = ui_backend_frames();
    t0 = HAL_GetTick();
    while ((HAL_GetTick() - t0) < UI_FPS_MS) {
        ui_backend_invalidate();
        ui_backend_handler();
        board_ipc_poll(HAL_GetTick());
    }
    dt = HAL_GetTick() - t0;
    n = ui_backend_frames() - f0;
    fps = (dt == 0u) ? 0u : ((n * 1000u) / dt);
    board_console_puts("ui_frames ");
    put_u32(n);
    board_console_puts("\r\n");
    board_console_puts("ui_ms ");
    put_u32(dt);
    board_console_puts("\r\n");
    board_console_puts("ui_fps ");
    put_u32(fps);
    board_console_puts("\r\n");
    if (fps >= 20u) {
        board_console_puts("ui_fps ok\r\n");
    } else {
        board_console_puts("ui_fps fail\r\n");
    }
}

int main(void)
{
    uint8_t id[3] = {0, 0, 0};
    uint32_t word = 0;
    uint32_t fail_off = 0;
    uint32_t blink_at = 0;
    uint32_t last_ms;
    uint8_t led_on = 0;
    uint8_t vfs_ok;
    err_t e;
    err_t ui_e;

    board_cm4_wait_stop();
    HAL_Init();
    led_init();
    board_console_init(0);

    e = board_clock_init();
    board_console_init(0);
    board_console_puts("M7 stm32h745-disco s9\r\n");
    if (board_cm4_saw_stop() == 0u) {
        board_console_puts("d2 stop to\r\n");
    }
    if (board_cm4_saw_wake() == 0u) {
        board_console_puts("d2 wake to\r\n");
    }
    log_err("clk", e);
    log_kv("sysclk", board_sysclk_hz());

    board_mpu_init();
    board_console_puts("mpu on\r\n");
    board_cache_init();
    board_console_puts("cache on\r\n");

    e = board_audio_clock_init(44100u);
    log_err("sai_clk", e);
    e = board_ipc_init();
    log_err("ipc", e);

    board_console_puts("sdram\r\n");
    e = board_sdram_init();
    log_err("sdram", e);
    if (e == ERR_OK) {
        board_console_puts("walk\r\n");
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

    e = board_disp_init();
    log_err("disp", e);
    e = board_input_init();
    log_err("input", e);
    if (board_touch_present() != 0u) {
        board_console_puts("touch ");
        board_console_puts(board_touch_name());
        board_console_puts("\r\n");
        board_touch_diag();
    } else {
        board_console_puts("touch none\r\n");
    }

    shell_init();
    shell_status_set_storage(0u);
    shell_status_set_m4(0u);
    ui_e = ui_backend_init();
    log_err("ui", ui_e);
    if (ui_e == ERR_OK) {
        board_console_puts("shell ready\r\n");
        ui_backend_handler();
        ui_fps_probe();
    }

    vfs_ok = vfs_bringup();
    shell_status_set_storage(vfs_ok);

    e = time_init();
    log_err("rtc", e);
    e = net_service_init();
    log_err("net", e);

    last_ms = HAL_GetTick();
    blink_at = last_ms + 250u;
    for (;;) {
        uint32_t now = HAL_GetTick();
        uint32_t dt = now - last_ms;

        last_ms = now;
        board_ipc_poll(now);
        shell_status_set_m4(board_ipc_peer_alive(now));
        shell_tick(dt);
        if (ui_e == ERR_OK) {
            ui_backend_handler();
        }

        if ((int32_t)(now - blink_at) >= 0) {
            blink_at = now + 250u;
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
