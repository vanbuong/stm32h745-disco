# Third-party components

Do **not** add [STM32CubeH7](https://github.com/STMicroelectronics/STM32CubeH7) as a submodule. It is a meta-package (every H7 board BSP, demos, TouchGFX, USB, mbedTLS, ST forks of FreeRTOS/FatFS/LwIP). This repo vendors **only the pieces a sprint needs**, as git submodules.

Policy (locked in `doc/Architecture.md` §3.1):

1. **Our BSP** lives in `firmware/src/bsp/stm32h745i_disco/` and talks to hardware through `port/cube` (HAL/LL).
2. **ST chip drivers** (touch, panel timings, codec, NOR, PHY) are pulled from ST's *component* repos, not from `stm32h745i-disco-bsp`.
3. **Middleware** (LVGL, FreeRTOS, FatFS, LwIP, Helix) comes from upstream. Cube `Middlewares/` is not used unless an ST glue file is the only practical port.
4. Apps never include these trees. Format / cppcheck / coverage skip `third_party/`.

## In tree now (CubeH7 1.13.0 set)

| Path | Upstream | Tag |
| --- | --- | --- |
| `stm32h7xx-hal-driver/` | [stm32h7xx-hal-driver](https://github.com/STMicroelectronics/stm32h7xx-hal-driver) | v1.11.6 |
| `cmsis-device-h7/` | [cmsis-device-h7](https://github.com/STMicroelectronics/cmsis-device-h7) | v1.10.7 |
| `cmsis_core/` | [cmsis_core](https://github.com/STMicroelectronics/cmsis_core) | v5.9.0_20220705 |

`stm32h7xx-hal-driver` is both HAL (`stm32h7xx_hal_*.c`) and LL (`stm32h7xx_ll_*.c`, `USE_FULL_LL_DRIVER`).

```
git submodule update --init --recursive
```

| Core | HAL | LL |
| --- | --- | --- |
| M7 | RCC, PWR, Cortex/MPU, GPIO AF, SDRAM, QSPI | FMC, USART3, GPIO (LED) |
| M4 | none | GPIO (LED) |

## Pull when a sprint needs it

| Sprint | Submodule | Why |
| --- | --- | --- |
| 2 display | [stm32-ft5336](https://github.com/STMicroelectronics/stm32-ft5336), [stm32-rk043fn48h](https://github.com/STMicroelectronics/stm32-rk043fn48h) | Touch + RK043FN48H timings. LTDC/DMA2D stay in our BSP via HAL. |
| QSPI assets | [stm32-mt25tl01g](https://github.com/STMicroelectronics/stm32-mt25tl01g) | Quad/mmap commands beyond Sprint 1's 1-1-1 READ. |
| 3 VFS | FatFS (elm-chan or `fatfs`) | `diskio` for SDMMC1 is our code. |
| 4 shell | [lvgl](https://github.com/lvgl/lvgl) | Only `src/ui/backend_lvgl`. |
| OSAL | [FreeRTOS-Kernel](https://github.com/FreeRTOS/FreeRTOS-Kernel) | Not Cube `stm32-mw-freertos` unless we need their CMSIS-RTOS glue. |
| 8 audio | [stm32-wm8994](https://github.com/STMicroelectronics/stm32-wm8994), Helix | Codec + MP3. SAI DMA is our BSP. |
| 9 net | [stm32-lan8742](https://github.com/STMicroelectronics/stm32-lan8742), LwIP | PHY. `ethernetif` is our port. |

Reference only (clone locally, do not add): [stm32h745i-disco-bsp](https://github.com/STMicroelectronics/stm32h745i-disco-bsp) for pin maps and init sequences.

Do not add: other H7 board BSPs, TouchGFX, USB device/host, mbedTLS, LibJPEG (use the H7 JPEG codec), `stm32-mw-*` copies of stacks we already take upstream.
