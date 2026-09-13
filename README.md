# stm32h745-disco

HMI firmware for the **STM32H745I-DISCO**: dual-core shell with file explorer, image and text viewers, audio, game, and **Zigbee home automation** (TI ZNP host) on the 4.3" 480×272 panel.

Both cores run a **superloop** (no FreeRTOS). UI is **LVGL** (`LV_OS_NONE`); LwIP is `NO_SYS`. The board BSP is **ours** (`firmware/src/bsp`); ST HAL/LL and later chip drivers are git submodules. The full [`STM32CubeH7`](https://github.com/STMicroelectronics/STM32CubeH7) package is **not** in tree — see [third_party/README.md](third_party/README.md). Apps still never include those headers.

## Documentation

| Document | Purpose |
| --- | --- |
| [doc/Design_Review.md](doc/Design_Review.md) | Review of the v1 drafts and why v2 changed |
| [doc/Architecture.md](doc/Architecture.md) | Layers, core split, memory, boot/thread/Zigbee flows |
| [doc/Board_Map.md](doc/Board_Map.md) | Peripherals, clocks, DMA engines, pin map |
| [doc/Plan.md](doc/Plan.md) | Sprints, dependencies, risks |
| [doc/UI_Design.md](doc/UI_Design.md) | Shell, screens, navigation and pairing flows |
| [doc/Requirements_and_Test_Cases.md](doc/Requirements_and_Test_Cases.md) | Shall statements, tests, traceability, CI reqs |
| [doc/CICD.md](doc/CICD.md) | GitHub Actions, static analysis, coverage gates |
| [third_party/README.md](third_party/README.md) | What we vendor from ST vs write ourselves |

## Hardware (used by the design)

- STM32H745XIH6 — Cortex-M7 480 MHz + Cortex-M4 240 MHz
- 4.3" 480×272 RGB panel, FT5336 touch
- 16 MB SDRAM at `0xD0000000`; 8 MB mapped (16-bit FMC, 12 row × 8 col × 4 banks)
- Dual QSPI NOR (memory-mapped assets)
- 4 GB eMMC (user files)
- WM8994 audio, LAN8740A Ethernet
- Wi-Fi only if an ESP32 is added on Arduino / STMod+
- Zigbee via a **TI ZNP** module on CN2 STMod+ (USART2 PD5/PD6, 921600); STM32 is the host/coordinator

## Design rules (short)

1. Apps do not call LVGL, FatFS, MQTT, MT, or HAL. There is no RTOS in the firmware images.
2. M7 owns UI, VFS, image decode, game sim, Zigbee host, and local automations. M4 owns SAI audio (and optional net or ZNP UART).
3. IPC is versioned messages in SRAM4, not shared C pointers.
4. Explorer is jailed to `/user`. Home UI talks only to `home_*`. Game logic talks only to `game_module_t` / `gfx_*`.
5. TI ZNP is an expansion on STMod+ USART2; USART3 stays the console.

# Build

Host tests (no board) use **Unity** (`third_party/unity`):

```
cmake --preset host-tests
cmake --build --preset host-tests
ctest --test-dir build-host --output-on-failure
```

PC LVGL window (Sprint 5b) — same shell as the board, SDL2, **Ubuntu and Windows**. Unit tests stay LVGL-free.

Ubuntu:

```
sudo apt-get install -y libsdl2-dev ninja-build
cmake --preset host-sim
cmake --build --preset host-sim
./build-sim/host_sim
```

Windows (Ninja + SDL2 via CMake FetchContent if not installed):

```
cmake --preset host-sim
cmake --build --preset host-sim
build-sim\host_sim.exe
```

The window is 480×272 at 2× scale. Demo files are seeded under `user/` next to the binary (`hello.txt`, `notes.txt`, `photo.jpg` / `.png` / `.bmp`, `sub/`, …). Override with `H745_SIM_USER` or `host_sim /path/to/folder`. Close the window to quit.

Firmware is built with the **STM32 VS Code** CubeCLT `arm-none-eabi-gcc` (same as `stm32h745-blinky`). That compiler is not on a normal shell PATH — do not expect `arm-none-eabi-gcc` from the terminal.

ST HAL/LL v1.11.6, CMSIS device H7 v1.10.7, and CMSIS Core v5.9.0 live in `third_party/` (git submodules). Glue lives in `firmware/src/port/cube`.

## STM32 VS Code extension

Install the [STM32CubeIDE for Visual Studio Code](https://marketplace.visualstudio.com/items?itemName=STMicroelectronics.stm32-vscode-extension) pack (brings CMake Tools, Ninja, `arm-none-eabi-gcc` via CubeCLT). Then:

1. `git submodule update --init --recursive`
2. **File → Open Folder** on this repo (or open `stm32h745-disco.code-workspace`)
3. Accept **Configure discovered CMake project(s) as STM32Cube project(s)?**
4. If the tool asks to map cores, set **CM7 → Cortex-M7** and **CM4 → Cortex-M4** (root is the dual-core CMake superbuild; each core is its own folder)
5. Select the **Debug** CMake preset and build — that produces `CM7/build/stm32h745-disco_CM7.elf` and `CM4/build/stm32h745-disco_CM4.elf`
6. **Flash without debugging:** **Terminal → Run Task… → Build + Flash**. That programs both ELFs over SWD and pulses NRST so the board runs on its own. Stop any debug session first (ST-LINK can only have one owner). Or **Flash board** if the ELFs are already built.
7. Debug with **CM7_Debug** (also flashes both ELFs) and optionally **DualCore_Debug** when you need breakpoints.

The STM32 configuration tool may add local `cube-cmake` / `starm-clangd` keys to `.vscode/settings.json`; leave those. Do not change the Debug preset to a single core. Register view uses in-tree SVD files (`third_party/cmsis-svd/`). Device is `STM32H745XIH6` on **STM32H745I-DISCO**. Apps still never include HAL; only `firmware/src/port/cube` and `firmware/src/bsp` do.

CI: format, layering, cppcheck, clang-tidy, coverage, and both ELF images — see `doc/CICD.md`.

## Sprint 10 boot log (USART3 115200)

After memory, eMMC, and display bring-up the M7 starts the LVGL shell (32 px status + 3×2 launcher: Files, Home, Game, Music, Calendar, Settings). The status bar shows time from the RTC, **eMMC**, **M4**, and **ETH** (red = down, amber = link/no IPv4, green = IPv4). Files lists `/user`; tap a folder or a known type to open the text, image, or audio player. Settings shows brightness, volume, link/IPv4, Zigbee channel and join default, and About (M7/M4 versions). Game is a library: built-in Brick, bundled CHIP-8 titles (Snek, Super Pong, Br8kout), plus carts from `/user/game` (`*.ch8` / `*.c8`). LD2 (PI13) still blinks:

```
M7 stm32h745-disco s10
clk ok
sysclk 1C9C3800
mpu on
cache on
sai_clk ok
ipc ok
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
...
rtc ok
net ok
disp ok
input ok
touch ok
ui ok
shell ready
```

Under ST-Link the MPU live probe is skipped (`mpu_faults 0`) so the debugger does not stop in `MemManage_Handler`. Without a debugger the probe still faults once at `0x2407FFE0` (`mpu_faults 1`).

`m4 ready` and `ipc_rtt` may appear earlier or later depending on when M4 attaches. `ipc_rtt` is a DWT ping-pong in microseconds (budget ≤ 2000). `m4: m4 ready` is the M4 log line relayed over IPC (M4 does not use USART3). Halt M4 in the debugger: the **M4** status label turns red within 1 s; the launcher stays navigable.

Tap a tile logs `shell_push <id>`. Files logs `files /user` (or the cwd) when entering a folder; opening a file logs `shell_push text|image|player`. Back in a nested folder logs `files <parent>`; Back at `/user` logs `shell_pop`. `emmc fail` / `vfs fail` is a soft error (status shows eMMC error); the shell still runs. `ui_fps ok` is ≥ 20 FPS. `touch none` is OK on boards that ship GT911 instead of FT5336. `qspi_word FFFFFFFF` means the NOR is erased; mmap worked (no bus fault). Text wraps and pages large logs; a corrupt image shows “Can't open image” and Back returns to Files. WAV/MP3 open into the player; pause/resume and volume go to M4 over IPC while the explorer can still scroll.

## License

MIT — see [LICENSE](LICENSE).
