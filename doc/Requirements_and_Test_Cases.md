# System Requirements & Test Case Matrix

## 1. Storage & Media Layer
* **REQ-STG-01:** The system shall support mounting a FatFS filesystem on the 4 GB internal eMMC via an SDMMC peripheral interface.
* **REQ-STG-02:** The system shall read binary media chunks from the eMMC at a minimum sequential transfer rate of 15 MB/s.
* **TC-STG-01 (Local PC Unit Test):** Verify the file extension dispatcher logic matches `.mp3` and `.wav` formats to their target callbacks correctly without hitting real hardware drivers.
* **TC-STG-02 (System HIL Test):** Profile sequential eMMC read speeds using hardware timer tracking during continuous asset transfers.

## 2. Inter-Core Communication (IPC)
* **REQ-IPC-01:** Core communication must use a lockless circular ring-buffer topology isolated inside Shared SRAM4 Memory (`0x38000000`).
* **REQ-IPC-02:** Message latency over the shared data link layer must not exceed 2 milliseconds.
* **TC-IPC-01 (Local PC Unit Test):** Validate that the circular queue logic handles boundary overflows gracefully without corrupting adjacent memory addresses.
* **TC-IPC-02 (System HIL Test):** Confirm M7 intercepts the HSEM interrupt vector and decodes M4 payload variables within the required < 2 ms window.

## 3. Audio & Network Automation
* **REQ-AUD-01:** The system shall stream raw 16-bit PCM stereo data via the SAI2 DMA configuration configured as a continuous ping-pong ring buffer.
* **REQ-NET-01:** The network layer must feature automatic hot-swapping failover logic to redirect packets from Ethernet (LwIP) to Wi-Fi (ESP32) if the primary link goes down.
* **TC-AUD-01 (Local PC Unit Test):** Assert the stability of the audio mixer's mathematical clipping thresholds under extreme combined waveforms.
* **TC-NET-01 (System HIL Test):** Physically disconnect the RJ45 Ethernet link under active telemetry load and verify that the M4 network layer restores communication over the ESP32 within a predefined timeout.