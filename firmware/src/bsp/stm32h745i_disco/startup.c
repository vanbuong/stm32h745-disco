#include <stdint.h>

#include "stm32h7xx.h"
#include "system_stm32h7xx.h"

extern uint32_t _sidata;
extern uint32_t _sdata;
extern uint32_t _edata;
extern uint32_t _sbss;
extern uint32_t _ebss;
extern uint32_t _estack;

int main(void);

void Reset_Handler(void);
void Reset_Startup(void);
void Default_Handler(void);
void NMI_Handler(void) __attribute__((weak, alias("Default_Handler")));
void HardFault_Handler(void) __attribute__((weak, alias("Default_Handler")));
void MemManage_Handler(void) __attribute__((weak, alias("Default_Handler")));
void BusFault_Handler(void) __attribute__((weak, alias("Default_Handler")));
void UsageFault_Handler(void) __attribute__((weak, alias("Default_Handler")));
void SVC_Handler(void) __attribute__((weak, alias("Default_Handler")));
void DebugMon_Handler(void) __attribute__((weak, alias("Default_Handler")));
void PendSV_Handler(void) __attribute__((weak, alias("Default_Handler")));
void SysTick_Handler(void) __attribute__((weak, alias("Default_Handler")));

/*
 * Hardware loads SP from the vector table (D2 for CM4). Clock D2 SRAM
 * before any C prologue so a pin reset cannot bus-lock the interconnect
 * the way a debugger-ordered CM4-then-CM7 start avoids.
 */
__attribute__((naked, noreturn)) void Reset_Handler(void)
{
    __asm volatile("ldr r0, =0x580244DC\n"
                   "ldr r1, [r0]\n"
                   "orr r1, r1, #0xE0000000\n"
                   "str r1, [r0]\n"
                   "dsb\n"
                   "ldr r1, [r0]\n"
                   "ldr r0, =_estack\n"
                   "msr msp, r0\n"
                   "b Reset_Startup\n");
}

__attribute__((used)) void Reset_Startup(void)
{
    uint32_t *src;
    uint32_t *dst;

#if defined(CORE_CM7)
    uint32_t n;

    PWR->CPUCR &= ~(PWR_CPUCR_PDDS_D2 | PWR_CPUCR_PDDS_D3);
    PWR->CPUCR |= PWR_CPUCR_RUN_D3;
    RCC->GCR |= RCC_GCR_BOOT_C2;
    /* DIRECT_SMPS; do not wait forever — that looks like a dead reset button. */
    PWR->CR3 &= ~PWR_CR3_LDOEN;
    n = 1000000u;
    while (((PWR->CSR1 & PWR_CSR1_ACTVOSRDY) == 0U) && (n > 0u)) {
        n--;
    }
#else
    RCC->GCR |= RCC_GCR_BOOT_C1;
#endif
    SystemInit();

    src = &_sidata;
    dst = &_sdata;
    while (dst < &_edata) {
        *dst++ = *src++;
    }
    dst = &_sbss;
    while (dst < &_ebss) {
        *dst++ = 0;
    }
    (void)main();
    for (;;) {
    }
}

void Default_Handler(void)
{
    for (;;) {
    }
}

__attribute__((section(".isr_vector"), used)) void (*const g_vectors[])(void) = {
    (void (*)(void))(&_estack),
    Reset_Handler,
    NMI_Handler,
    HardFault_Handler,
    MemManage_Handler,
    BusFault_Handler,
    UsageFault_Handler,
    0,
    0,
    0,
    0,
    SVC_Handler,
    DebugMon_Handler,
    0,
    PendSV_Handler,
    SysTick_Handler,
};
