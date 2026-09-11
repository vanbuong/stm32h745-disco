#include "bsp/board.h"
#include "svc/memtest.h"

#include "stm32h745_regs.h"

static void delay(void)
{
    volatile uint32_t n = board_sysclk_hz() / 8u;
    while (n > 0u) {
        n--;
    }
}

static void led_init(void)
{
    RCC_AHB4ENR |= RCC_AHB4ENR_GPIOIEN;
    GPIO_MODER(GPIOI_BASE) &= ~(3u << 26);
    GPIO_MODER(GPIOI_BASE) |= (1u << 26);
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

int main(void)
{
    uint8_t id[3] = {0, 0, 0};
    uint32_t word = 0;
    uint32_t fail_off = 0;
    err_t e;

    led_init();
    board_console_init(BOARD_HSI_HZ);
    board_console_puts("M7 stm32h745-disco s1\r\n");

    e = board_clock_init();
    board_console_init(board_pclk1_hz());
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

    for (;;) {
        GPIO_BSRR(GPIOI_BASE) = (1u << 13);
        delay();
        GPIO_BSRR(GPIOI_BASE) = (1u << 29);
        delay();
    }
}
