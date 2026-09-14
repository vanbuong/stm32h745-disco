#include "bsp/board.h"

#include "cube.h"

/* clang-format off */
#include "ff.h"
#include "diskio.h"
/* clang-format on */

#include <string.h>

/*
 * eMMC on SDMMC1, 8-bit. Init matches Cube H7 V1.13.0
 * FatFs_Shared_Device MX_MMC_SD_Init: ClockDiv 8 (12.5 MHz @ PLL1Q
 * 200 MHz), rising edge, no HFC, no HS switch. That example's FatFs
 * path uses polling HAL_MMC_Read/WriteBlocks; we use SDMMC IDMA into a
 * 32-byte-aligned AXI bounce and poll DATAEND (vector table has no
 * SDMMC1 IRQ). 4 KB native pages need 8-sector aligned transfers.
 */

#define MMC_ALIGN_SEC 8u
#define MMC_BOUNCE_SEC 64u
#define MMC_TIMEOUT_MS 2000u
#define MMC_CLKDIV_CUBE 8u
#define MMC_CLKDIV_SLOW 16u

static MMC_HandleTypeDef g_mmc;
static uint8_t g_ready;
static uint32_t g_blocks;
static uint32_t g_last_err;

static uint8_t g_bounce[MMC_BOUNCE_SEC * 512u] __attribute__((section(".dma_buf"), aligned(32)));

static uint32_t align_down8(uint32_t s)
{
    return s & ~7u;
}

static uint32_t align_up8(uint32_t s)
{
    return (s + 7u) & ~7u;
}

static void bounce_inv(uint32_t bytes)
{
    SCB_InvalidateDCache_by_Addr((uint32_t *)(uintptr_t)g_bounce, (int32_t)bytes);
}

static void bounce_clean_inv(uint32_t bytes)
{
    SCB_CleanInvalidateDCache_by_Addr((uint32_t *)(uintptr_t)g_bounce, (int32_t)bytes);
}

static void bounce_clean(uint32_t bytes)
{
    SCB_CleanDCache_by_Addr((uint32_t *)(uintptr_t)g_bounce, (int32_t)bytes);
}

static err_t mmc_wait_ready(uint32_t ms)
{
    uint32_t t0 = HAL_GetTick();
    while (HAL_MMC_GetCardState(&g_mmc) != HAL_MMC_CARD_TRANSFER) {
        if ((HAL_GetTick() - t0) > ms) {
            g_last_err = HAL_MMC_GetError(&g_mmc);
            return ERR_TIMEOUT;
        }
    }
    return ERR_OK;
}

static err_t mmc_wait_idma(uint32_t ms)
{
    uint32_t t0 = HAL_GetTick();
    uint32_t bad =
        SDMMC_FLAG_DCRCFAIL | SDMMC_FLAG_DTIMEOUT | SDMMC_FLAG_RXOVERR | SDMMC_FLAG_TXUNDERR;

    while (!__HAL_MMC_GET_FLAG(&g_mmc, SDMMC_FLAG_DATAEND | bad)) {
        if ((HAL_GetTick() - t0) > ms) {
            g_last_err = HAL_MMC_ERROR_TIMEOUT;
            (void)HAL_MMC_Abort(&g_mmc);
            g_mmc.State = HAL_MMC_STATE_READY;
            return ERR_TIMEOUT;
        }
    }
    HAL_MMC_IRQHandler(&g_mmc);
    g_last_err = HAL_MMC_GetError(&g_mmc);
    g_mmc.State = HAL_MMC_STATE_READY;
    if (g_last_err != HAL_MMC_ERROR_NONE) {
        return ERR_IO;
    }
    return ERR_OK;
}

static err_t mmc_read_idma_once(uint32_t lba, uint32_t n)
{
    uint32_t bytes = n * 512u;

    bounce_clean_inv(bytes);
    if (HAL_MMC_ReadBlocks_DMA(&g_mmc, g_bounce, lba, n) != HAL_OK) {
        g_last_err = HAL_MMC_GetError(&g_mmc);
        g_mmc.State = HAL_MMC_STATE_READY;
        return ERR_IO;
    }
    if (mmc_wait_idma(MMC_TIMEOUT_MS) != ERR_OK) {
        return ERR_IO;
    }
    bounce_inv(bytes);
    return mmc_wait_ready(MMC_TIMEOUT_MS);
}

static err_t mmc_read_blocks(uint32_t lba, uint32_t n)
{
    err_t e = mmc_read_idma_once(lba, n);
    if (e == ERR_OK) {
        return ERR_OK;
    }
    (void)HAL_MMC_Abort(&g_mmc);
    g_mmc.State = HAL_MMC_STATE_READY;
    (void)mmc_wait_ready(MMC_TIMEOUT_MS);
    return mmc_read_idma_once(lba, n);
}

static err_t mmc_write_idma_once(uint32_t lba, uint32_t n)
{
    uint32_t bytes = n * 512u;

    bounce_clean(bytes);
    if (HAL_MMC_WriteBlocks_DMA(&g_mmc, g_bounce, lba, n) != HAL_OK) {
        g_last_err = HAL_MMC_GetError(&g_mmc);
        g_mmc.State = HAL_MMC_STATE_READY;
        return ERR_IO;
    }
    if (mmc_wait_idma(MMC_TIMEOUT_MS) != ERR_OK) {
        return ERR_IO;
    }
    return mmc_wait_ready(MMC_TIMEOUT_MS);
}

static err_t mmc_write_blocks(uint32_t lba, uint32_t n)
{
    err_t e = mmc_write_idma_once(lba, n);
    if (e == ERR_OK) {
        return ERR_OK;
    }
    (void)HAL_MMC_Abort(&g_mmc);
    g_mmc.State = HAL_MMC_STATE_READY;
    (void)mmc_wait_ready(MMC_TIMEOUT_MS);
    return mmc_write_idma_once(lba, n);
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

static void mmc_fill_init(uint32_t div)
{
    g_mmc.Instance = SDMMC1;
    g_mmc.Init.ClockEdge = SDMMC_CLOCK_EDGE_RISING;
    g_mmc.Init.ClockPowerSave = SDMMC_CLOCK_POWER_SAVE_DISABLE;
    g_mmc.Init.BusWide = SDMMC_BUS_WIDE_8B;
    g_mmc.Init.HardwareFlowControl = SDMMC_HARDWARE_FLOW_CONTROL_DISABLE;
    HAL_NVIC_DisableIRQ(SDMMC1_IRQn);
    g_mmc.Init.ClockDiv = div;
}

static err_t mmc_probe_read(void)
{
    return mmc_read_blocks(0u, MMC_ALIGN_SEC);
}

static void mmc_set_div(uint32_t div)
{
    uint32_t clkcr;

    g_mmc.Init.ClockDiv = div;
    clkcr = g_mmc.Instance->CLKCR;
    clkcr &= ~SDMMC_CLKCR_CLKDIV;
    clkcr |= (div & SDMMC_CLKCR_CLKDIV);
    g_mmc.Instance->CLKCR = clkcr;
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

    /* Cube MX_MMC_SD_Init: ClockDiv 8, no ConfigSpeedBusOperation. */
    mmc_fill_init(MMC_CLKDIV_CUBE);
    if (HAL_MMC_Init(&g_mmc) != HAL_OK) {
        g_ready = 0u;
        g_last_err = HAL_MMC_GetError(&g_mmc);
        return ERR_IO;
    }
    if (HAL_MMC_GetCardInfo(&g_mmc, &info) != HAL_OK) {
        g_ready = 0u;
        g_last_err = HAL_MMC_GetError(&g_mmc);
        return ERR_IO;
    }
    g_blocks = info.LogBlockNbr;
    if (mmc_probe_read() != ERR_OK) {
        (void)HAL_MMC_DeInit(&g_mmc);
        mmc_fill_init(MMC_CLKDIV_SLOW);
        if (HAL_MMC_Init(&g_mmc) != HAL_OK || mmc_probe_read() != ERR_OK) {
            g_ready = 0u;
            return ERR_IO;
        }
        if (HAL_MMC_GetCardInfo(&g_mmc, &info) == HAL_OK) {
            g_blocks = info.LogBlockNbr;
        }
    }
    g_ready = 1u;
    return ERR_OK;
}

err_t board_emmc_fallback(void)
{
    if (g_ready == 0u) {
        return ERR_IO;
    }
    mmc_set_div(MMC_CLKDIV_CUBE);
    (void)mmc_wait_ready(MMC_TIMEOUT_MS);
    return mmc_probe_read();
}

int board_emmc_ready(void)
{
    return g_ready ? 1 : 0;
}

uint32_t board_emmc_block_count(void)
{
    return g_blocks;
}

uint32_t board_emmc_last_error(void)
{
    return g_last_err;
}

uint32_t board_emmc_clock_hz(void)
{
    uint32_t ker;
    uint32_t div;

    if (g_mmc.Instance == NULL) {
        return 0u;
    }
    ker = HAL_RCCEx_GetPeriphCLKFreq(RCC_PERIPHCLK_SDMMC);
    if (ker == 0u) {
        ker = 200000000u;
    }
    div = g_mmc.Instance->CLKCR & SDMMC_CLKCR_CLKDIV;
    if (div == 0u) {
        return ker;
    }
    return ker / (2u * div);
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
        if (mmc_read_blocks(al, n) != ERR_OK) {
            return RES_ERROR;
        }
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
            if (mmc_read_blocks(al, n) != ERR_OK) {
                return RES_ERROR;
            }
        }
        memcpy(g_bounce + off, buff + (done * 512u), (size_t)chunk * 512u);
        if (mmc_write_blocks(al, n) != ERR_OK) {
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
