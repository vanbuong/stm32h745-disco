# Design Review (v1 → v2)

Review of the first plan, requirements, and UI documents. The v2 specs in this folder replace those drafts.

## What was already right

- Dual-core split (M7 for UI, M4 for real-time IO) matches the STM32H745.
- Shared SRAM4 (`0x38000000`) plus hardware semaphores is the correct on-chip IPC path.
- eMMC + FatFS for user media, QSPI for execute-in-place, SDRAM for framebuffers is the right storage split.
- Host unit tests plus on-target HIL tests is the right verification strategy for an MCU HMI.

## Gaps and corrections

| Topic | v1 issue | v2 change |
| --- | --- | --- |
| Portability | TouchGFX, FreeRTOS, FatFS, and HSEM ring buffers were treated as the product | Introduce OSAL, VFS, UI backend, display/input HAL, and an IPC protocol so LVGL and Zephyr can replace the first ports |
| UI framework | TouchGFX named as the UI | Prefer **LVGL from the first UI sprint**. TouchGFX is STM32-only and would be thrown away for Zephyr |
| Missing apps | File explorer, image viewer, and text viewer were not specified | First-class applications with requirements and screens |
| GUI chrome | Persistent 50 px left dock on a 480×272 panel | Full-width status bar + launcher + in-app top bar. Dock wasted ~10% of an already small canvas and missed finger-target size |
| Sprint plan | Sprints jumped 1 → 4 → 5 with no exit criteria | Sequential sprints with dependencies and done-when checks |
| SDRAM size | Documented as 16 MB | Chip is 128 Mbit (16 MB), but the board wires a **16-bit** FMC bus, so **8 MB is usable** |
| ESP32 Wi-Fi | Treated as on-board | Not present on STM32H745I-DISCO. Optional expansion via Arduino / STMod+ |
| Ethernet vs QSPI | Not mentioned | Default routing multiplexes ETH MII_CRS/COL with QSPI bank 2. Dual-flash + full Ethernet needs a pin/solder-bridge policy |
| Shared I2C | Not mentioned | I2C4 is shared by FT5336 touch and WM8994 codec. Needs a bus lock in the BSP |
| Cache / MPU | “D-Cache sync” only | Explicit non-cacheable IPC region, DMA buffer policy, and cache APIs in BSP |
| Requirements quality | Mixed goals, few IDs, no priority, no traceability | EARS-style shalls, MoSCoW, verification method, and a RTM |
| Tests | A handful of cases, some testing the wrong layer | Host tests for pure logic; HIL for timing, storage, display, audio, failover |
| Security / robustness | None | Path sandbox, bounded buffers, decoder failure, out-of-memory, and FS unmount |
| Game loop | Named without a product reason | Demoted to P2 optional app, same shell as other apps |

## GUI recommendation (adopted in `UI_Design.md`)

Do not keep a permanent left navigation rail on this panel.

Use a phone-like **shell**:

1. Always-on 32 px status bar.
2. Home launcher (icon grid) as the app switcher.
3. In-app 40 px top bar (Back, title, one action).
4. Optional 36 px now-playing bar only while audio is active.

That layout is framework-agnostic, maximizes the 480×240 content region, and maps cleanly to LVGL screens or a later Zephyr shell.

## Portability recommendation (adopted in `Architecture.md`)

| Layer | First implementation | Later replacement | App code sees |
| --- | --- | --- | --- |
| OS | FreeRTOS (or CMSIS-RTOS2) | Zephyr `k_*` | `osal_*` |
| UI | LVGL | LVGL on Zephyr (same widgets) | `ui_*` view-models + nav stack |
| Display / touch | STM32 LTDC + FT5336 BSP | Zephyr display + input DT | `disp_*` / `input_*` |
| FS | FatFS on eMMC | Zephyr FS / LittleFS | `vfs_*` |
| IPC | SRAM4 rings + HSEM | OpenAMP / RPMsg | `ipc_*` messages |
| Net | LwIP on one core | Zephyr net stack | `net_*` |
| Image decode | JPEG HW codec + SW PNG/BMP | Same HAL, Zephyr JPEG driver | `media_*` |

Start with LVGL, not TouchGFX. Switching OS later is cheaper than rewriting every screen.

## Hardware facts used in v2

- MCU: STM32H745XIH6, M7 480 MHz + M4 240 MHz, 2 MB flash, 1 MB SRAM.
- Panel: RK043FN48H, 4.3", 480×272, RGB, FT5336 capacitive multi-touch.
- SDRAM: MT48LC4M32B2 at `0xD0000000`, **8 MB usable**.
- QSPI: dual MT25TL01G, memory-mapped at `0x90000000` (~64 MB window).
- eMMC: 4 GB on SDMMC1.
- Audio: WM8994 on SAI + shared I2C4.
- Ethernet: LAN8740A MII; pin conflict with QSPI bank 2 in the default assembly.
- Debug: STLINK-V3E, USART3 VCP.
- Zephyr already supports `stm32h745i_disco` for both cores, LTDC, FT5336, HSEM mailbox, JPEG, SDMMC, QSPI, and FMC SDRAM.
