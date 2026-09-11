# ST MCU components (git submodules)

These are the CubeH7 1.13.0 parts used by `firmware/src/port/cube` and the Discovery BSP. Apps never include them.

| Path | Upstream | Tag |
| --- | --- | --- |
| `stm32h7xx-hal-driver/` | [STMicroelectronics/stm32h7xx-hal-driver](https://github.com/STMicroelectronics/stm32h7xx-hal-driver) | v1.11.6 |
| `cmsis-device-h7/` | [STMicroelectronics/cmsis-device-h7](https://github.com/STMicroelectronics/cmsis-device-h7) | v1.10.7 |
| `cmsis_core/` | [STMicroelectronics/cmsis_core](https://github.com/STMicroelectronics/cmsis_core) | v5.9.0_20220705 |

`stm32h7xx-hal-driver` is both the high-level HAL (`stm32h7xx_hal_*.c`) and the low-level driver (`stm32h7xx_ll_*.c` / `USE_FULL_LL_DRIVER`).

```
git submodule update --init --recursive
```

| Core | HAL | LL |
| --- | --- | --- |
| M7 | RCC, PWR, Cortex/MPU, GPIO AF, SDRAM, QSPI | FMC, USART3, GPIO (LED) |
| M4 | none | GPIO (LED) |
