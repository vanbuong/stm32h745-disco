# stm32h745-disco

HMI firmware for the **STM32H745I-DISCO**: dual-core shell with file explorer, image and text viewers, audio, game, and **Zigbee home automation** (TI ZNP host) on the 4.3" 480×272 panel.

The first implementation is **FreeRTOS + LVGL + STM32Cube HAL/LL**. The board BSP is **ours** (`firmware/src/bsp`); ST HAL/LL and later chip drivers are git submodules. The full [`STM32CubeH7`](https://github.com/STMicroelectronics/STM32CubeH7) package is **not** in tree — see [third_party/README.md](third_party/README.md). Apps still never include those headers.

## Documentation

| Document | Purpose |
| --- | --- |
| [doc/Design_Review.md](doc/Design_Review.md) | Review of the v1 drafts and why v2 changed |
| [doc/Architecture.md](doc/Architecture.md) | Layers, core split, memory, boot/thread/Zigbee flows |
| [doc/Plan.md](doc/Plan.md) | Sprints, dependencies, risks |
| [doc/UI_Design.md](doc/UI_Design.md) | Shell, screens, navigation and pairing flows |
| [doc/Requirements_and_Test_Cases.md](doc/Requirements_and_Test_Cases.md) | Shall statements, tests, traceability, CI reqs |
| [doc/CICD.md](doc/CICD.md) | GitHub Actions, static analysis, coverage gates |
| [third_party/README.md](third_party/README.md) | What we vendor from ST vs write ourselves |

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

Cross-compile M7 / M4 (needs `gcc-arm-none-eabi` and the ST submodules):

```
git submodule update --init --recursive
cmake --preset Debug && cmake --build --preset Debug
```

That is the STM32 VS Code / Ninja path and produces `build/Debug/firmware-m7.elf` and `firmware-m4.elf`. CLI without Ninja:

```
cmake --preset m7-debug && cmake --build --preset m7-debug
cmake --preset m4-debug && cmake --build --preset m4-debug
```

ST HAL/LL v1.11.6, CMSIS device H7 v1.10.7, and CMSIS Core v5.9.0 live in `third_party/` (git submodules). Glue lives in `firmware/src/port/cube`.

## STM32 VS Code extension

Install the [STM32CubeIDE for Visual Studio Code](https://marketplace.visualstudio.com/items?itemName=STMicroelectronics.stm32-vscode-extension) pack (brings CMake Tools, Ninja, `arm-none-eabi-gcc` via CubeCLT). Then:

1. `git submodule update --init --recursive`
2. **File → Open Folder** on this repo (or open `stm32h745-disco.code-workspace`)
3. Accept **Configure discovered CMake project(s) as STM32Cube project(s)?** if asked
4. Select the **Debug (M7 + M4)** CMake preset
5. Build, then debug with **CM7_Debug** (flashes both ELFs) and optionally **DualCore_Debug**

Device is `STM32H745XIH6` on **STM32H745I-DISCO** (`.settings/ide.store.json`). Apps still never include HAL; only `firmware/src/port/cube` and `firmware/src/bsp` do.

CI: format, layering, cppcheck, clang-tidy, coverage, and both ELF images — see `doc/CICD.md`.

## Sprint 3 boot log (USART3 115200)

After memory, display, and eMMC bring-up the M7 mounts FAT on `/user`, lists it, sequential-reads `/user/s3.bin` (creates a 1 MB file if missing, then reads 8 MB by looping), then paints color bars and tracks a touch crosshair. LD2 (PI13) still blinks:

```
M7 stm32h745-disco s3
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
emmc ok
emmc_blocks <count>
vfs ok
vfs_dir ok
vfs_ent ...
vfs_ents N
vfs_mk ok
vfs_open ok
vfs_bytes 8388608
vfs_ms ...
vfs_kBps ...
vfs_mbps ok
disp ok
input ok
touch ok
bars ok
fps_ms 1000
fps 60
fps ok
```

`emmc fail` / `vfs fail` is a soft error (no crash); bars still run. `vfs_mbps ok` is ≥ 15 MB/s. Formal REQ-STG-02 uses a ≥ 64 MB file; the demo file is 1 MB looped so first boot stays short. `touch none` is OK on boards that ship GT911 instead of FT5336. `qspi_word FFFFFFFF` means the NOR is erased; mmap worked (no bus fault).

## License

MIT — see [LICENSE](LICENSE).
