# Development Plan

HMI firmware for STM32H745I-DISCO. Architecture and portability rules are in `Architecture.md`. UI screens are in `UI_Design.md`. Requirements and tests are in `Requirements_and_Test_Cases.md`.

## 1. Product intent

A small dual-core handheld-style shell on the 4.3" panel:

- Launcher
- File explorer, image viewer, text viewer
- Audio player
- **Game** (first title: Brick)
- **Home** (lights/switches/sensors over MQTT)
- Status (time, storage, Ethernet; optional Wi-Fi)

Game and Home use the same `ui_app_t` contract as Files. Game logic is a host-testable sim; Home UI talks only to `home_*`, never to MQTT.

## 2. Non-goals (this generation)

- TouchGFX UI.
- Writing a custom GPU toolkit.
- Linux / MPU migration.
- Full POSIX, USB mass-storage gadget, or network file server.
- Treating ESP32 as on-board hardware.
- A 3D / GPU game engine, or Home Assistant running **on** the STM32.
- Binding Home UI to one vendor app (HA dashboard scrape, Zigbee coordinator, Matter stack on-chip).

## 3. Platform choices

| Concern | Now | Later |
| --- | --- | --- |
| RTOS | FreeRTOS on each core (M7 first) | Zephyr |
| UI | LVGL over `disp_*` / `input_*` | LVGL on Zephyr |
| FS | FatFS on eMMC behind `vfs_*` | Zephyr FS |
| IPC | SRAM4 rings + HSEM | OpenAMP / RPMsg |
| Net | LwIP on a single core | Zephyr net |
| Home bus | `home_*` + mock, then MQTT | Zephyr MQTT, same `home_*` |
| Game | `game_sim` + `gfx_*` | Same modules; gfx via Zephyr display |
| Wi-Fi | Optional ESP32 AT/SPI driver | Same `net_*` API |
| Build | CMake + STM32Cube HAL in `port/cube` | `west` + DTS |

M7-only is acceptable through Sprint 5. M4 starts when audio or offloaded net lands.

## 4. Sprints

Each sprint has a demo on hardware or a host-test gate. Do not start the next sprint until the exit check passes.

### Sprint 0 — Repo and contracts (1–3 days)

- CMake skeleton: `m7`, `host-tests`.
- Empty headers for OSAL, VFS, disp, input, IPC messages.
- UART3 log, assert, reset reason.
- **Exit:** host build of `tests/host` runs on PC; M7 blinks LED and prints over VCP.

### Sprint 1 — Clocks, memory, MPU

- M7 clock 480 MHz, MPU regions, cache enable with documented policy.
- SDRAM init at `0xD0000000` (8 MB).
- QSPI memory-map at `0x90000000` (read).
- SRAM4 reserved and marked non-cacheable.
- **Exit:** walking-bit test on SDRAM; execute-from-QSPI smoke; MPU fault test harness.

### Sprint 2 — Display and touch BSP

- LTDC RGB565, double framebuffer in SDRAM, DMA2D fill/copy.
- Backlight, display on, FT5336 on I2C4 with bus mutex.
- `disp_flush` + `input_poll` only; no LVGL yet.
- **Exit:** color bars, touch crosshair, ≥ 30 FPS full-screen fill.

### Sprint 3 — Storage VFS

- SDMMC1 + FatFS (or equivalent) behind `vfs_*`.
- Mount `/user` on eMMC; reject path escape (`..`, absolute outside jail).
- Directory listing, sequential read benchmark.
- **Exit:** host tests for path jail and extension dispatch; HIL sequential read ≥ 15 MB/s on large files (see REQ-STG-02).

### Sprint 4 — LVGL shell

- LVGL ported **only** in `ui/backend_lvgl`.
- Theme tokens, 32 px status bar, launcher grid, navigation stack.
- Dummy apps that push/pop screens (Files stub, Game stub, Home stub).
- Launcher 3×2: Files, Home, Game, Music, Network, Settings.
- **Exit:** launcher opens and returns from two stub apps; 20 FPS UI with touch scroll.

### Sprint 5 — File explorer

- List/grid of `/user`, breadcrumbs, empty and error states.
- Open-with registry: `.txt/.md/.c/.h` → text, `.jpg/.jpeg/.png/.bmp` → image, `.mp3/.wav` → player.
- **Exit:** browse nested folders, open a file into a stub viewer, Back returns with list position restored.

### Sprint 6 — Text and image viewers

- Text: UTF-8, wrap, windowed read (do not load huge files).
- Image: JPEG HW codec when possible, SW PNG/BMP, fit-to-screen, pan, next/prev in folder.
- **Exit:** 10 MB log scrolls; 2 MP JPEG downscales without killing the shell; corrupt file shows an error modal.

### Sprint 7 — M4 bring-up and IPC

- M4 independent image, HSEM notify, lockless rings.
- `ipc_msg` version check and credit/back-pressure.
- Heartbeat + log relay to M7.
- **Exit:** host tests for ring wrap and overflow; HIL round-trip < 2 ms; M4 halt is visible in the status bar.

### Sprint 8 — Audio pipeline

- WM8994 + SAI DMA ping-pong on M4.
- Helix (or replacement) MP3 + WAV behind `media_open_audio`.
- Mini-player in the status/now-playing bar; full player screen.
- **Exit:** 44.1 kHz stereo, no audible glitch while scrolling the explorer; pause/resume/volume via IPC.

### Sprint 9 — Network and time

- Ethernet LwIP on **one** core, link LED/status in the bar.
- Document QSPI-bank2 vs ETH pin policy in BSP.
- RTC on the status bar; NTP optional.
- Optional ESP32 failover **only** if hardware is attached (compile-time).
- **Exit:** DHCP or static IP shown; unplug RJ45 updates status < 2 s; if ESP32 enabled, failover within the REQ-NET timeout.

### Sprint 10 — Game

- `game_module_t` host + **Brick** (paddle, bricks, one-finger drag).
- `gfx_*` on a playfield buffer; pause on Back/Home; high score in `/user/game`.
- **Exit:** host tests for collision/score; HIL ≥ 30 FPS; Home button returns to launcher with audio still healthy.

### Sprint 11 — Home automation

- `home_*` with `mock` backend (8 rooms / 32 devices capacity tests on host).
- MQTT backend: connect, discover or static map, on/off + brightness for lights, binary sensor read-only.
- Dashboard + room list; offline banner + last-known state; broker URL in Settings.
- **Exit:** mock UI works with Ethernet unplugged; with broker, toggle a light and see state round-trip; command failure reverts the switch.

### Sprint 12 — Hardening

- Watchdogs on both cores, brown-out, FS remount, OOM UI.
- Settings app (brightness, volume, IP, MQTT broker).
- HIL pack for the requirement matrix, including game soak and home mock.
- **Exit:** RTM P0/P1 rows green; 8-hour soak (UI + audio + explorer; game 30 min; home mock) without leak or deadlock.

### Later — Zephyr port (not a product sprint yet)

- `osal/zephyr`, DTS for this board, OpenAMP transport, same apps.
- Track as a spike after Sprint 4 so the HAL stays honest.
- Home MQTT client and game `gfx_*` must survive that spike without app changes.

## 5. Suggested GUI change (locked for v2)

Replace the v1 left dock (50×272) + 430×242 canvas with:

- Status bar 480×32
- Content 480×240 (launcher or app)
- In-app top bar 480×40 over content
- Now-playing 480×36 only when audio is active

Rationale: 40 px minimum hit targets, more room for lists and images, matches LVGL patterns, no toolkit-specific dock widget to throw away later.

## 6. Risks

| Risk | Mitigation |
| --- | --- |
| SDRAM bandwidth vs LTDC | RGB565, double buffer, DMA2D; measure FPS in Sprint 2 |
| I2C4 contention (touch vs codec) | BSP mutex; never I2C from ISR |
| ETH / QSPI pin mux | Pick one policy; test both XiP and Ethernet |
| FatFS + LVGL on one core | FS work on a worker thread; UI thread never blocks on eMMC |
| M7 D-cache vs DMA/IPC | Non-cacheable SRAM4; cache API for DMA |
| TouchGFX samples leaking in | Ban TouchGFX in review; LVGL-only backend |
| Game in LVGL widgets | Keep `game_sim` + `gfx_*`; LVGL canvas is a backend, not the model |
| Home UI talking MQTT | `home_*` only; mock backend so UI is not blocked on Sprint 9 |
| MQTT / HA schema churn | Prefer a small topic map; HA discovery is optional |
| Scope (extra games, climate, NTP, Wi-Fi) | C/P2; do not block Files, Brick, or light toggles |

## 7. Deliverables per milestone

| Milestone | Demo |
| --- | --- |
| M1 (Sprint 2) | Touch paint on color bars |
| M2 (Sprint 5) | Browse eMMC and open files |
| M3 (Sprint 6) | View JPEG + UTF-8 text |
| M4 (Sprint 8) | Play MP3 while browsing |
| M5 (Sprint 10) | Brick playable at ≥ 30 FPS |
| M6 (Sprint 11) | Toggle a mocked (and optionally MQTT) light |
| M7 (Sprint 12) | Full shell, soak |

## 8. Documentation map

| File | Contents |
| --- | --- |
| `Design_Review.md` | Why v1 changed |
| `Architecture.md` | Layers, memory, interfaces |
| `UI_Design.md` | Shell, screens, theme |
| `Requirements_and_Test_Cases.md` | Shall statements + tests + RTM |
| `Plan.md` | This file |
