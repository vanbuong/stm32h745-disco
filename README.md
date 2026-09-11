# stm32h745-disco

HMI firmware for the **STM32H745I-DISCO**: dual-core shell with file explorer, image and text viewers, audio, game, and **Zigbee home automation** (TI ZNP host) on the 4.3" 480×272 panel.

The first implementation is **FreeRTOS + LVGL + STM32Cube**. Interfaces are written so the same apps can move to **Zephyr + LVGL** later.

## Documentation

| Document | Purpose |
| --- | --- |
| [doc/Design_Review.md](doc/Design_Review.md) | Review of the v1 drafts and why v2 changed |
| [doc/Architecture.md](doc/Architecture.md) | Layers, core split, memory, boot/thread/Zigbee flows |
| [doc/Plan.md](doc/Plan.md) | Sprints, dependencies, risks |
| [doc/UI_Design.md](doc/UI_Design.md) | Shell, screens, navigation and pairing flows |
| [doc/Requirements_and_Test_Cases.md](doc/Requirements_and_Test_Cases.md) | Shall statements, tests, traceability, CI reqs |
| [doc/CICD.md](doc/CICD.md) | GitHub Actions, static analysis, coverage gates |

## Hardware (used by the design)

- STM32H745XIH6 — Cortex-M7 480 MHz + Cortex-M4 240 MHz
- 4.3" 480×272 RGB panel, FT5336 touch
- 8 MB usable SDRAM at `0xD0000000` (16-bit FMC)
- Dual QSPI NOR (memory-mapped assets)
- 4 GB eMMC (user files)
- WM8994 audio, LAN8740A Ethernet
- Wi-Fi only if an ESP32 is added on Arduino / STMod+
- Zigbee via a **TI ZNP** module on USART1 (Arduino); STM32 is the host/coordinator

## Design rules (short)

1. Apps do not call LVGL, FreeRTOS, FatFS, MQTT, MT, or HAL.
2. M7 owns UI, VFS, image decode, game sim, Zigbee host, and local automations. M4 owns SAI audio (and optional net or ZNP UART).
3. IPC is versioned messages in SRAM4, not shared C pointers.
4. Explorer is jailed to `/user`. Home UI talks only to `home_*`. Game logic talks only to `game_module_t` / `gfx_*`.
5. TI ZNP is an expansion on USART1; USART3 stays the console.

# Build

Host tests (no board):

```
cmake --preset host-tests
cmake --build --preset host-tests
ctest --test-dir build-host --output-on-failure
```

Cross-compile M7 / M4 (needs `gcc-arm-none-eabi`):

```
cmake --preset m7-debug && cmake --build --preset m7-debug
cmake --preset m4-debug && cmake --build --preset m4-debug
```

CI: format, layering, cppcheck, clang-tidy, coverage, and both ELF images — see `doc/CICD.md`.

## Sprint 1 boot log (USART3 115200)

After clocks, MPU, cache, SDRAM, and QSPI init the M7 prints on the ST-LINK VCP, then blinks LD2 (PI13):

```
M7 stm32h745-disco s1
clk ok
sysclk 1C9C3800
mpu on
cache on
sdram ok
walk ok
qspi ok
qspi id 20BA20
qspi_id ok
qspi_mmap ok
qspi_word FFFFFFFF
mpu_test ok
mpu_faults 00000001
mpu_mmfar 2407FFE0
```

`qspi_word FFFFFFFF` means the NOR is erased; mmap worked (no bus fault). Do not execute that window until assets are programmed.

## License

MIT — see [LICENSE](LICENSE).
