#include "bsp/board.h"
#include "bsp/mpu_map.h"

#include "stm32h745_regs.h"

static volatile uint32_t g_mpu_faults;
static volatile uint32_t g_mpu_last_mmfar;

#define MPU_SELFTEST_BASE 0x2407FFE0u

static uint32_t rasr_attr(uint8_t attr, uint8_t exec, uint8_t size_enc, uint8_t ap)
{
    uint32_t r = MPU_RASR_ENABLE | ((uint32_t)size_enc << 1) | ((uint32_t)ap << 24) | MPU_RASR_S;

    if (exec == 0u) {
        r |= MPU_RASR_XN;
    }
    switch (attr) {
    case MPU_ATTR_DEVICE:
        r |= (1u << 16); /* B, TEX=000 C=0 */
        break;
    case MPU_ATTR_NORMAL_NC:
        r |= (1u << 19); /* TEX=001 */
        break;
    case MPU_ATTR_WT:
        r |= (1u << 17); /* C */
        break;
    case MPU_ATTR_WB:
        r |= (1u << 19) | (1u << 17) | (1u << 16); /* TEX=001 C B */
        break;
    default:
        break;
    }
    return r;
}

static void mpu_program(uint8_t region, uint32_t base, uint32_t rasr)
{
    MPU_RNR = region;
    MPU_RBAR = base;
    MPU_RASR = rasr;
}

void board_mpu_init(void)
{
    unsigned i;

    MPU_CTRL = 0;
    for (i = 0; i < 16u; i++) {
        mpu_program((uint8_t)i, 0, 0);
    }
    for (i = 0; i < g_mpu_map_n; i++) {
        const mpu_region_desc_t *r = &g_mpu_map[i];
        mpu_program((uint8_t)i, r->base, rasr_attr(r->attr, r->exec, r->size_enc, 3u));
    }
    SCB_SHCSR |= SCB_SHCSR_MEMFAULTENA;
    MPU_CTRL = MPU_CTRL_ENABLE | MPU_CTRL_PRIVDEFENA;
    dsb();
    isb();
}

uint32_t board_mpu_faults(void)
{
    return g_mpu_faults;
}

uint32_t board_mpu_last_mmfar(void)
{
    return g_mpu_last_mmfar;
}

__attribute__((used)) static void memmanage_from_frame(uint32_t *frame)
{
    uint32_t pc = frame[6];
    uint16_t hw = *(const uint16_t *)(pc & ~1u);

    g_mpu_faults++;
    g_mpu_last_mmfar = SCB_MMFAR;
    SCB_CFSR = SCB_CFSR;

    if ((hw & 0xF800u) >= 0xE800u) {
        frame[6] = pc + 4u;
    } else {
        frame[6] = pc + 2u;
    }
}

void MemManage_Handler(void) __attribute__((naked));
void MemManage_Handler(void)
{
    __asm volatile(".syntax unified\n"
                   "tst lr, #4\n"
                   "ite eq\n"
                   "mrseq r0, msp\n"
                   "mrsne r0, psp\n"
                   "push {r4, lr}\n"
                   "bl memmanage_from_frame\n"
                   "pop {r4, pc}\n");
}

err_t board_mpu_selftest(void)
{
    uint32_t before = g_mpu_faults;
    volatile uint32_t *p = (volatile uint32_t *)MPU_SELFTEST_BASE;

    /* Region 7: 32-byte no-access hole at the end of AXI SRAM (unused; .data is DTCM). */
    mpu_program(7u, MPU_SELFTEST_BASE, rasr_attr(MPU_ATTR_NORMAL_NC, 0, MPU_ENC_32B, 0u));
    dsb();
    isb();

    *p = 0xA5A5A5A5u;
    dsb();

    mpu_program(7u, 0, 0);
    dsb();
    isb();

    if (g_mpu_faults == before) {
        return ERR_IO;
    }
    return ERR_OK;
}
