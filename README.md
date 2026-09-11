# stm32h745-disco

HMI firmware for the **STM32H745I-DISCO**: dual-core shell with file explorer, image and text viewers, audio, **game**, and **home automation** on the 4.3" 480×272 panel.

The first implementation is **FreeRTOS + LVGL + STM32Cube**. Interfaces are written so the same apps can move to **Zephyr + LVGL** later.

## Documentation

| Document | Purpose |
| --- | --- |
| [doc/Design_Review.md](doc/Design_Review.md) | Review of the v1 drafts and why v2 changed |
| [doc/Architecture.md](doc/Architecture.md) | Layers, core split, memory map, portable APIs |
| [doc/Plan.md](doc/Plan.md) | Sprints, GUI decision, risks |
| [doc/UI_Design.md](doc/UI_Design.md) | Shell, screens, theme, touch rules |
| [doc/Requirements_and_Test_Cases.md](doc/Requirements_and_Test_Cases.md) | Shall statements, tests, traceability |

## Hardware (used by the design)

- STM32H745XIH6 — Cortex-M7 480 MHz + Cortex-M4 240 MHz
- 4.3" 480×272 RGB panel, FT5336 touch
- 8 MB usable SDRAM at `0xD0000000` (16-bit FMC)
- Dual QSPI NOR (memory-mapped assets)
- 4 GB eMMC (user files)
- WM8994 audio, LAN8740A Ethernet
- Wi-Fi only if an ESP32 is added on Arduino / STMod+

## Design rules (short)

1. Apps do not call LVGL, FreeRTOS, FatFS, MQTT, or HAL.
2. M7 owns UI, VFS, image decode, game sim, and the home device model. M4 owns SAI audio (and optional net).
3. IPC is versioned messages in SRAM4, not shared C pointers.
4. Explorer is jailed to `/user`. Home UI talks only to `home_*`. Game logic talks only to `game_module_t` / `gfx_*`.

## Status

Documentation v2. Firmware bring-up starts at Sprint 0 in `doc/Plan.md`.

## License

MIT — see [LICENSE](LICENSE).
