#include "bsp/board.h"

#include "stm32h745_regs.h"

static uint32_t g_sysclk_hz = BOARD_HSI_HZ;
static uint32_t g_pclk1_hz = BOARD_HSI_HZ;

uint32_t board_sysclk_hz(void)
{
    return g_sysclk_hz;
}

uint32_t board_pclk1_hz(void)
{
    return g_pclk1_hz;
}

static int wait_set(volatile uint32_t *reg, uint32_t bit, uint32_t spins)
{
    while (spins > 0u) {
        if ((*reg & bit) != 0u) {
            return 1;
        }
        spins--;
    }
    return 0;
}

static void fmc_disable_bank1(void)
{
    RCC_AHB3ENR |= RCC_AHB3ENR_FMCEN;
    (void)RCC_AHB3ENR;
    /* Stop CPU speculation on FMC bank1 (blocks the bus ~24 us). */
    FMC_BTCR0 = 0x000030D2u;
}

static err_t pwr_vos0(void)
{
    RCC_APB4ENR |= RCC_APB4ENR_SYSCFGEN;
    (void)RCC_APB4ENR;

    /*
     * Discovery is SMPS hardware. VOS0 (480 MHz) needs LDO in the path:
     * SMPS 1.8 V supplies LDO. Direct-SMPS-only is capped at 400 MHz.
     */
    PWR_CR3 =
        (PWR_CR3 & ~((3u << 4) | 1u)) | PWR_CR3_SMPSLEVEL_1V8 | PWR_CR3_SMPSEN | PWR_CR3_LDOEN;
    if (!wait_set(&PWR_CSR1, PWR_CSR1_ACTVOSRDY, 2000000u)) {
        return ERR_TIMEOUT;
    }

    /*
     * H74x: VOS[1:0]=11 is scale 1 (400 MHz). Scale 0 (480 MHz) is that
     * encoding plus SYSCFG_PWRCR.ODEN.
     */
    PWR_D3CR |= PWR_D3CR_VOS;
    (void)PWR_D3CR;
    if (!wait_set(&PWR_CSR1, PWR_CSR1_ACTVOSRDY, 2000000u)) {
        return ERR_TIMEOUT;
    }

    SYSCFG_PWRCR |= SYSCFG_PWRCR_ODEN;
    (void)SYSCFG_PWRCR;
    if (!wait_set(&PWR_CSR1, PWR_CSR1_ACTVOSRDY, 2000000u)) {
        return ERR_TIMEOUT;
    }
    if (!wait_set(&PWR_D3CR, PWR_D3CR_VOSRDY, 2000000u)) {
        return ERR_TIMEOUT;
    }
    return ERR_OK;
}

static err_t start_hse(void)
{
    RCC_CR |= RCC_CR_HSEON;
    if (!wait_set(&RCC_CR, RCC_CR_HSERDY, 2000000u)) {
        RCC_CR &= ~RCC_CR_HSEON;
        return ERR_TIMEOUT;
    }
    return ERR_OK;
}

static err_t start_pll1(void)
{
    RCC_CR &= ~RCC_CR_PLL1ON;
    {
        uint32_t spins = 2000000u;
        while ((RCC_CR & RCC_CR_PLL1RDY) != 0u) {
            if (spins == 0u) {
                return ERR_TIMEOUT;
            }
            spins--;
        }
    }

    /* HSE 25 MHz / M=5 * N=192 / P=2 = 480 MHz. VCI range 4–8 MHz, wide VCO. */
    RCC_PLLCKSELR = (2u << 0) | (5u << 4);
    RCC_PLLCFGR = (2u << 2) | (1u << 16) | (1u << 17) | (1u << 18);
    RCC_PLL1DIVR = (191u << 0) | (1u << 9) | (3u << 16) | (1u << 24);

    RCC_CR |= RCC_CR_PLL1ON;
    if (!wait_set(&RCC_CR, RCC_CR_PLL1RDY, 2000000u)) {
        return ERR_TIMEOUT;
    }
    return ERR_OK;
}

static err_t switch_to_pll(void)
{
    /* CPU /1, AHB /2 (240 MHz), APB3/1/2/4 /2 (120 MHz). */
    RCC_D1CFGR = (8u << 0) | (4u << 4);
    RCC_D2CFGR = (4u << 4) | (4u << 8);
    RCC_D3CFGR = (4u << 4);

    RCC_CFGR = (RCC_CFGR & ~7u) | RCC_CFGR_SW_PLL1;
    {
        uint32_t spins = 2000000u;
        while (((RCC_CFGR & RCC_CFGR_SWS_MSK) >> RCC_CFGR_SWS_SHIFT) != RCC_CFGR_SW_PLL1) {
            if (spins == 0u) {
                return ERR_TIMEOUT;
            }
            spins--;
        }
    }
    return ERR_OK;
}

err_t board_clock_init(void)
{
    err_t e;

    RCC_CR |= RCC_CR_HSION;

    /* Rev Y AXI SRAM issuing capability. */
    if ((DBGMCU_IDCODE & 0xFFFF0000u) < 0x20000000u) {
        *(volatile uint32_t *)0x51008108u = 1u;
    }

    fmc_disable_bank1();

    e = pwr_vos0();
    if (e != ERR_OK) {
        return e;
    }

    FLASH_ACR = FLASH_ACR_LATENCY_4 | FLASH_ACR_WRHIGHFREQ;

    e = start_hse();
    if (e != ERR_OK) {
        return e;
    }
    e = start_pll1();
    if (e != ERR_OK) {
        return e;
    }
    e = switch_to_pll();
    if (e != ERR_OK) {
        return e;
    }

    g_sysclk_hz = BOARD_SYSCLK_HZ;
    g_pclk1_hz = BOARD_PCLK1_HZ;
    return ERR_OK;
}
