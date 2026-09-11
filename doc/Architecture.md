# Software Architecture

Portable HMI architecture for the STM32H745I-DISCO. Application code must not call FreeRTOS, Zephyr, LVGL, TouchGFX, FatFS, or STM32 HAL directly.

## 1. Goals

- Ship a usable dual-core HMI: launcher, file explorer, image viewer, text viewer, audio playback, and network status.
- Keep OS, UI toolkit, filesystem, and board drivers behind stable C interfaces.
- Allow a later move to **Zephyr + LVGL** without rewriting apps.
- Keep Cortex-M4 work real-time (audio DMA, optional network) and Cortex-M7 work latency-tolerant (UI, FS, decode).

## 2. Hardware baseline

| Item | Value |
| --- | --- |
| MCU | STM32H745XIH6 (M7 480 MHz, M4 240 MHz) |
| Display | 4.3" 480×272 RGB (RK043FN48H), FT5336 touch on I2C4 |
| Graphics | LTDC + DMA2D (Chrom-ART) + JPEG codec |
| SDRAM | `0xD0000000`, 8 MB usable (16-bit FMC) |
| QSPI NOR | `0x90000000`, dual 512 Mbit, memory-mapped XiP |
| eMMC | 4 GB, SDMMC1 |
| Audio | WM8994 on SAI, I2C4 shared with touch |
| Ethernet | LAN8740A MII; default pins collide with QSPI bank 2 |
| Wi-Fi | **Not on board.** Optional ESP32 on Arduino/STMod+ |
| Shared SRAM | SRAM4 64 KB at `0x38000000` (D3 domain) |

## 3. Layered design

```
┌─────────────────────────────────────────────────────────────┐
│  Apps (no toolkit/OS types)                                 │
│  launcher, files, image, text, player, settings [, game]    │
├─────────────────────────────────────────────────────────────┤
│  Shell: navigation stack, status model, app registry        │
├─────────────────────────────────────────────────────────────┤
│  ui_backend  │  services                                    │
│  (LVGL now)  │  vfs, media, audio, net, time, settings      │
├──────────────┴──────────────────────────────────────────────┤
│  OSAL   IPC protocol   disp/input HAL   media decode HAL    │
├─────────────────────────────────────────────────────────────┤
│  Ports: FreeRTOS + STM32Cube  →  later Zephyr + device tree │
│  BSP: clocks, MPU, cache, LTDC, SDMMC, SAI, ETH, QSPI       │
└─────────────────────────────────────────────────────────────┘
```

Rules:

1. Headers under `include/app` and `include/svc` may include only C11, project types, and OSAL.
2. LVGL (or any toolkit) lives only in `src/ui/backend_*`.
3. STM32 HAL / Zephyr DT live only in `src/port/*` and `src/bsp/*`.
4. Dual-core messages are structs with explicit endianness and version, never pointers to M7-only memory.

## 4. Recommended source tree

```
firmware/
  include/
    osal/          osal.h
    ipc/           ipc.h, ipc_msg.h
    hal/           disp.h, input.h, audio_out.h, net_if.h
    svc/           vfs.h, media.h, audio.h, net.h, time.h
    ui/            shell.h, nav.h, theme.h
    app/           apps.h
  src/
    app/           launcher, files, image, text, player, settings
    shell/
    svc/
    ui/backend_lvgl/
    ipc/
    osal/freertos/           later: osal/zephyr/
    bsp/stm32h745i_disco/
    port/cube/               later: port/zephyr/
  tests/
    host/          PC unit tests, no HAL
    hil/           on-target scripts and fixtures
```

Two firmware images: `m7` and `m4`. They share only `include/ipc`.

## 5. Core split

```
          Cortex-M7 (UI / FS / decode)         Cortex-M4 (real-time IO)
          ----------------------------         -------------------------
          LVGL + shell + apps                  SAI DMA ping-pong
          VFS / eMMC / QSPI assets             MP3/WAV decode
          JPEG / PNG / text paging             Optional LwIP or ESP32 AT
          IPC client                           IPC server
          Display + touch                      No framebuffer access
```

Bring-up order: **M7-only** until display, storage, and shell work. Enable M4 when audio or offloaded net is needed. Do not put FatFS or LVGL on M4.

If Zephyr is adopted, keep the same split: M7 `stm32h745i_disco/stm32h745xx/m7`, M4 `.../m4`, IPC via HSEM mailbox then OpenAMP.

## 6. Memory map

### 6.1 Internal

| Region | Address | Size | Owner | Notes |
| --- | --- | --- | --- | --- |
| ITCM | `0x00000000` | 64 KB | M7 | Hot UI/decode routines |
| Flash bank 1 | `0x08000000` | 1 MB | M7 | Default boot |
| Flash bank 2 | `0x08100000` | 1 MB | M4 | Default CM4 boot |
| DTCM | `0x20000000` | 128 KB | M7 | Stacks, hard-RT data |
| AXI SRAM | `0x24000000` | 512 KB | M7 | LVGL working set, FS cache |
| SRAM1 | `0x30000000` | 128 KB | M4 | M4 .data/.bss/heap |
| SRAM2 | `0x30020000` | 128 KB | M4 / DMA | Audio PCM rings if not in SRAM3 |
| SRAM3 | `0x30040000` | 32 KB | DMA | Small DMA pools |
| SRAM4 | `0x38000000` | 64 KB | Shared | IPC only, non-cacheable |
| Backup SRAM | `0x38800000` | 4 KB | Shared | Boot reason, net config flags |

### 6.2 External SDRAM (`0xD0000000`, 8 MB)

RGB565 is the default pixel format (480×272×2 = 261 120 bytes per full buffer).

| Offset | Size | Use |
| --- | --- | --- |
| `+0x000000` | 261 KB | LTDC framebuffer 0 |
| `+0x040000` | 261 KB | LTDC framebuffer 1 (double buffer) |
| `+0x080000` | 261 KB | LVGL draw buffer (or DMA2D staging) |
| `+0x0C0000` | ~2 MB | Image decode / scaler working buffer |
| `+0x2C0000` | remainder | File cache, text window, heap fallback |

ARGB8888 is allowed later if color quality requires it; each full buffer then costs ~522 KB. Do not put M4 code/data in SDRAM.

### 6.3 QSPI and eMMC roles

| Media | Role | Mount / map |
| --- | --- | --- |
| QSPI NOR | Fonts, icons, firmware assets, optional XiP | Memory-mapped, read-only in production |
| eMMC | User files, albums, logs, optional OTA scratch | VFS mount `/user`, `/log` |
| Internal flash | Bootloaders, both core images | Not exposed in the explorer |

### 6.4 MPU and cache

- SRAM4: MPU **Device or Normal non-cacheable**, shared, no execute.
- DMA audio buffers: aligned, MPU non-cacheable **or** explicit clean/invalidate in `bsp_cache_*`.
- SDRAM framebuffers: write-through or normal + clean before LTDC flip.
- Never share a cached M7 buffer with M4 without a cache API call.

## 7. Public interfaces (stable)

Keep these headers OS- and toolkit-free.

### 7.1 OSAL

Threads, mutexes, recursive mutexes, semaphores, queues, timers, sleep, millis, heap. Timeouts in milliseconds. Fatal errors go to `osal_panic()` (log + reset policy).

First port: FreeRTOS. Second port: Zephyr. CMSIS-RTOS2 is acceptable as an intermediate, but apps still call `osal_*`.

### 7.2 Display and input

```c
typedef struct { uint16_t w, h, stride; disp_fmt_t fmt; } disp_info_t;
void disp_flush(const disp_rect_t *r, const void *pixels);
void disp_blit_dma2d(...);   /* optional accelerator */

typedef enum { INPUT_PTR_DOWN, INPUT_PTR_MOVE, INPUT_PTR_UP, INPUT_KEY, INPUT_BTN } input_kind_t;
bool input_poll(input_event_t *out);
```

LVGL `flush_cb` and `indev_read_cb` are adapters over these two calls. Zephyr `display_write` / `input` subsystems replace the BSP, not the apps.

### 7.3 VFS

POSIX-like subset: `open/read/write/close/seek/stat/opendir/readdir/mkdir/unlink/rename`. Paths are UTF-8, `/` separated, rooted at a jail (`/user` for the explorer).

No raw `FIL` / `fs_file_t` in apps.

### 7.4 Media

```c
media_kind_t media_probe(const char *path, media_info_t *info);
int media_decode_image(const char *path, image_buf_t *out, const image_req_t *req);
int media_open_audio(const char *path, audio_stream_t *s);
```

JPEG uses the STM32 JPEG codec when present. PNG/BMP are software. Failures return codes, never abort the UI.

### 7.5 IPC

Transport and protocol are separate.

**Protocol** (`ipc_msg.h`): versioned header `{magic, ver, src, dst, type, flags, seq, len}` + payload. Endpoints: `SYS`, `AUDIO`, `NET`, `LOG`. No pointers in payloads.

**Transport now:** two lockless rings in SRAM4 + HSEM notify.

**Transport later:** OpenAMP / RPMsg over the same SRAM4 carve-out. Apps keep calling `ipc_send` / `ipc_recv`.

SRAM4 sketch (64 KB):

| Offset | Size | Use |
| --- | --- | --- |
| 0 | 256 | Control: magic, versions, head/tail, ready flags |
| 256 | 16 KB | M7→M4 ring |
| 16640 | 16 KB | M4→M7 ring |
| 33024 | rest | Reserved (future RPMsg vring) |

Latency budget: command round-trip **< 2 ms** for control messages. Audio PCM does not ride this ring; it uses a dedicated DMA buffer.

## 8. UI architecture

Apps implement a small contract:

```c
typedef struct {
    const char *id;          /* "files" */
    const char *title;
    const void *icon;        /* id, not an LVGL image */
    void (*on_start)(void *args);
    void (*on_stop)(void);
    void (*on_tick)(uint32_t dt_ms);
    void (*on_event)(const ui_event_t *e);
} ui_app_t;
```

Shell owns:

- Screen stack (`push` / `pop` / `replace`).
- Status model (time, net, storage, audio).
- Modal errors and toasts.
- Theme tokens (colors, font sizes, hit-target min 40 px).

The LVGL backend draws those models. A future backend can draw the same models. **Do not** call `lv_*` from `src/app`.

See `UI_Design.md` for layout and screens.

## 9. Services

| Service | Core | Responsibility |
| --- | --- | --- |
| `vfs` | M7 | Mounts, jail, extension → app dispatch |
| `media` | M7 | Probe and decode images; open audio files |
| `audio` | M7 API, M4 engine | Play/pause/seek, volume, now-playing |
| `net` | One core only | Link state, IPv4, optional failover |
| `time` | M7 | RTC display; NTP is P2 |
| `settings` | M7 | Key/value in eMMC or backup SRAM |

Audio path: M7 sends `{play path | pause | volume}` over IPC. M4 decodes and feeds SAI DMA. M7 never blocks the UI thread on decode.

Network ownership: pick **one** core at build time (default M4 if audio+net isolation is wanted, M7 if the first Zephyr port should be single-core-simple). Never initialize ETH on both.

## 10. Pin and bus constraints

- **I2C4:** FT5336 + WM8994. BSP provides a mutex; no driver talks to I2C4 directly.
- **Ethernet vs QSPI bank 2:** document the chosen solder-bridge map in the BSP README. Prefer QSPI dual-flash for XiP unless Ethernet full-duplex + CRS/COL is required.
- **LTDC pixel clock** and SDRAM bandwidth: RGB565 double-buffer + DMA2D is the safe default at 480×272.

## 11. Build, log, test

- CMake presets: `m7-debug`, `m4-debug`, `host-tests`.
- Logs: UART3 115200 8N1, tagged `core,lvl,mod,msg`. No `printf` to ITM as the only log.
- Host tests compile `svc` + `ipc` protocol with a POSIX OSAL stub.
- HIL tests run on the Discovery board via VCP.

## 12. Later Zephyr + LVGL migration

If LVGL is already the backend, the remaining work is a **port swap**, not an app rewrite:

1. Replace `osal/freertos` with `osal/zephyr`.
2. Replace `port/cube` + Cube clock init with Zephyr DTS (`stm32h745i_disco`).
3. Map `disp_*` to Zephyr display, `input_*` to FT5336 input, `vfs_*` to Zephyr FS.
4. Map `ipc` transport to `ipm` / OpenAMP; keep `ipc_msg.h`.
5. Keep `src/app` and `src/shell` unchanged except Kconfig feature flags.

Do not introduce TouchGFX. Do not scatter `#ifdef ZEPHYR` inside apps.
