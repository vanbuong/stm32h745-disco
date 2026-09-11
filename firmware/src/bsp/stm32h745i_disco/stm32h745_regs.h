#ifndef STM32H745_REGS_H
#define STM32H745_REGS_H

#include <stdint.h>

#define RCC_BASE 0x58024400u
#define RCC_CR (*(volatile uint32_t *)(RCC_BASE + 0x00u))
#define RCC_CFGR (*(volatile uint32_t *)(RCC_BASE + 0x10u))
#define RCC_D1CFGR (*(volatile uint32_t *)(RCC_BASE + 0x18u))
#define RCC_D2CFGR (*(volatile uint32_t *)(RCC_BASE + 0x1Cu))
#define RCC_D3CFGR (*(volatile uint32_t *)(RCC_BASE + 0x20u))
#define RCC_PLLCKSELR (*(volatile uint32_t *)(RCC_BASE + 0x28u))
#define RCC_PLLCFGR (*(volatile uint32_t *)(RCC_BASE + 0x2Cu))
#define RCC_PLL1DIVR (*(volatile uint32_t *)(RCC_BASE + 0x30u))
#define RCC_AHB3RSTR (*(volatile uint32_t *)(RCC_BASE + 0x7Cu))
#define RCC_AHB3ENR (*(volatile uint32_t *)(RCC_BASE + 0xD4u))
#define RCC_AHB4ENR (*(volatile uint32_t *)(RCC_BASE + 0xE0u))
#define RCC_APB1LENR (*(volatile uint32_t *)(RCC_BASE + 0xE8u))
#define RCC_APB4ENR (*(volatile uint32_t *)(RCC_BASE + 0xF4u))

#define PWR_BASE 0x58024800u
#define PWR_CR3 (*(volatile uint32_t *)(PWR_BASE + 0x0Cu))
#define PWR_CSR1 (*(volatile uint32_t *)(PWR_BASE + 0x10u))
#define PWR_D3CR (*(volatile uint32_t *)(PWR_BASE + 0x18u))

#define SYSCFG_BASE 0x58000400u
#define SYSCFG_PWRCR (*(volatile uint32_t *)(SYSCFG_BASE + 0x04u))

#define FLASH_ACR (*(volatile uint32_t *)0x52002000u)
#define DBGMCU_IDCODE (*(volatile uint32_t *)0x5C001000u)

#define GPIOB_BASE 0x58020400u
#define GPIOD_BASE 0x58020C00u
#define GPIOE_BASE 0x58021000u
#define GPIOF_BASE 0x58021400u
#define GPIOG_BASE 0x58021800u
#define GPIOH_BASE 0x58021C00u
#define GPIOI_BASE 0x58022000u
#define GPIOJ_BASE 0x58022400u

#define GPIO_MODER(b) (*(volatile uint32_t *)((b) + 0x00u))
#define GPIO_OTYPER(b) (*(volatile uint32_t *)((b) + 0x04u))
#define GPIO_OSPEEDR(b) (*(volatile uint32_t *)((b) + 0x08u))
#define GPIO_PUPDR(b) (*(volatile uint32_t *)((b) + 0x0Cu))
#define GPIO_ODR(b) (*(volatile uint32_t *)((b) + 0x14u))
#define GPIO_BSRR(b) (*(volatile uint32_t *)((b) + 0x18u))
#define GPIO_AFR(b, n) (*(volatile uint32_t *)((b) + 0x20u + 4u * (n)))

#define USART3_BASE 0x40004800u
#define USART_CR1(b) (*(volatile uint32_t *)((b) + 0x00u))
#define USART_BRR(b) (*(volatile uint32_t *)((b) + 0x0Cu))
#define USART_ISR(b) (*(volatile uint32_t *)((b) + 0x1Cu))
#define USART_TDR(b) (*(volatile uint32_t *)((b) + 0x28u))

#define FMC_BANK1_BASE 0x52004000u
#define FMC_BTCR0 (*(volatile uint32_t *)(FMC_BANK1_BASE))
#define FMC_SDRAM_BASE 0x52004140u
#define FMC_SDCR1 (*(volatile uint32_t *)(FMC_SDRAM_BASE + 0x00u))
#define FMC_SDCR2 (*(volatile uint32_t *)(FMC_SDRAM_BASE + 0x04u))
#define FMC_SDTR1 (*(volatile uint32_t *)(FMC_SDRAM_BASE + 0x08u))
#define FMC_SDTR2 (*(volatile uint32_t *)(FMC_SDRAM_BASE + 0x0Cu))
#define FMC_SDCMR (*(volatile uint32_t *)(FMC_SDRAM_BASE + 0x10u))
#define FMC_SDRTR (*(volatile uint32_t *)(FMC_SDRAM_BASE + 0x14u))
#define FMC_SDSR (*(volatile uint32_t *)(FMC_SDRAM_BASE + 0x18u))

#define QUADSPI_BASE 0x52005000u
#define QUADSPI_CR (*(volatile uint32_t *)(QUADSPI_BASE + 0x00u))
#define QUADSPI_DCR (*(volatile uint32_t *)(QUADSPI_BASE + 0x04u))
#define QUADSPI_SR (*(volatile uint32_t *)(QUADSPI_BASE + 0x08u))
#define QUADSPI_FCR (*(volatile uint32_t *)(QUADSPI_BASE + 0x0Cu))
#define QUADSPI_DLR (*(volatile uint32_t *)(QUADSPI_BASE + 0x10u))
#define QUADSPI_CCR (*(volatile uint32_t *)(QUADSPI_BASE + 0x14u))
#define QUADSPI_AR (*(volatile uint32_t *)(QUADSPI_BASE + 0x18u))
#define QUADSPI_DR (*(volatile uint32_t *)(QUADSPI_BASE + 0x20u))
#define QUADSPI_DR8 (*(volatile uint8_t *)(QUADSPI_BASE + 0x20u))

#define SCB_CPACR (*(volatile uint32_t *)0xE000ED88u)
#define SCB_CCR (*(volatile uint32_t *)0xE000ED14u)
#define SCB_SHCSR (*(volatile uint32_t *)0xE000ED24u)
#define SCB_CFSR (*(volatile uint32_t *)0xE000ED28u)
#define SCB_MMFAR (*(volatile uint32_t *)0xE000ED34u)
#define SCB_CCSIDR (*(volatile uint32_t *)0xE000ED80u)
#define SCB_CSSELR (*(volatile uint32_t *)0xE000ED84u)
#define SCB_ICIALLU (*(volatile uint32_t *)0xE000EF50u)
#define SCB_DCISW (*(volatile uint32_t *)0xE000EF60u)
#define SCB_DCCISW (*(volatile uint32_t *)0xE000EF74u)

#define MPU_CTRL (*(volatile uint32_t *)0xE000ED94u)
#define MPU_RNR (*(volatile uint32_t *)0xE000ED98u)
#define MPU_RBAR (*(volatile uint32_t *)0xE000ED9Cu)
#define MPU_RASR (*(volatile uint32_t *)0xE000EDA0u)

#define RCC_CR_HSION (1u << 0)
#define RCC_CR_HSEON (1u << 16)
#define RCC_CR_HSERDY (1u << 17)
#define RCC_CR_PLL1ON (1u << 24)
#define RCC_CR_PLL1RDY (1u << 25)

#define RCC_CFGR_SW_PLL1 3u
#define RCC_CFGR_SWS_SHIFT 3u
#define RCC_CFGR_SWS_MSK (7u << 3)

#define RCC_AHB3ENR_FMCEN (1u << 12)
#define RCC_AHB3ENR_QSPIEN (1u << 14)
#define RCC_AHB3RSTR_QSPIRST (1u << 14)

#define RCC_AHB4ENR_GPIOBEN (1u << 1)
#define RCC_AHB4ENR_GPIODEN (1u << 3)
#define RCC_AHB4ENR_GPIOEEN (1u << 4)
#define RCC_AHB4ENR_GPIOFEN (1u << 5)
#define RCC_AHB4ENR_GPIOGEN (1u << 6)
#define RCC_AHB4ENR_GPIOHEN (1u << 7)
#define RCC_AHB4ENR_GPIOIEN (1u << 8)
#define RCC_AHB4ENR_GPIOJEN (1u << 9)

#define RCC_APB1LENR_USART3EN (1u << 18)
#define RCC_APB4ENR_SYSCFGEN (1u << 1)

#define PWR_CR3_LDOEN (1u << 1)
#define PWR_CR3_SMPSEN (1u << 2)
#define PWR_CR3_SMPSLEVEL_1V8 (1u << 4)
#define PWR_CSR1_ACTVOSRDY (1u << 13)
#define PWR_D3CR_VOSRDY (1u << 13)
#define PWR_D3CR_VOS (3u << 14)
#define SYSCFG_PWRCR_ODEN (1u << 0)

#define FLASH_ACR_LATENCY_4 4u
#define FLASH_ACR_WRHIGHFREQ (3u << 4)

#define USART_CR1_UE (1u << 0)
#define USART_CR1_TE (1u << 3)
#define USART_ISR_TXE (1u << 7)

#define QUADSPI_CR_EN (1u << 0)
#define QUADSPI_CR_ABORT (1u << 1)
#define QUADSPI_CR_SSHIFT (1u << 4)
#define QUADSPI_CR_DFM (1u << 6)
#define QUADSPI_CR_FSEL (1u << 7)
#define QUADSPI_SR_TCF (1u << 1)
#define QUADSPI_SR_BUSY (1u << 5)
#define QUADSPI_FCR_CTCF (1u << 1)

#define SCB_CCR_DC (1u << 16)
#define SCB_CCR_IC (1u << 17)
#define SCB_SHCSR_MEMFAULTENA (1u << 16)

#define MPU_CTRL_ENABLE (1u << 0)
#define MPU_CTRL_PRIVDEFENA (1u << 2)
#define MPU_RASR_ENABLE (1u << 0)
#define MPU_RASR_S (1u << 18)
#define MPU_RASR_XN (1u << 28)

#define FMC_SDSR_BUSY (1u << 5)
#define FMC_BCR1_FMCEN (1u << 31)

static inline void dsb(void)
{
    __asm volatile("dsb" ::: "memory");
}

static inline void isb(void)
{
    __asm volatile("isb" ::: "memory");
}

static inline void gpio_af(uint32_t base, unsigned pin, unsigned af, unsigned pupd)
{
    unsigned m = pin * 2u;
    unsigned afr = pin / 8u;
    unsigned s = (pin % 8u) * 4u;

    GPIO_MODER(base) = (GPIO_MODER(base) & ~(3u << m)) | (2u << m);
    GPIO_OSPEEDR(base) |= (3u << m);
    GPIO_OTYPER(base) &= ~(1u << pin);
    GPIO_PUPDR(base) = (GPIO_PUPDR(base) & ~(3u << m)) | ((pupd & 3u) << m);
    GPIO_AFR(base, afr) = (GPIO_AFR(base, afr) & ~(0xFu << s)) | ((af & 0xFu) << s);
}

#endif /* STM32H745_REGS_H */
