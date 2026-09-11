# STM32H745I-DISCO BSP

Board: MB1381 with STM32H745XIH6. Pin mux and clocks live here; apps never include HAL.

## Ethernet vs QSPI bank 2

The LAN8740Ai PHY is **hardwired MII** (not RMII). PH2 and PH3 are muxed:

| Pin | QSPI | ETH MII |
| --- | --- | --- |
| PH2 | BK2 D0 | CRS |
| PH3 | BK2 D1 | COL |

UM2488 default (this firmware):

- **SB3 and SB4 OFF, R38 and R40 ON** — PH2/PH3 stay on QSPI bank 2.
- Ethernet runs **MII 100 Mbit/s full-duplex** without CRS/COL.
- 10 Mbit/s **half-duplex** needs SB3/SB4 ON and R38/R40 removed. That configuration is **unsupported**.

QSPI init still muxes BK2 GPIOs (including PG9/PG14) but uses `QSPI_FLASH_ID_1` / `QSPI_DUALFLASH_DISABLE` for the Sprint 1 mmap smoke. Do not execute blank NOR (`0xFF`).

ETH DMA descriptors, Rx/Tx bounce, and the LwIP heap live in **SRAM3** (`0x30040000`, 32 KB, MPU non-cacheable). ETH is brought up on **M7 only**.

## RTC

LSE 32.768 kHz when the crystal starts; LSI otherwise. Status-bar clock is `time_rtc_get` (NTP is not required).
