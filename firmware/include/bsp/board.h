#ifndef BOARD_H
#define BOARD_H

#include "err.h"

#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

#define BOARD_SYSCLK_HZ 400000000u
#define BOARD_HCLK_HZ 200000000u
#define BOARD_PCLK1_HZ 100000000u
#define BOARD_HSI_HZ 64000000u
#define BOARD_UART_BAUD 115200u
#define BOARD_ZNP_UART_BAUD 921600u

#define BOARD_SDRAM_BASE 0xD0000000u
/* MB1381 IS42S32800G (or 16-bit equivalent). 16-bit FMC, 12 row × 9 col ×
 * 4 banks maps the full 16 MB. Cube examples use 8 col (8 MB); walking
 * past that window with an 8 MB MPU region hangs the boot memtest. */
#define BOARD_SDRAM_BYTES (16u * 1024u * 1024u)
#define BOARD_SDRAM_CHIP_BYTES BOARD_SDRAM_BYTES
#define BOARD_QSPI_BASE 0x90000000u
#define BOARD_SRAM4_BASE 0x38000000u
#define BOARD_SRAM4_BYTES (64u * 1024u)
#define BOARD_M4_SYSCLK_HZ BOARD_HCLK_HZ
#define BOARD_HSEM_M7_TO_M4 0u
#define BOARD_HSEM_M4_TO_M7 1u

#define BOARD_LCD_W 480u
#define BOARD_LCD_H 272u
#define BOARD_LCD_BPP 2u
#define BOARD_FB_BYTES (BOARD_LCD_W * BOARD_LCD_H * BOARD_LCD_BPP)
#define BOARD_FB_PITCH 0x40000u
#define BOARD_FB0_BASE BOARD_SDRAM_BASE
#define BOARD_FB1_BASE (BOARD_SDRAM_BASE + BOARD_FB_PITCH)

#define BOARD_FT5336_ADDR 0x70u
#define BOARD_GT911_ADDR 0xBAu
#define BOARD_GT911_ADDR_ALT 0x28u
#define BOARD_WM8994_ADDR 0x34u

#define BOARD_LVGL_MEM_BASE 0x24010000u
#define BOARD_LVGL_MEM_BYTES (96u * 1024u)

uint32_t board_sysclk_hz(void);
uint32_t board_pclk1_hz(void);
uint32_t board_millis(void);

err_t board_clock_init(void);
void board_cm4_boot(void);
void board_cm4_wait_stop(void);
uint8_t board_cm4_saw_stop(void);
uint8_t board_cm4_saw_wake(void);
void board_hsem_init(void);
void board_hsem_notify(uint32_t sem);
void board_hsem_wake(uint32_t sem);
uint8_t board_hsem_poll(uint32_t sem);
err_t board_ipc_init(void);
void board_ipc_poll(uint32_t now_ms);
uint8_t board_ipc_peer_alive(uint32_t now_ms);
void board_console_init(uint32_t pclk1_hz);
void board_console_puts(const char *s);
void board_console_put_hex32(uint32_t v);

void board_mpu_init(void);
void board_cache_init(void);
void board_cache_d_disable(void);
void board_cache_d_enable(void);
void board_cache_invalidate_d(void);

err_t board_sdram_init(void);
err_t board_qspi_init(void);
err_t board_qspi_read_id(uint8_t id[3]);
err_t board_qspi_mmap_probe(uint32_t *first_word);

uint32_t board_mpu_faults(void);
uint32_t board_mpu_last_mmfar(void);
err_t board_mpu_selftest(void);

err_t board_disp_init(void);
void board_disp_show(const void *fb);
err_t board_i2c4_init(void);
err_t board_i2c4_lock(void);
void board_i2c4_unlock(void);
int32_t board_i2c4_read_reg(uint16_t addr, uint16_t reg, uint8_t *data, uint16_t len);
int32_t board_i2c4_write_reg(uint16_t addr, uint16_t reg, uint8_t *data, uint16_t len);
int32_t board_i2c4_read16(uint16_t addr, uint16_t reg, uint8_t *data, uint16_t len);
int32_t board_i2c4_write16(uint16_t addr, uint16_t reg, uint8_t *data, uint16_t len);

err_t board_ipc_send(uint8_t dst, uint16_t type, const void *payload, uint16_t len);
void *board_ipc_base(void);

err_t board_audio_clock_init(uint32_t sample_hz);
err_t board_codec_init(uint32_t sample_hz, uint8_t vol_pct);
err_t board_codec_volume(uint8_t pct);
err_t board_codec_play(void);
err_t board_codec_pause(void);
err_t board_codec_stop(void);
err_t board_input_init(void);
uint8_t board_touch_present(void);
const char *board_touch_name(void);
void board_touch_diag(void);

err_t board_emmc_init(void);
err_t board_emmc_fallback(void);
int board_emmc_ready(void);
uint32_t board_emmc_block_count(void);
uint32_t board_emmc_last_error(void);
uint32_t board_emmc_clock_hz(void);

err_t board_rtc_init(void);
err_t board_rtc_get(uint8_t *hh, uint8_t *mm, uint8_t *ss);
err_t board_rtc_set(uint8_t hh, uint8_t mm, uint8_t ss);
err_t board_rtc_get_date(uint16_t *year, uint8_t *month, uint8_t *day, uint8_t *hh, uint8_t *mm,
                         uint8_t *ss);
err_t board_rtc_set_date(uint16_t year, uint8_t month, uint8_t day, uint8_t hh, uint8_t mm,
                         uint8_t ss);

#ifdef __cplusplus
}
#endif

#endif /* BOARD_H */
