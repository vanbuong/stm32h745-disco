# User Interface Design Specification

## 1. Global Navigation Layout
* **Screen Area:** Mapped to the integrated 4.3" 480x272 LCD touch screen.
* **Left Dock (50x272 px):** A persistent navigation panel housing touch targets to instantly swap the active canvas view.
* **Top Status Bar (430x30 px):** Displays real-time parameters from the M4 (NTP system time, Ethernet/Wi-Fi states, and audio playback strings).
* **Active Screen Region (430x242 px):** The target zone dedicated to the loaded application screen.

## 2. Dynamic Asset Management
* **TouchGFX Frame Buffers:** Allocated cleanly within the 16 MB External SDRAM pool (`0xD0000000`) with Double Buffering managed by the Chrom-ART (DMA2D) graphics accelerator.
* **Dynamic Bitmaps:** Massive full-bleed screen backdrops or album art files are kept as raw binary blocks inside the 4 GB eMMC. When a screen initializes (`setupScreen()`), the M7 streams the target binary from the eMMC directly into the pre-allocated SDRAM graphics pointer, clearing it entirely during `tearDownScreen()` to avoid runtime memory leaks.