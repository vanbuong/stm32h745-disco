# STM32H745I-DISCO board map

What this firmware actually muxes and clocks. Policy (who owns ETH, I2C4, the vector table) stays in `Architecture.md`. Solder-bridge defaults stay in `firmware/src/bsp/stm32h745i_disco/README.md`.

Update this file when the listed BSP sources change. Do not treat a CubeMX `.ioc` as the source of truth.

Board: MB1381, STM32H745XIH6. Analog out is **CN10 3.5 mm headphone**.

## 1. Peripherals by core

| Block | Core | Status | Source |
| --- | --- | --- | --- |
| RCC / PLL1 / PWR SMPS | M7 | In use | `clock.c` |
| HSEM 0 / 1 | both | In use | `hsem.c` |
| SysTick | both | 1 ms tick | `cube_it.c`, `main_m4.c` |
| USART3 | M7 | Console TX 115200 8N1 | `console.c` |
| FMC SDRAM bank 2 | M7 | 16 MB chip, 8 MB mapped, `0xD0000000` | `sdram.c` |
| QUADSPI | M7 | Bank 1 mmap smoke; BK2 pins held | `qspi.c` |
| LTDC layer 0 RGB565 | M7 | Polled VBR reload | `lcd.c` |
| DMA2D | M7 | Polled fill/copy | `lcd.c` |
| I2C4 | M7 | FT5336 or GT911 + WM8994 | `i2c4.c` |
| SDMMC1 | M7 | 8-bit eMMC, IDMA | `emmc.c` |
| ETH1 MAC + LAN8742 | M7 | MII 100 full, no CRS/COL | `eth.c` |
| RTC | M7 | LSE then LSI | `rtc.c` |
| SAI2_A | M4 | I2S master TX + MCLK | `sai_out.c` |
| IWDG1 | M7 start, both kick | After VFS bring-up | `wdog.c` |
| USART1 | — | Reserved for TI ZNP; `uart_*` returns `ERR_IO` | `uart.c` |
| USB OTG FS | — | Later TinyUSB MSC | — |
| JPEG / MDMA | — | Not started | — |
| TIM PWM backlight | — | Brightness is LTDC constant alpha | `lcd.c` |
| WWDG | — | Needs IRQ | — |

Vector table is the **16 Cortex-M exceptions only**. DMA, LTDC, SDMMC, ETH, SAI, and I2C stay polled. Do not enable those NVIC lines.

## 2. Clocks

Datasheet 480 / 240 MHz is not this tree. `BOARD_SYSCLK_HZ` is **400 MHz**.

HSE is 25 MHz. If HSE fails, PLL1 falls back to HSI 64 MHz with the same 400 MHz SYSCLK.

```
HSE 25 MHz
  ├─ PLL1  M=5 N=160  VCO 800 MHz
  │    P/2 → SYSCLK 400 MHz
  │    HCLK = SYSCLK/2 = 200 MHz   (AHB)
  │    APB1/2/3/4 = HCLK/2 = 100 MHz
  │    Q/4 → PLL1Q 200 MHz → SDMMC1
  │    M4 SYSCLK = HCLK 200 MHz
  ├─ PLL2  M=25 N=429 P=38 → SAI2/3 kernel ≈ 11.289 MHz (44.1 k family)
  └─ PLL3  M=5 N=160 R=83 → LTDC pixel ≈ 9.64 MHz
HSI 64 MHz fallback: PLL1 M=4 N=50; PLL3 M=16 N=160 R=66 ≈ 9.70 MHz
LSE 32.768 kHz → RTC (LSI if LSE fails)
LSI ~32 kHz → IWDG1
```

| Consumer | Source | Value |
| --- | --- | --- |
| M7 SYSCLK / M4 | PLL1P / HCLK | 400 / 200 MHz |
| FMC SDCLK | HCLK / 2 | 100 MHz |
| USART3 | PCLK1 | 100 MHz, 115200 8N1, TX only |
| I2C4 | D3PCLK1 | 100 MHz kernel, 100 kHz bus (`0x10B017DB`) |
| SDMMC1 | PLL1Q / (2 × ClockDiv) | ClockDiv 8 → 12.5 MHz; fallback 16 → 6.25 MHz |
| QSPI | prescaler 3 | kernel / 4 (typically 50 MHz from HCLK) |
| LTDC | PLL3R | ~9.6 MHz |
| SAI2_A | PLL2P | ~11.289 MHz; 16-bit in 32-bit Philips I2S slots, default 44.1 kHz |
| IWDG1 | LSI / 256, reload 2047 | ~16 s, window off |
| RTC | LSE or LSI | async 127, sync 255 |

## 3. DMA and copy engines

Not “how many DMA,” but **which engine**. DMA1 streams are free.

| Engine | Channel / instance | Dir | Owner | IRQ | Buffer |
| --- | --- | --- | --- | --- | --- |
| DMA2 Stream1 | `DMA_REQUEST_SAI2_A` | M2P circular | M4 SAI2_A | Off (poll HT/TC) | 2 × 512 frames stereo, 32-byte aligned. FIFO **off**. |
| SDMMC1 IDMA | internal | both | M7 eMMC | Off (poll DATAEND) | 32 KB AXI `.dma_buf` bounce |
| ETH1 DMA | dedicated MAC DMA | both | M7 | Off (poll) | 4 Tx + 4 Rx descriptors, SRAM3 `.eth_dma` |
| DMA2D | Chrom-ART | R2M / M2M | M7 `disp_fill` / blit | Off (poll) | SDRAM framebuffers |
| MDMA | — | — | unused | — | — |
| USART DMA | — | — | unused | — | — |

M7 D-cache: clean/invalidate the eMMC bounce and ETH SRAM3 is MPU non-cacheable. SAI PCM lives on M4 (no M7 D-cache).

## 4. Pin map

AF and GPIO as coded. RGB bit numbers for LTDC are not listed; the port masks in `lcd.c` are the contract.

### 4.1 Always-on / bring-up

| Signal | Pin | Mode | Notes |
| --- | --- | --- | --- |
| USART3_TX | PB10 AF7 | console | ST-LINK VCP |
| USART3_RX | PB11 AF7 | muxed, unused | TX-only driver |
| M7 LED LD2 | PI13 | GPIO out | Not LTDC VSYNC |
| M4 LED | PJ2 | GPIO out | LTDC must not take PJ2 |
| HSEM | — | — | Sem 0 M7→M4, sem 1 M4→M7 |

### 4.2 I2C4 (shared)

| Signal | Pin | Mode | Device |
| --- | --- | --- | --- |
| I2C4_SCL | PD12 AF4 OD | 100 kHz | FT5336 `0x70` or GT911 `0xBA`/`0x28`, WM8994 `0x34` |
| I2C4_SDA | PD13 AF4 OD | | BSP mutex; no I2C from ISR |
| TS_INT | PG2 | input PU | Polled. Held low during GT911 address latch |

### 4.3 Display

| Signal | Pin | Mode | Notes |
| --- | --- | --- | --- |
| LTDC RGB + sync | PI0, PI1, PI9, PI12, PI14, PI15; PJ except PJ2; PK1–PK6; PH1, PH9 | AF14 | PI9 VSYNC, PI12 HSYNC, PI14 CLK, PI15 DE on this board |
| LCD_DISP / RST | PA2 | GPIO then released | GT911 latches 0x5D while INT is low as PA2 rises |
| LCD enable | PD7, PK0 | GPIO high | PK0 is AF-init then retaken as GPIO |
| Backlight | PK7 | GPIO high | Not PWM. Dim = LTDC layer alpha |

### 4.4 Audio (CN10)

| Signal | Pin | Mode |
| --- | --- | --- |
| SAI2_MCLK_A | PI4 AF10 | On; clocks WM8994 |
| SAI2_SCK_A | PI5 AF10 | |
| SAI2_SD_A | PI6 AF10 | |
| SAI2_FS_A | PI7 AF10 | Philips I2S, not TDM `SLOT_02` |

### 4.5 eMMC (SDMMC1, 8-bit)

| Signal | Pin |
| --- | --- |
| D0–D3 | PC8–PC11 AF12 |
| D4 D5 | PB8 PB9 AF12 |
| D6 D7 | PC6 PC7 AF12 |
| CK | PC12 AF12 |
| CMD | PD2 AF12 |

### 4.6 QSPI NOR

| Signal | Pin | AF |
| --- | --- | --- |
| CLK | PF10 | AF9 |
| NCS | PG6 | AF10 |
| BK1 D0–D3 | PD11, PF9, PF7, PF6 | AF9/AF10 |
| BK2 D0–D3 | PH2, PH3, PG9, PG14 | AF9 |

Init still muxes bank 2 but uses `QSPI_FLASH_ID_1` / dual-flash off. Do not execute blank NOR (`0xFF`).

### 4.7 Ethernet MII (no CRS/COL)

| Signal | Pin | AF11 |
| --- | --- | --- |
| RX_CLK | PA1 | |
| MDIO | PA2 | Remuxed after LCD uses PA2 as reset |
| RX_DV | PA7 | |
| RXD0 RXD1 | PC4 PC5 | |
| RXD2 RXD3 | PB0 PB1 | |
| TX_CLK | PC3 | |
| TX_EN | PG11 | |
| TXD0 TXD1 | PG13 PG12 | |
| TXD2 TXD3 | PC2 PE2 | |
| MDC | PC1 | |

PH2/PH3 stay on QSPI (CRS/COL unused). UM2488 default: SB3/SB4 OFF, R38/R40 ON. 10 Mbit half-duplex is unsupported.

### 4.8 FMC SDRAM bank 2 (16-bit, AF12)

128 Mbit (**16 MB**) on the board. 16-bit FMC, 12 row × 8 col × 4 banks → **8 MB** at `0xD0000000`. MPU region and `memtest` walk that window only.

Port masks in `sdram.c`: PD{0,1,8,9,10,14,15}, PE{0,1,7–15}, PF{0–5,11–15}, PG{0,1,4,5,8,15}, PH{5,6,7}. CAS 3, SDCLK 100 MHz.

### 4.9 Reserved, not muxed

| Signal | Intended pin | Status |
| --- | --- | --- |
| USART1 ZNP | Arduino PB6/PB7 | `uart_open(UART_ID_ZNP)` → `ERR_IO` |
| USB OTG FS | micro-AB | Later TinyUSB exclusive MSC |
| Speaker | — | Board has none; headphone only |

## 5. Conflicts (do not “fix” without a new map)

| Pins | Owners | Rule |
| --- | --- | --- |
| PH2 PH3 | QSPI BK2 vs ETH CRS/COL | Keep QSPI. ETH is 100 full without those pins. |
| PA2 | LCD RST / GT911 latch vs ETH MDIO | LCD bring-up first; ETH remuxes MDIO. |
| PJ2 | M4 LED vs LTDC | `lcd.c` skips PJ2. |
| PI13 | M7 LED vs typical LTDC VSYNC | VSYNC is PI9. |
| I2C4 | Touch + codec | BSP lock only. |
| USART3 | Console | Never ZNP. |
| SAI vs ETH | M4 vs M7 | ETH only on M7. |

## 6. Memory the map implies

| Region | Use |
| --- | --- |
| SDRAM 16 MB chip / 8 MB mapped | `0xD0000000` |
| SDRAM `+0x000000` / `+0x040000` | LTDC FB0 / FB1 |
| AXI `0x24010000` 96 KB | LVGL heap |
| AXI `.dma_buf` | eMMC IDMA bounce |
| SRAM3 `0x30040000` | ETH DMA + LwIP heap (MPU NC) |
| SRAM4 `0x38000000` | IPC only |
| M4 SRAM1 | SAI PCM ping-pong |
