# Development Plan

HMI firmware for STM32H745I-DISCO. Architecture and portability rules are in `Architecture.md`. UI screens are in `UI_Design.md`. Requirements and tests are in `Requirements_and_Test_Cases.md`. CI/CD is in `CICD.md`.

## 1. Product intent

A small dual-core handheld-style shell on the 4.3" panel:

- Launcher
- File explorer, image viewer, text viewer
- Audio player
- **Game** (library: built-in Brick + retro carts from eMMC)
- **Home** (TI ZNP Zigbee host: network, device list, local automations)
- Status (time, storage, Ethernet, Zigbee radio; optional Wi-Fi)

Game and Home use the same `ui_app_t` contract as Files. Game logic is a host-testable sim. Home UI talks only to `home_*`; the Zigbee radio is a TI ZNP on UART, with `zb_host` + `auto_*` on the STM32.

## 2. Non-goals (this generation)

- TouchGFX UI.
- Writing a custom GPU toolkit.
- Linux / MPU migration.
- Full POSIX or a network file server.
- littlefs (or any second user-visible filesystem). The explorer volume stays FAT.
- USB MSC as always-on gadget (it is **in-scope later**, exclusive / opt-in — see Later — USB MSC).
- Treating ESP32 or TI ZNP as on-board hardware (both are UART expansions).
- A 3D / GPU game engine, libretro / MAME, or a NES/GB/SNES core on this MCU (CHIP-8 carts are the first eMMC-loadable retro core).
- Home Assistant / zigbee2mqtt running **on** the STM32.
- Binding Home UI to MQTT or HA dashboards as the primary control path.
- A Matter/Thread stack, or a full Linux Zigbee gateway, on this MCU.

## 3. Platform choices

| Concern | Now | Later |
| --- | --- | --- |
| RTOS | FreeRTOS on each core (M7 first) | Zephyr |
| UI | LVGL over `disp_*` / `input_*` | LVGL on Zephyr |
| Host UI sim | none (logic-only `host-tests`) | SDL2 window on **Ubuntu and Windows** (Sprint 5b) |
| FS | FatFs (FAT) on eMMC behind `vfs_*` | Same FAT volume (Zephyr FAT or FatFs). No littlefs |
| USB MSC | Out until a later sprint | TinyUSB device MSC; exclusive with FatFs |
| IPC | SRAM4 rings + HSEM | OpenAMP / RPMsg |
| Net | LwIP on a single core | Zephyr net |
| Home | `home_*` + `zb_host` + TI ZNP UART + `auto_*` | Same host; Zephyr `uart` only |
| Game | `game_sim` + `gfx_*` | Same modules; gfx via Zephyr display |
| Wi-Fi | Optional ESP32 AT/SPI driver | Same `net_*` API |
| Build | CMake + STM32Cube HAL in `port/cube` | `west` + DTS |
| Cube package | Component repos only (HAL/LL, ft5336, …) | Zephyr DTS; ST components dropped |

**Vendor rule:** write our Discovery BSP; do not submodule [STM32CubeH7](https://github.com/STMicroelectronics/STM32CubeH7) or `stm32h745i-disco-bsp`. Pull chip drivers and upstream middleware per sprint — see `Architecture.md` §3.1 and `third_party/README.md`.

M7-only is acceptable through Sprint 5. M4 starts when audio or offloaded net lands.

## 4. Sprints

Each sprint has a demo on hardware or a host-test gate. Do not start the next sprint until the exit check passes.

```mermaid
flowchart LR
  S0[0 CI + contracts] --> S1[1 memory]
  S1 --> S2[2 display]
  S2 --> S3[3 VFS]
  S3 --> S4[4 LVGL shell]
  S4 --> S5[5 explorer]
  S5 --> S5b[5b host sim]
  S5 --> S6[6 viewers]
  S5b -.-> S6
  S4 --> S10[10 game]
  S6 --> S7[7 M4 IPC]
  S7 --> S8[8 audio]
  S7 --> S9[9 net]
  S4 --> S11[11 ZNP]
  S11 --> S12[12 automations]
  S8 --> S13[13 harden]
  S10 --> S10b[10b retro]
  S10b --> S13
  S12 --> S13
  S9 --> S13
```

```mermaid
gantt
  title Firmware sprints
  dateFormat  YYYY-MM-DD
  axisFormat  %b %d
  section Platform
  Sprint 0 CI and contracts     :s0, 2026-09-14, 4d
  Sprint 1 memory MPU           :s1, after s0, 7d
  Sprint 2 display touch        :s2, after s1, 7d
  Sprint 3 VFS                  :s3, after s2, 7d
  section Shell
  Sprint 4 LVGL                 :s4, after s3, 10d
  Sprint 5 explorer             :s5, after s4, 7d
  Sprint 5b host sim Ubuntu+Win :s5b, after s5, 5d
  Sprint 6 viewers              :s6, after s5, 10d
  section Dual core
  Sprint 7 M4 IPC               :s7, after s6, 7d
  Sprint 8 audio                :s8, after s7, 10d
  Sprint 9 net                  :s9, after s7, 7d
  section Apps
  Sprint 10 game                :s10, after s4, 7d
  Sprint 10b retro library      :s10b, after s10, 5d
  Sprint 11 ZNP host            :s11, after s4, 14d
  Sprint 12 automations         :s12, after s11, 7d
  section Quality
  Sprint 13 harden CI HIL       :s13, after s8, 10d
```

Dates are indicative; the dependency graph is normative. Game and ZNP may overlap after the shell exists. Sprint 5b (PC window) may overlap Sprint 6; it does not block viewers on hardware.

### Sprint 0 — Repo, contracts, CI (1–3 days)

- CMake skeleton: `m7`, `host-tests`.
- Empty headers for OSAL, VFS, disp, input, IPC messages.
- UART3 log, assert, reset reason.
- GitHub Actions `ci.yml` as specified in `CICD.md`: format, layering grep, host-tests job, ARM GCC hello.
- `.clang-format`, `scripts/ci/check-layering.sh`.
- **Exit:** host build of `tests/host` runs on PC; M7 blinks LED and prints over VCP; a PR cannot merge if layering or format fails.

### Sprint 1 — Clocks, memory, MPU

Status: **done** (host-tested walking/MPU map; board boot log is the HIL check).

- M7 clock 480 MHz via STM32Cube HAL (`HAL_RCC_*`, VOS0 / SMPS 1.8 V supplies LDO), MPU regions (`HAL_MPU_*`), cache via CMSIS. USART3 console and LED GPIO use the LL driver from the same [`stm32h7xx-hal-driver`](https://github.com/STMicroelectronics/stm32h7xx-hal-driver) package.
- SDRAM init at `0xD0000000` (8 MB, 16-bit FMC bank 2).
- QSPI memory-map at `0x90000000` (read; dual-flash pins, bank-1 1-1-1 smoke). Blank NOR is `0xFF` — do not execute it; true XiP comes with programmed assets.
- SRAM4 reserved and marked non-cacheable, no-execute.
- **Exit:** walking-bit test on SDRAM; QSPI READ ID + mmap read without bus fault; MPU 32-byte no-access fault harness (skip stacked PC).

### Sprint 2 — Display and touch BSP

Status: **done** (host-tested clip/RGB565; board boot log is the HIL check).

- LTDC RGB565, double framebuffer in SDRAM, DMA2D fill/copy — our BSP + HAL, not `BSP_LCD_*`.
- Submodule ST components `stm32-rk043fn48h` (panel timings) and `stm32-ft5336` (touch). I2C4 bus mutex in our BSP.
- `disp_flush` + `input_poll` only; no LVGL yet. Reload and touch INT are polled (no extra IRQs).
- **Exit:** color bars, touch crosshair, ≥ 30 FPS full-screen fill.

### Sprint 3 — Storage VFS

Status: **done** (host-tested jail, extension dispatch, RAM VFS dir walk; board boot log is the HIL check).

- SDMMC1 + FatFs R0.15b behind `vfs_*`. `diskio` is our BSP (polling + IDMA bounce in AXI SRAM).
- Mount `/user` on eMMC; reject path escape (`..`, absolute outside jail).
- Directory listing, sequential read benchmark (`/user/s3.bin`).
- **Exit:** host tests for path jail and extension dispatch; HIL sequential read ≥ 15 MB/s on large files (see REQ-STG-02).

### Sprint 4 — LVGL shell

Status: **done** (host-tested nav stack, launcher geometry, stub registry; board boot log is the HIL check).

- LVGL v9.5.0 ported **only** in `ui/backend_lvgl`.
- Theme tokens, 32 px status bar, launcher grid, navigation stack.
- Dummy apps that push/pop screens (Files stub with a scroll list, Game/Home/Music/Network/Settings stubs).
- Launcher 3×2: Files, Home, Game, Music, Network, Settings.
- **Exit:** launcher opens and returns from two stub apps; 20 FPS UI with touch scroll.

### Sprint 5 — File explorer

Status: **done** (host-tested listing, open-with, Back restores scroll).

- List of `/user` with breadcrumb cwd, empty / unmounted / IO / unknown-type prompt.
- Open-with: `.txt/.md/.c/.h/.log` → text, `.jpg/.jpeg/.png/.bmp` → image, `.mp3/.wav` → player.
- No `..` row; Back leaves the folder. Unknown files: Properties + “Open as text?”.
- **Exit:** browse nested folders, open a file into a stub viewer, Back restores list position.

### Sprint 5b — Host LVGL simulator (Ubuntu + Windows)

Status: **done** (SDL2 window on Ubuntu; Windows link is a CI gate).

Run the same shell on a PC so Files/launcher can be checked without a Discovery board. **Ubuntu and Windows are both first-class.** One backend, not two window toolkits.

Lock:

- **Windowing:** upstream LVGL v9 **SDL2** (`LV_USE_SDL`). Do not add a Win32-only backend, an X11-only backend, or a second widget tree. macOS is nice-to-have if SDL works; it is not an exit gate.
- **Target:** CMake preset `host-sim` → `host_sim`. This is **not** `host-tests`. Unit tests stay LVGL-free (`Architecture.md` §11, `CICD.md`).
- **Code share:** reuse `ui/backend_lvgl/ui_lvgl.c`, `src/shell`, `src/app`. Swap only the port: host `lv_port` + `lv_conf_sim.h` (malloc heap, not `LV_MEM_ADR 0x24010000`). Apps still never include `lvgl.h`.
- **Panel:** 480×272 RGB565. Optional integer scale (2×) so the window is readable on a desktop. Mouse is the pointer (down/move/up in panel space).
- **VFS:** map a host folder to `/user` (jail still applies). Seed tree for demo files. No FatFs, no HAL, no FreeRTOS.
- **Out of sim:** real audio SAI, Ethernet, TI ZNP UART. Those stay stubs or `mock` until their sprints.
- **Deps:** Ubuntu `libsdl2-dev`; Windows SDL2 via vcpkg or CMake `FetchContent`. Document both in README when the sprint lands.
- **CI:** link `host-sim` on GitHub `ubuntu-24.04` **and** `windows-latest`. A display is not required in CI (no xvfb gate). Coverage floor does not include LVGL/SDL.

- **Exit:** `cmake --preset host-sim && cmake --build --preset host-sim` produces a windowed binary on Ubuntu and on Windows; launcher opens Files against a host `/user` folder; Back restores the list; layering grep still passes.

### Sprint 6 — Text and image viewers

Status: **done** (host-tested windowed UTF-8 text, SW JPEG/PNG/BMP, error state; JPEG HW is `ERR_UNSUPPORTED` until the HAL JPEG port).

- Text: UTF-8, wrap, windowed read (`TEXT_WIN_MAX` 32 KB; files larger than 256 KB are paged). Invalid bytes become `?`. CRLF/LF/CR are line breaks.
- Image: software JPEG (ChaN TJpgDec), PNG, BMP; contain-fit into 480×200 RGB565. Next/prev in the folder. JPEG hardware codec is tried first and falls back to SW.
- **Exit:** 10 MB log scrolls via windows; 2 MP JPEG downscales (TJpgDec 1/2–1/8 + contain-fit) without allocating a full RGB buffer; corrupt file shows an error and Back returns to Files.

### Sprint 7 — M4 bring-up and IPC

Status: **done** (host-tested lockless wrap/overflow, credits, version, heartbeat watch; HIL: ping-pong RTT and M4 status).

- M4 independent image, HSEM notify (polled), lockless SRAM4 rings.
- `ipc_msg` version check and credit/back-pressure (`ERR_NOSPC`, no overwrite).
- Heartbeat + log relay to M7. Status bar **M4** goes red if the peer is silent > 500 ms.
- **Exit:** host tests for ring wrap and overflow; HIL round-trip < 2 ms; M4 halt is visible in the status bar.

### Sprint 8 — Audio pipeline

Status: **done** (host-tested mixer, WAV/MP3 probe, bitstream pipe, player model; firmware links M7 codec + M4 SAI DMA).

- WM8994 + SAI DMA ping-pong on M4.
- Helix MP3 + WAV behind `media_open_audio`.
- Mini-player in the status/now-playing bar; full player screen.
- **Exit:** 44.1 kHz stereo, no audible glitch while scrolling the explorer; pause/resume/volume via IPC.

### Sprint 9 — Network and time

Status: **done** (host-tested net service, DHCP/static, 10 s failover, RTC clock; firmware links M7 MII + LwIP + HAL RTC).

- Ethernet LwIP on **M7 only** (M4 stays SAI). Status-bar ETH + RTC.
- QSPI-bank2 vs ETH pin policy: keep default SB3/SB4 (see BSP README). MII 100 full-duplex, no CRS/COL.
- Optional ESP32 failover **only** if `NET_WIFI` is compiled; default off (host tests inject a fake Wi-Fi link).
- **Exit:** DHCP or static IP shown; unplug RJ45 updates status < 2 s (PHY poll 200 ms); if ESP32 enabled, failover within the REQ-NET timeout.

Follow-on (landed after the Sprint 9 exit, still not Sprint 10):

- Launcher is **Files, Home, Game, Music, Calendar, Settings**. Network is not a home tile.
- Network status lives in **Settings** (link, IPv4, path, NTP). Opening `APP_ID_NETWORK` shows Settings.
- SNTP to **216.239.35.0** when Ethernet has IPv4; time is UTC; `SNTP_SET_SYSTEM_TIME` → `time_ntp_apply_unix`.
- Calendar month grid (drawn with objs, not `lv_calendar`) + live clock. Tile shows today’s day number.
- Light theme, rounded-rect home tiles, mini-player-aware launcher height.

### Follow-up — DMA2D + JPEG hardware (after Sprint 9, not Sprint 10)

Status: **planned** (REQ-IMG-03 is still a stub: `media_jpeg_hw_decode` returns `ERR_UNSUPPORTED`).

Both blocks are on the H745 and are applicable. They are **not** a general GPU for LVGL. Use them together for the image viewer; keep LVGL software draw.

**Why they go together**

The JPEG codec outputs YCbCr MCU blocks, not RGB565. DMA2D (Chrom-ART) is the H7 path that converts YCbCr → RGB565. DMA2D does **not** scale. The JPEG codec does **not** downscale. TJpgDec already does 1/2–1/8 while decoding, which is the right path for 2 MP photos (REQ-IMG-02, TC-IMG-01 1920×1080).

**Do now (one slice)**

1. Add `stm32h7xx_hal_jpeg.c` to the M7 Cube list. Clock JPEG in BSP. Poll only (vector table stays the 16 Cortex-M exceptions; no JPEG/DMA2D/MDMA IRQ).
2. New BSP API `board_jpeg_decode(...)` in `firmware/src/bsp/stm32h745i_disco/`. `media_jpeg_hw_decode` calls it on `STM32H745xx` and stays `ERR_UNSUPPORTED` on host (TC-IMG-03 unchanged).
3. HW path only when the frame is **baseline** and YCbCr fits the unused SDRAM slot at `+0x080000` (261 KB). Practical gate: **width ≤ 480 and height ≤ 272** (panel-sized album shots, TC-IMG-01 480×272). 4:4:4 / 4:2:2 / 4:2:0 only.
4. Flow: JPEG poll-decode → YCbCr staging → DMA2D convert into the existing 480×200 RGB565 dest at `+0x0C0000` with contain-fit offsets. Cache-clean before DMA2D; poll `HAL_DMA2D_PollForTransfer`.
5. Any header parse fail, progressive, odd chroma, oversize, timeout, or HAL error → existing TJpgDec path. Never fail the viewer because HW is busy.
6. Do **not** set `LV_USE_DRAW_DMA2D`. LVGL stays `LV_USE_DRAW_SW` + DIRECT into FB0/FB1. Flush stays cache-clean + LTDC address flip. DMA2D in `disp_fill` / `disp_flush` stays a HAL helper; the shell does not call it.

**Do not in this slice**

- LVGL Chrom-ART draw unit (cache vs DIRECT, second DMA2D owner next to JPEG, IRQ policy).
- Full-resolution HW decode of 2 MP then CPU scale (burns SDRAM; TJpgDec is smaller and already works).
- LibJPEG, Cube JPEG examples copied in, or extra IRQ vectors.
- Sprint 10 game / `gfx_*` DMA2D blit (that sprint owns playfield FPS).

**Later (only if measured)**

- If `ui_fps` stays under ~20, consider `LV_USE_DRAW_DMA2D` for opaque fills only, still polled, still firmware-only (`lv_conf` host-sim stays 0).
- If album shots are all near-panel size, raise the HW size gate; keep TJpgDec for everything else.

**Exit**

- Host: `media_jpeg_hw_decode` still `ERR_UNSUPPORTED`; golden SW JPEG checksum unchanged.
- HIL: 480×272 JPEG paints via HW (UART can log `jpeg hw`); 1920×1080 still TJpgDec; truncated JPEG still errors; `disp ok` / `shell ready` unchanged.
- Layering: apps/shell still have no HAL/LVGL; `svc` still has no `stm32h7xx_hal*.h`.

### Sprint 10 — Game

Status: **done** (host-tested Brick collision/score/`gfx` spy + VFS high score; firmware links M7 playfield).

`game_module_t` host + **Brick** (paddle, bricks, one-finger drag). `gfx_*` on a playfield buffer; pause on Back/Home; high score in `/user/game`.

Lock:

- **Sim:** `game_module_t` in `src/game`. First module id `brick`. No LVGL, HAL, or FatFs in `src/game`. Apps stay LVGL-free; `src/app/game.c` is the shell wrapper (VFS save + tick/draw).
- **Draw:** `gfx_clear` / `gfx_fill` / `gfx_blit` into one RGB565 playfield. LVGL shows that buffer as a single `lv_image`. Not one widget per brick. Host tests use a `gfx` spy (clear + paddle fill).
- **Buffer:** while Game is foreground, borrow the image SDRAM window at `+0x0C0000` (host: static 480×240). Leaving Game releases it for the viewer. Do not enable `LV_USE_DRAW_DMA2D` or JPEG HW in this sprint.
- **Field:** `reset(w,h)` sized to `content_h() - APPBAR_H` (mini-player may shrink it). App bar stays (Back + live score/lives/high + pause). Drag band on the lower playfield ≥ 40 px (56 px when height allows).
- **Control:** one-finger drag only. Back while playing pauses; Back while paused or game-over pops. Pause overlay: Resume + Quit. Game-over: New game + Quit. Home / Quit returns to launcher; `game_close` must not stop audio.
- **Save:** high score in `/user/game/brick.sav` (decimal text). `vfs_mkdir("/user/game")` if needed. Keep the value in RAM if VFS is down.
- **Tick:** do not rebuild the widget tree on every `game_step` (same rule as player elapsed). Bump `game_gen` only on pause / resume / new / over.
- **Registry:** `game_module_by_id("brick")` so a second title can register later without changing the shell. No second title in this sprint.
- **Out:** no DMA2D blit, no extra NVIC, no SDMMC/ETH/SAI vectors, no ZNP pairing, no Home automations.

- **Exit:** host tests for collision/score/`gfx` spy; HIL ≥ 30 FPS; Home returns to launcher with audio still healthy.

### Follow-up — Retro library (after Sprint 10, not Sprint 11)

Status: **done** (host-tested library jail, CHIP-8 load/draw, Brick still playable).

The Game app is a **library**. Brick stays the built-in title. Other titles are **carts on eMMC** under `/user/game`, opened through `vfs_*` (jail still applies). Each cart is a ROM plus a `game_module_t` **core**. Do not fork the shell or draw one LVGL widget per sprite.

Lock:

- **Library:** opening Game lists titles. Row 0 is always **Brick**. Then every `/user/game/*.ch8` and `*.c8` (skip `.sav`). Tap starts that core. Back while playing pauses; Back while paused or “Games” returns to the library; Back in the library pops to the launcher. Audio stays running.
- **Load:** `game_module_t.load(rom, n)` is optional. Brick has `load == NULL`. A core copies the ROM into its own RAM; it does not mmap eMMC. Max CHIP-8 ROM **3584** bytes (4 KB machine minus `0x200`). Jail: path must normalize under `/user`.
- **First core:** **CHIP-8** (`id "chip8"`). Host-test the opcode groups used by the bundled demo (CLS, LD, ADD, JMP, DRW, font). 64×32 display scaled into the playfield via `gfx_fill`; 4×4 COSMAC keypad on the right when width allows (40 px cells), otherwise a lower-band 4×4. Delay timer at 60 Hz. **No SAI / no beep** (do not steal the music pipe).
- **Files:** `.ch8` / `.c8` probe as `MEDIA_KIND_GAME` and open the Game app with that path (same as `.mp3` → player).
- **Saves:** Brick keeps `/user/game/brick.sav`. Carts do not share that file. No copyrighted ROM dumps in tree; seed only an original tiny demo (`demo.ch8`) for host-sim / tests.
- **Later cores (not this slice):** Game Boy / NES would be new `game_module_t` files + an extension table. Out until a dedicated sprint. No libretro, no MAME, no ZIP, no extra IRQ, no DMA2D blit.
- **Out:** ZNP pairing, JPEG HW, `LV_USE_DRAW_DMA2D`.

- **Exit:** host tests for library jail, CHIP-8 load/draw, Brick still playable; HIL: copy a `.ch8` onto eMMC, it appears in Game and runs; Brick still ≥ 30 FPS.

### Sprint 11 — Zigbee host (TI ZNP)

Status: **done** (mock-first Home; no extra NVIC).

`znp_mt` framing is already in tree. This sprint makes Home a real app: device table, persist, pair countdown, and a mock backend so the panel is usable without a dongle.

Lock:

- **Mock first:** seed ≥ 2 rooms and ≥ 4 devices (Living lamp, Hall switch, Front door, Motion stair) so Home works on the board and host-sim with no CC2652 (`REQ-HOME-02`). Firmware `uart_*` for `UART_ID_ZNP` returns `ERR_IO` (no USART1 IRQ this sprint). Host `uart_*` is a software stub that can auto-reply SYS_PING (`0x21 0x01` → `0x61 0x01`).
- **USART3 stays console.** Never steal it for ZNP. No new NVIC vector.
- **Apps use only `home_*`.** `src/app/home.c` must not include `znp_mt.h`, `uart.h`, HAL, MQTT, or LwIP (`REQ-HOME-01`).
- **`zb_host`** owns the device table (32), form / permit-join countdown, interview apply, and persist under `/user/home` (jail). Cluster map: OnOff+Level → LIGHT, OnOff → SWITCH, Occupancy/IAS Zone → BINARY_SENSOR, Temperature → CLIMATE.
- **`home_*`** wraps the table: list, optimistic `home_cmd` (mock / seeded list succeeds; radio-down + not-mock reverts + `ERR_IO`), `home_set_meta`, callback, `auto_eval`. Persist is dirty + `home_poll` (not on the LVGL tap path).
- **UI:** replace the Home stub with Devices (default), Device, Network. Pair opens join for 60 s. Radio-down banner when SYS ping failed. Do not rebuild the list every tick (permit countdown is a live label). Automations editor stays Sprint 12.
- **Out:** MQTT, climate/scenes, extra IRQ, JPEG HW, DMA2D, real pairing as the only path.

- **Exit:** host tests for MT checksum, SYS ping stub, interview map, 32-device cap, cmd revert, persist jail; HIL: Home opens with mock list and “Radio not ready”, Pair countdown, toggle a light, reboot keeps names.

### Sprint 12 — Local automation + Home polish

Status: **done** (local rules on M7; no cloud / no MQTT).

`auto_*` already matches occupancy/on in RAM. This sprint makes rules do work: `home_cmd`, delay, persist, and an Automations screen. Ethernet is not required.

Lock:

- **Engine:** `auto_eval` is rising-edge and only queues work. `home_poll` / `auto_poll` apply due actions (not on the LVGL tap path). `AUTO_ACT_ON` + `delay_ms` means On now and Off after the delay (occupancy timeout, `TC-HOME-10`). TOGGLE/OFF honour delay as a single deferred cmd.
- **Table:** 32 rules in RAM. Persist `/user/home/rules.bin` (jail). Host RAM VFS is 512 B — persist a few rules, test the 32-cap in RAM only.
- **Seed:** one enabled rule, “Motion light”: Motion stair occupied → Living lamp On, Off after 3 s. So the board and host-sim show a real automation with no dongle.
- **Apps use `home_*` + thin `home_app_*`.** `src/app/home.c` still must not include `znp_mt.h`, `uart.h`, HAL, MQTT, or LwIP. UI never shows MT names.
- **UI:** Automations list (name, summary, ≥40 px enable). Tap opens a rule page (trigger, action, delay, delete). Add creates the occupancy template if missing. Device page shows IEEE, NWK, LQI, last-seen, and cluster labels. Do not rebuild the device list every tick.
- **Out:** MQTT, climate/scenes editor, extra IRQ, JPEG HW, DMA2D, cloud conditions.

- **Exit:** host tests for occupancy→light + 3 s Off, enable/persist, 32-rule cap; HIL: open Home → Automations, trip the mock motion (or a real sensor), lamp turns on then off, no Ethernet.

### Sprint 13 — Hardening

- Watchdogs on both cores, brown-out, FS remount, OOM UI.
- Settings app (brightness, volume, IP, Zigbee channel/permit-join default).
- HIL pack for the requirement matrix, including game soak and ZNP mock.
- Size budgets and coverage floors as in `CICD.md` (80% line / 60% branch on host-testable C).
- **Exit:** RTM P0/P1 rows green; 8-hour soak (UI + audio + explorer; game 30 min; ZNP mock or radio idle) without leak or deadlock; CI gates all green.

### Later — Zephyr port (not a product sprint yet)

- `osal/zephyr`, DTS for this board, OpenAMP transport, same apps.
- Track as a spike after Sprint 4 so the HAL stays honest.
- `uart_*` / `zb_host` / `auto_*` and game `gfx_*` must survive that spike without app changes.
- `vfs_*` stays a **FAT** volume. Do not retarget `/user` to littlefs.

### Later — USB MSC (not a product sprint yet)

Opt-in **USB file transfer** so a PC can copy photos/music onto the eMMC. Do **not** implement this until a dedicated sprint; Sprint 4+ product work is unchanged.

Lock:

- **Stack:** upstream **TinyUSB** device MSC. Not Cube `USB_Device`.
- **LUN:** the eMMC **FAT** volume (the same partition FatFs mounts). A PC cannot mount littlefs; that is one reason FAT stays.
- **Mode:** exclusive, started from Settings (“USB file transfer”). Not always-on.
- **While connected:** unmount FatFs, stop audio/decode/home flushes, idle the explorer. Never two writers on the same FAT.
- **On unplug:** remount FatFs, drop VFS caches, refresh `/user`.
- **Jail:** MSC exposes the **volume**, not the `/user` jail. The PC can see `/user/home` and can delete anything on that LUN. Document that in Settings copy.
- **Speed:** board USB is OTG FS (~0.8–1 MB/s practical). Convenience, not a fast pipe.
- **Power-loss on FAT:** for small records (`/user/home`, game saves) write `*.tmp` + rename + `f_sync` if needed. eMMC already has FTL; do not add littlefs wear-leveling on top.

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
| USB MSC vs FatFs writers | Exclusive mode: unmount FatFs while the PC owns the LUN |
| littlefs “safer writes” | Rejected: PC MSC needs FAT; eMMC has FTL; use tmp+rename on small records |
| M7 D-cache vs DMA/IPC | Non-cacheable SRAM4; cache API for DMA |
| TouchGFX samples leaking in | Ban TouchGFX in review; LVGL-only backend; no STM32CubeH7 monolith |
| Game in LVGL widgets | Keep `game_sim` + `gfx_*`; LVGL canvas is a backend, not the model |
| Home UI talking MT/MQTT | `home_*` only; `mock` so UI is not blocked on a dongle |
| USART3 stolen for ZNP | Console stays USART3; ZNP on USART1 |
| Permit join left open | UI countdown; auto-close; no join while locked |
| ZNP UART vs LVGL | DMA + worker; offload to M4 if needed |
| Host sim only on Linux | One SDL2 backend; CI links Ubuntu and Windows |
| Sim widgets drift from the board | Same `ui_lvgl.c` + apps/shell; only `lv_port` / `lv_conf` / VFS / tick differ |
| `host-tests` accidentally link LVGL | Keep presets separate; layering + CICD forbid LVGL in unit tests |
| Scope (MQTT export, climate, extra games, Wi-Fi) | C/P2; do not block Files, Brick, or Zigbee OnOff |

## 7. Deliverables per milestone

| Milestone | Demo |
| --- | --- |
| M1 (Sprint 2) | Touch paint on color bars |
| M2 (Sprint 5) | Browse eMMC and open files |
| M2b (Sprint 5b) | Same shell in an SDL window on Ubuntu and Windows |
| M3 (Sprint 6) | View JPEG + UTF-8 text |
| M4 (Sprint 8) | Play MP3 while browsing |
| M5 (Sprint 10) | Brick playable at ≥ 30 FPS |
| M6 (Sprint 11) | Form Zigbee net, show joined device, toggle OnOff |
| M7 (Sprint 12) | Local rule fires on the panel with no Ethernet |
| M8 (Sprint 13) | Full shell, soak |

## 8. Documentation map

| File | Contents |
| --- | --- |
| `Design_Review.md` | Why v1 changed |
| `Architecture.md` | Layers, memory, interfaces, boot/thread/Zigbee flows |
| `UI_Design.md` | Shell, screens, navigation and pairing flows |
| `Requirements_and_Test_Cases.md` | Shall statements + tests + RTM + CI reqs |
| `CICD.md` | GitHub Actions, static analysis, coverage gates |
| `Plan.md` | This file |
| `third_party/README.md` | CubeH7 vs our BSP vs component submodules |
