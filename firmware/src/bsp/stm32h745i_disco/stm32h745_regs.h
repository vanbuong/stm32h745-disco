#ifndef STM32H745_REGS_H
#define STM32H745_REGS_H

#include <stdint.h>

#define RCC_BASE 0x58024400u
#define RCC_AHB4ENR (*(volatile uint32_t *)(RCC_BASE + 0xE0u))
#define RCC_APB1LENR (*(volatile uint32_t *)(RCC_BASE + 0xE8u))

#define GPIOB_BASE 0x58020400u
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

#define SCB_CPACR (*(volatile uint32_t *)0xE000ED88u)

#define RCC_AHB4ENR_GPIOBEN (1u << 1)
#define RCC_AHB4ENR_GPIOIEN (1u << 8)
#define RCC_AHB4ENR_GPIOJEN (1u << 9)
#define RCC_APB1LENR_USART3EN (1u << 18)

#define USART_CR1_UE (1u << 0)
#define USART_CR1_TE (1u << 3)
#define USART_ISR_TXE (1u << 7)

#endif /* STM32H745_REGS_H */
