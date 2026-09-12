#include "bsp/board.h"
#include "bsp/mpu_map.h"

#include "cube.h"

static volatile uint32_t g_mpu_faults;
static volatile uint32_t g_mpu_last_mmfar;

#define MPU_SELFTEST_BASE 0x2407FFE0u

static void mpu_fill(MPU_Region_InitTypeDef *r, uint8_t number, uint32_t base, uint8_t size_enc,
                     uint8_t attr, uint8_t exec, uint8_t ap)
{
    r->Enable = MPU_REGION_ENABLE;
    r->Number = number;
    r->BaseAddress = base;
    r->Size = size_enc;
    r->SubRegionDisable = 0;
    r->AccessPermission = ap;
    r->DisableExec = (exec != 0u) ? MPU_INSTRUCTION_ACCESS_ENABLE : MPU_INSTRUCTION_ACCESS_DISABLE;
    r->IsShareable = MPU_ACCESS_SHAREABLE;
    r->TypeExtField = MPU_TEX_LEVEL0;
    r->IsCacheable = MPU_ACCESS_NOT_CACHEABLE;
    r->IsBufferable = MPU_ACCESS_NOT_BUFFERABLE;

    switch (attr) {
    case MPU_ATTR_DEVICE:
        r->IsBufferable = MPU_ACCESS_BUFFERABLE;
        break;
    case MPU_ATTR_NORMAL_NC:
        r->TypeExtField = MPU_TEX_LEVEL1;
        break;
    case MPU_ATTR_WT:
        r->IsCacheable = MPU_ACCESS_CACHEABLE;
        break;
    case MPU_ATTR_WB:
        r->TypeExtField = MPU_TEX_LEVEL1;
        r->IsCacheable = MPU_ACCESS_CACHEABLE;
        r->IsBufferable = MPU_ACCESS_BUFFERABLE;
        break;
    default:
        break;
    }
}

void board_mpu_init(void)
{
    MPU_Region_InitTypeDef r = {0};
    unsigned i;

    HAL_MPU_Disable();
    for (i = 0; i < g_mpu_map_n; i++) {
        const mpu_region_desc_t *d = &g_mpu_map[i];
        mpu_fill(&r, (uint8_t)i, d->base, d->size_enc, d->attr, d->exec, MPU_REGION_FULL_ACCESS);
        HAL_MPU_ConfigRegion(&r);
    }
    SCB->SHCSR |= SCB_SHCSR_MEMFAULTENA_Msk;
    HAL_MPU_Enable(MPU_PRIVILEGED_DEFAULT);
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
    g_mpu_last_mmfar = SCB->MMFAR;
    SCB->CFSR = SCB->CFSR;

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
    MPU_Region_InitTypeDef r = {0};
    uint32_t before = g_mpu_faults;
    volatile uint32_t *p = (volatile uint32_t *)MPU_SELFTEST_BASE;

    /* A live probe writes a NO_ACCESS region and expects MemManage. ST-Link
     * breaks on that fault; skip the store when a debugger is attached. */
    if ((CoreDebug->DHCSR & CoreDebug_DHCSR_C_DEBUGEN_Msk) != 0u) {
        return ERR_OK;
    }

    mpu_fill(&r, MPU_REGION_NUMBER7, MPU_SELFTEST_BASE, MPU_ENC_32B, MPU_ATTR_NORMAL_NC, 0,
             MPU_REGION_NO_ACCESS);
    HAL_MPU_ConfigRegion(&r);

    *p = 0xA5A5A5A5u;
    __DSB();

    HAL_MPU_DisableRegion(MPU_REGION_NUMBER7);

    if (g_mpu_faults == before) {
        return ERR_IO;
    }
    return ERR_OK;
}
