#include "bsp/board.h"

#include "cube.h"

/* clang-format off */
#include "ff.h"
#include "diskio.h"
/* clang-format on */

#include <string.h>

/*
 * eMMC on SDMMC1, 8-bit (UM2488). Polling + IDMA, no SDMMC IRQ (vector table
 * is the Cortex-M 16). Bounce buffer lives in AXI SRAM; DTCM is not IDMA-capable.
 */

#define MMC_ALIGN_SEC 8u
#define MMC_BOUNCE_SEC 64u
#define MMC_TIMEOUT_MS 1000u

static MMC_HandleTypeDef g_mmc;
static uint8_t g_ready;
static uint32_t g_blocks;

static uint8_t g_bounce[MMC_BOUNCE_SEC * 512u] __attribute__((section(".dma_buf"), aligned(32)));

static uint32_t align_down8(uint32_t s)
{
    return s & ~7u;
}

static uint32_t align_up8(uint32_t s)
{
    return (s + 7u) & ~7u;
}

static err_t mmc_wait_ready(uint32_t ms)
{
    uint32_t t0 = HAL_GetTick();
    while (HAL_MMC_GetCardState(&g_mmc) != HAL_MMC_CARD_TRANSFER) {
        if ((HAL_GetTick() - t0) > ms) {
            return ERR_TIMEOUT;
        }
    }
    return ERR_OK;
}

static void bounce_invalidate(uint32_t nbytes)
{
    SCB_InvalidateDCache_by_Addr((uint32_t *)(uintptr_t)g_bounce, (int32_t)nbytes);
}

static void bounce_clean(uint32_t nbytes)
{
    SCB_CleanDCache_by_Addr((uint32_t *)(uintptr_t)g_bounce, (int32_t)nbytes);
}

void HAL_MMC_MspInit(MMC_HandleTypeDef *hmmc)
{
    GPIO_InitTypeDef g = {0};

    (void)hmmc;
    __HAL_RCC_GPIOB_CLK_ENABLE();
    __HAL_RCC_GPIOC_CLK_ENABLE();
    __HAL_RCC_GPIOD_CLK_ENABLE();
    __HAL_RCC_SDMMC1_CLK_ENABLE();

    g.Mode = GPIO_MODE_AF_PP;
    g.Pull = GPIO_PULLUP;
    g.Speed = GPIO_SPEED_FREQ_VERY_HIGH;
    g.Alternate = GPIO_AF12_SDMMC1;

    g.Pin = GPIO_PIN_8 | GPIO_PIN_9;
    HAL_GPIO_Init(GPIOB, &g);

    g.Pin =
        GPIO_PIN_6 | GPIO_PIN_7 | GPIO_PIN_8 | GPIO_PIN_9 | GPIO_PIN_10 | GPIO_PIN_11 | GPIO_PIN_12;
    HAL_GPIO_Init(GPIOC, &g);

    g.Pin = GPIO_PIN_2;
    HAL_GPIO_Init(GPIOD, &g);
}

err_t board_emmc_init(void)
{
    RCC_PeriphCLKInitTypeDef p = {0};
    HAL_MMC_CardInfoTypeDef info;

    p.PeriphClockSelection = RCC_PERIPHCLK_SDMMC;
    p.SdmmcClockSelection = RCC_SDMMCCLKSOURCE_PLL;
    if (HAL_RCCEx_PeriphCLKConfig(&p) != HAL_OK) {
        return ERR_IO;
    }

    g_mmc.Instance = SDMMC1;
    g_mmc.Init.ClockEdge = SDMMC_CLOCK_EDGE_RISING;
    g_mmc.Init.ClockPowerSave = SDMMC_CLOCK_POWER_SAVE_DISABLE;
    g_mmc.Init.BusWide = SDMMC_BUS_WIDE_8B;
    g_mmc.Init.HardwareFlowControl = SDMMC_HARDWARE_FLOW_CONTROL_DISABLE;
    /* PLL1Q = 200 MHz → SDMMC_CK = 200 / (2 * 4) = 25 MHz. */
    g_mmc.Init.ClockDiv = 4;
    if (HAL_MMC_Init(&g_mmc) != HAL_OK) {
        g_ready = 0u;
        return ERR_IO;
    }
    if (HAL_MMC_GetCardInfo(&g_mmc, &info) != HAL_OK) {
        g_ready = 0u;
        return ERR_IO;
    }
    g_blocks = info.LogBlockNbr;
    g_ready = 1u;
    return ERR_OK;
}

int board_emmc_ready(void)
{
    return g_ready ? 1 : 0;
}

uint32_t board_emmc_block_count(void)
{
    return g_blocks;
}

DSTATUS disk_initialize(BYTE pdrv)
{
    if (pdrv != 0u) {
        return STA_NOINIT;
    }
    return g_ready ? 0u : STA_NOINIT;
}

DSTATUS disk_status(BYTE pdrv)
{
    if (pdrv != 0u) {
        return STA_NOINIT;
    }
    return g_ready ? 0u : STA_NOINIT;
}

DRESULT disk_read(BYTE pdrv, BYTE *buff, LBA_t sector, UINT count)
{
    uint32_t al;
    uint32_t n;
    uint32_t off;
    uint32_t done = 0u;

    if (pdrv != 0u || buff == NULL || count == 0u || !g_ready) {
        return RES_PARERR;
    }

    while (done < count) {
        uint32_t s = (uint32_t)sector + done;
        uint32_t remain = (uint32_t)count - done;
        UINT chunk;

        al = align_down8(s);
        n = align_up8(s + remain) - al;
        if (n > MMC_BOUNCE_SEC) {
            n = MMC_BOUNCE_SEC;
        }
        SCB_CleanInvalidateDCache_by_Addr((uint32_t *)(uintptr_t)g_bounce, (int32_t)(n * 512u));
        if (HAL_MMC_ReadBlocks(&g_mmc, g_bounce, al, n, MMC_TIMEOUT_MS) != HAL_OK) {
            return RES_ERROR;
        }
        if (mmc_wait_ready(MMC_TIMEOUT_MS) != ERR_OK) {
            return RES_ERROR;
        }
        bounce_invalidate(n * 512u);
        off = (s - al) * 512u;
        chunk = remain;
        if ((off / 512u) + chunk > n) {
            chunk = n - (off / 512u);
        }
        memcpy(buff + (done * 512u), g_bounce + off, (size_t)chunk * 512u);
        done += chunk;
    }
    return RES_OK;
}

DRESULT disk_write(BYTE pdrv, const BYTE *buff, LBA_t sector, UINT count)
{
    uint32_t al;
    uint32_t n;
    uint32_t off;
    uint32_t done = 0u;

    if (pdrv != 0u || buff == NULL || count == 0u || !g_ready) {
        return RES_PARERR;
    }

    while (done < count) {
        uint32_t s = (uint32_t)sector + done;
        uint32_t remain = (uint32_t)count - done;
        UINT chunk;

        al = align_down8(s);
        n = align_up8(s + remain) - al;
        if (n > MMC_BOUNCE_SEC) {
            n = MMC_BOUNCE_SEC;
        }
        off = (s - al) * 512u;
        chunk = remain;
        if ((off / 512u) + chunk > n) {
            chunk = n - (off / 512u);
        }
        if (off != 0u || chunk != n) {
            SCB_CleanInvalidateDCache_by_Addr((uint32_t *)(uintptr_t)g_bounce, (int32_t)(n * 512u));
            if (HAL_MMC_ReadBlocks(&g_mmc, g_bounce, al, n, MMC_TIMEOUT_MS) != HAL_OK) {
                return RES_ERROR;
            }
            if (mmc_wait_ready(MMC_TIMEOUT_MS) != ERR_OK) {
                return RES_ERROR;
            }
            bounce_invalidate(n * 512u);
        }
        memcpy(g_bounce + off, buff + (done * 512u), (size_t)chunk * 512u);
        bounce_clean(n * 512u);
        if (HAL_MMC_WriteBlocks(&g_mmc, g_bounce, al, n, MMC_TIMEOUT_MS) != HAL_OK) {
            return RES_ERROR;
        }
        if (mmc_wait_ready(MMC_TIMEOUT_MS) != ERR_OK) {
            return RES_ERROR;
        }
        done += chunk;
    }
    return RES_OK;
}

DRESULT disk_ioctl(BYTE pdrv, BYTE cmd, void *buff)
{
    if (pdrv != 0u || !g_ready) {
        return RES_PARERR;
    }
    switch (cmd) {
    case CTRL_SYNC:
        return (mmc_wait_ready(MMC_TIMEOUT_MS) == ERR_OK) ? RES_OK : RES_ERROR;
    case GET_SECTOR_COUNT:
        if (buff == NULL) {
            return RES_PARERR;
        }
        *(LBA_t *)buff = (LBA_t)g_blocks;
        return RES_OK;
    case GET_SECTOR_SIZE:
        if (buff == NULL) {
            return RES_PARERR;
        }
        *(WORD *)buff = 512;
        return RES_OK;
    case GET_BLOCK_SIZE:
        if (buff == NULL) {
            return RES_PARERR;
        }
        *(DWORD *)buff = 8;
        return RES_OK;
    default:
        return RES_PARERR;
    }
}
