# Third-party components

Do **not** add [STM32CubeH7](https://github.com/STMicroelectronics/STM32CubeH7) as a submodule. It is a meta-package (every H7 board BSP, demos, TouchGFX, USB, mbedTLS, ST forks of FreeRTOS/FatFS/LwIP). This repo vendors **only the pieces a sprint needs**, as git submodules.

Policy (locked in `doc/Architecture.md` §3.1):

1. **Our BSP** lives in `firmware/src/bsp/stm32h745i_disco/` and talks to hardware through `port/cube` (HAL/LL).
2. **ST chip drivers** (touch, panel timings, codec, NOR, PHY) are pulled from ST's *component* repos, not from `stm32h745i-disco-bsp`.
3. **Middleware** (LVGL, FatFS, LwIP, Helix, later TinyUSB) comes from upstream. There is **no FreeRTOS**. Cube `Middlewares/` is not used unless an ST glue file is the only practical port. Do **not** add littlefs.
4. Apps never include these trees. Format / cppcheck / coverage skip `third_party/`.

## In tree now (CubeH7 1.13.0 set)

| Path | Upstream | Tag |
| --- | --- | --- |
| `stm32h7xx-hal-driver/` | [stm32h7xx-hal-driver](https://github.com/STMicroelectronics/stm32h7xx-hal-driver) | v1.11.6 |
| `cmsis-device-h7/` | [cmsis-device-h7](https://github.com/STMicroelectronics/cmsis-device-h7) | v1.10.7 |
| `cmsis_core/` | [cmsis_core](https://github.com/STMicroelectronics/cmsis_core) | v5.9.0_20220705 |
| `stm32-rk043fn48h/` | [stm32-rk043fn48h](https://github.com/STMicroelectronics/stm32-rk043fn48h) | v1.0.3-2 |
| `stm32-ft5336/` | [stm32-ft5336](https://github.com/STMicroelectronics/stm32-ft5336) | v2.0.1-2 |
| `fatfs/` | [abbrev/fatfs](https://github.com/abbrev/fatfs) (ChaN FatFs) | R0.15b |
| `lvgl/` | [lvgl](https://github.com/lvgl/lvgl) | v9.5.0 |
| `unity/` | [ThrowTheSwitch/Unity](https://github.com/ThrowTheSwitch/Unity) | v2.6.1 |
| `helix/` | [ultraembedded/libhelix-mp3](https://github.com/ultraembedded/libhelix-mp3) | `0a0e067` (master) |
| `stm32-wm8994/` | [stm32-wm8994](https://github.com/STMicroelectronics/stm32-wm8994) | v3.0.3 |
| `stm32-lan8742/` | [stm32-lan8742](https://github.com/STMicroelectronics/stm32-lan8742) | v1.0.4 |
| `lwip/` | [lwip-tcpip/lwip](https://github.com/lwip-tcpip/lwip) | STABLE-2_2_1_RELEASE |
| `cmsis-svd/` | [STM32H7xx_DFP](https://github.com/Open-CMSIS-Pack/STM32H7xx_DFP) `CMSIS/SVD/` | STM32H745_CM7 / CM4 (debug only) |

`stm32h7xx-hal-driver` is both HAL (`stm32h7xx_hal_*.c`) and LL (`stm32h7xx_ll_*.c`, `USE_FULL_LL_DRIVER`). Panel timings are header-only; FT5336 is compiled in the M7 image with I2C4 in our BSP. FatFs `ff.c` is compiled on M7; `diskio` and `ffconf.h` are ours (`firmware/src/bsp/.../emmc.c`, `firmware/src/port/fatfs/`). LVGL is compiled on M7 and in the Sprint 5b `host-sim` SDL binary; `lv_conf.h` lives in `firmware/src/port/lvgl/` (MCU) and `firmware/src/port/lvgl_sim/` (PC). Apps never include `lvgl.h`. Helix is compiled on M4 and in host-tests/host-sim (C fallback in `firmware/src/svc/vendor/helix_generic_asm.h`). WM8994 is compiled on M7; SAI DMA ping-pong is our BSP on M4. LAN8742 + LwIP (`NO_SYS`, UDP/DHCP/ICMP, no TCP) are compiled on **M7 only**; `ethernetif` is our port. `cmsis-svd/` is debug-only (VS Code register view); it is not linked.

```
git submodule update --init --recursive
```

| Core | HAL | LL |
| --- | --- | --- |
| M7 | RCC, PWR, Cortex/MPU, GPIO AF, SDRAM, QSPI, LTDC, DMA2D, I2C4, MMC/SDMMC1, ETH, RTC | FMC, USART3, GPIO (LED), SDMMC |
| M4 | RCC, DMA, SAI2 | GPIO (LED) |

## Pull when a sprint needs it

| Sprint | Submodule | Why |
| --- | --- | --- |
| QSPI assets | [stm32-mt25tl01g](https://github.com/STMicroelectronics/stm32-mt25tl01g) | Quad/mmap commands beyond Sprint 1's 1-1-1 READ. |
| 4 shell | [lvgl](https://github.com/lvgl/lvgl) | Done. Only `src/ui/backend_lvgl`. |
| 6 viewers | TJpgDec + puff | Done. Vendored under `firmware/src/svc/vendor/` (standalone ChaN TJpgDec, Mark Adler puff). Not LVGL's `libs/tjpgd`. LibJPEG stays out. |
| OSAL | — | Superloop. Do not add FreeRTOS-Kernel. Host tests use `osal/posix`. |
| 8 audio | [stm32-wm8994](https://github.com/STMicroelectronics/stm32-wm8994), Helix | Done. Codec + MP3. SAI DMA is our BSP. |
| 9 net | [stm32-lan8742](https://github.com/STMicroelectronics/stm32-lan8742), LwIP | Done. PHY + DHCP. `ethernetif` is our port. |
| USB MSC (later) | [tinyusb](https://github.com/hathach/tinyusb) | Device MSC over OTG FS. Exclusive with FatFs. Not Cube `USB_Device`. |

Reference only (clone locally, do not add): [stm32h745i-disco-bsp](https://github.com/STMicroelectronics/stm32h745i-disco-bsp) for pin maps and init sequences.

Do not add: other H7 board BSPs, TouchGFX, Cube USB device/host, **littlefs**, mbedTLS, LibJPEG (use the H7 JPEG codec), `stm32-mw-*` copies of stacks we already take upstream.
