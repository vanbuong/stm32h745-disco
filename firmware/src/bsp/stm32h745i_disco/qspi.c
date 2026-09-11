#include "bsp/board.h"

#include "stm32h745_regs.h"

/*
 * Dual QSPI NOR on STM32H745I-DISCO (ST BSP pin map). BK2 PH2/PH3 is the
 * Ethernet CRS/COL mux; Sprint 1 keeps dual-flash, Ethernet full MII later.
 * Mmap smoke is a 1-1-1 READ of bank 1. Do not execute blank 0xFF NOR.
 */

static err_t qspi_wait_not_busy(void)
{
    uint32_t t = 2000000u;
    while ((QUADSPI_SR & QUADSPI_SR_BUSY) != 0u) {
        if (t == 0u) {
            return ERR_TIMEOUT;
        }
        t--;
    }
    return ERR_OK;
}

static err_t qspi_wait_tc(void)
{
    uint32_t t = 2000000u;
    while ((QUADSPI_SR & QUADSPI_SR_TCF) == 0u) {
        if (t == 0u) {
            return ERR_TIMEOUT;
        }
        t--;
    }
    QUADSPI_FCR = QUADSPI_FCR_CTCF;
    return ERR_OK;
}

static err_t qspi_abort(void)
{
    QUADSPI_CR |= QUADSPI_CR_ABORT;
    return qspi_wait_not_busy();
}

static void qspi_gpio(void)
{
    RCC_AHB4ENR |=
        RCC_AHB4ENR_GPIODEN | RCC_AHB4ENR_GPIOFEN | RCC_AHB4ENR_GPIOGEN | RCC_AHB4ENR_GPIOHEN;
    (void)RCC_AHB4ENR;

    gpio_af(GPIOF_BASE, 10u, 9u, 0u); /* CLK PF10 AF9 */
    gpio_af(GPIOG_BASE, 6u, 10u, 1u); /* BK1/BK2 NCS PG6 AF10 pull-up */
    gpio_af(GPIOD_BASE, 11u, 9u, 0u); /* BK1 D0 PD11 AF9 */
    gpio_af(GPIOF_BASE, 9u, 10u, 0u); /* BK1 D1 PF9 AF10 */
    gpio_af(GPIOF_BASE, 7u, 9u, 0u);  /* BK1 D2 PF7 AF9 */
    gpio_af(GPIOF_BASE, 6u, 9u, 0u);  /* BK1 D3 PF6 AF9 */
    gpio_af(GPIOH_BASE, 2u, 9u, 0u);  /* BK2 D0 PH2 AF9 */
    gpio_af(GPIOH_BASE, 3u, 9u, 0u);  /* BK2 D1 PH3 AF9 */
    gpio_af(GPIOG_BASE, 9u, 9u, 0u);  /* BK2 D2 PG9 AF9 */
    gpio_af(GPIOG_BASE, 14u, 9u, 0u); /* BK2 D3 PG14 AF9 */
}

static err_t qspi_cmd_write(uint8_t instr)
{
    err_t e = qspi_abort();
    if (e != ERR_OK) {
        return e;
    }
    QUADSPI_DLR = 0;
    QUADSPI_CCR = (uint32_t)instr | (1u << 8); /* IMODE 1-line, FMODE write */
    e = qspi_wait_tc();
    if (e != ERR_OK) {
        return e;
    }
    return qspi_wait_not_busy();
}

err_t board_qspi_init(void)
{
    err_t e;

    qspi_gpio();

    RCC_AHB3ENR |= RCC_AHB3ENR_QSPIEN;
    RCC_AHB3RSTR |= RCC_AHB3RSTR_QSPIRST;
    RCC_AHB3RSTR &= ~RCC_AHB3RSTR_QSPIRST;

    /* Prescaler 3 → HCLK/4 ≈ 60 MHz. FSIZE 25 → 64 MB (one 512 Mbit die). */
    QUADSPI_CR = (3u << 24) | QUADSPI_CR_SSHIFT;
    QUADSPI_DCR = (25u << 16) | (3u << 8);
    QUADSPI_CR |= QUADSPI_CR_EN;

    e = qspi_cmd_write(0x66u); /* reset enable */
    if (e != ERR_OK) {
        return e;
    }
    e = qspi_cmd_write(0x99u); /* reset memory */
    if (e != ERR_OK) {
        return e;
    }
    {
        volatile uint32_t n = 20000u;
        while (n > 0u) {
            n--;
        }
    }

    e = qspi_abort();
    if (e != ERR_OK) {
        return e;
    }
    /* 1-1-1 READ (0x03), 24-bit address, memory-mapped. */
    QUADSPI_CCR = 0x03u | (1u << 8) | (1u << 10) | (2u << 12) | (1u << 24) | (3u << 26);
    return qspi_wait_not_busy();
}

err_t board_qspi_read_id(uint8_t id[3])
{
    err_t e;
    unsigned i;

    if (id == NULL) {
        return ERR_INVAL;
    }

    e = qspi_abort();
    if (e != ERR_OK) {
        return e;
    }

    QUADSPI_DLR = 2u;
    QUADSPI_CCR = 0x9Fu | (1u << 8) | (1u << 24) | (1u << 26); /* indirect read */
    e = qspi_wait_tc();
    if (e != ERR_OK) {
        return e;
    }
    for (i = 0; i < 3u; i++) {
        id[i] = QUADSPI_DR8;
    }
    return qspi_wait_not_busy();
}

err_t board_qspi_mmap_probe(uint32_t *first_word)
{
    err_t e;
    volatile uint32_t *q = (volatile uint32_t *)BOARD_QSPI_BASE;

    e = qspi_abort();
    if (e != ERR_OK) {
        return e;
    }
    QUADSPI_CCR = 0x03u | (1u << 8) | (1u << 10) | (2u << 12) | (1u << 24) | (3u << 26);
    e = qspi_wait_not_busy();
    if (e != ERR_OK) {
        return e;
    }
    if (first_word != NULL) {
        *first_word = *q;
    } else {
        (void)*q;
    }
    return ERR_OK;
}
