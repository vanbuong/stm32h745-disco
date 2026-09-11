# STM32H745-DISCO Development Plan

## Phase 1: System Design & Memory Mapping
* Establish core boundaries for Cortex-M7 (480 MHz) and Cortex-M4 (240 MHz).
* Configure Dual QSPI Flash in Memory-Mapped Mode (`0x90000000`) for M7 Execution-in-Place (XiP).
* Configure eMMC via 8-bit/4-bit SDMMC1 with FatFS for bulk data storage.
* Configure External SDRAM (`0xD0000000`) for the LTDC graphics frame buffers and dynamic caches.
* Set up Shared SRAM4 Memory (`0x38000000`) with Hardware Semaphores (HSEM) for inter-core communication.

## Phase 2: Core Responsibilities & Peripherals
* **Cortex-M7:** Runs FreeRTOS. Manages the TouchGFX/LVGL UI framework, processes the local game loop, and handles the eMMC file systems.
* **Cortex-M4:** Runs FreeRTOS or Bare-Metal. Manages the LwIP Ethernet stack, ESP32 Wi-Fi bridge communications, and executes the Helix MP3 software decoder library.

## Phase 3: Software Implementation Iterations (Sprints)
* **Sprint 1: Inter-Core & Memory Foundations** -> Initialize memory maps, verify D-Cache synchronization routines, and launch the lockless IPC ring buffers over SRAM4.
* **Sprint 4: Network & Audio Pipelines** -> Interface the LwIP stack (Ethernet) alongside the ESP32 driver (Wi-Fi failover). Build the SAI2 DMA audio engine paired with the Helix MP3 decoder.
* **Sprint 5: UI Integration & Game Loop** -> Bind TouchGFX dynamic asset loading hooks to the eMMC storage layers and connect the M4 network states to the M7 UI dashboard.