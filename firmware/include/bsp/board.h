#ifndef BOARD_H
#define BOARD_H

#include "err.h"

#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

#define BOARD_SYSCLK_HZ 480000000u
#define BOARD_HCLK_HZ 240000000u
#define BOARD_PCLK1_HZ 120000000u
#define BOARD_HSI_HZ 64000000u
#define BOARD_UART_BAUD 115200u

#define BOARD_SDRAM_BASE 0xD0000000u
#define BOARD_SDRAM_BYTES (8u * 1024u * 1024u)
#define BOARD_QSPI_BASE 0x90000000u
#define BOARD_SRAM4_BASE 0x38000000u
#define BOARD_SRAM4_BYTES (64u * 1024u)

#define BOARD_LCD_W 480u
#define BOARD_LCD_H 272u
#define BOARD_LCD_BPP 2u
#define BOARD_FB_BYTES (BOARD_LCD_W * BOARD_LCD_H * BOARD_LCD_BPP)
#define BOARD_FB_PITCH 0x40000u
#define BOARD_FB0_BASE BOARD_SDRAM_BASE
#define BOARD_FB1_BASE (BOARD_SDRAM_BASE + BOARD_FB_PITCH)

#define BOARD_FT5336_ADDR 0x70u

#define BOARD_LVGL_MEM_BASE 0x24010000u
#define BOARD_LVGL_MEM_BYTES (96u * 1024u)

uint32_t board_sysclk_hz(void);
uint32_t board_pclk1_hz(void);
uint32_t board_millis(void);

err_t board_clock_init(void);
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
err_t board_input_init(void);
uint8_t board_touch_present(void);

err_t board_emmc_init(void);
int board_emmc_ready(void);
uint32_t board_emmc_block_count(void);

#ifdef __cplusplus
}
#endif

#endif /* BOARD_H */
