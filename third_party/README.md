# ST MCU components (fetched at CMake configure)

CMake pulls matching CubeH7 1.13.0 parts into this directory:

| Path | Repo | Tag |
| --- | --- | --- |
| `stm32h7xx-hal-driver/` | [stm32h7xx-hal-driver](https://github.com/STMicroelectronics/stm32h7xx-hal-driver) | v1.11.6 |
| `cmsis-device-h7/` | [cmsis-device-h7](https://github.com/STMicroelectronics/cmsis-device-h7) | v1.10.7 |
| `cmsis_core/` | [cmsis_core](https://github.com/STMicroelectronics/cmsis_core) | v5.9.0_20220705 |

Not committed. The BSP talks to these only through `firmware/src/port/cube` and `firmware/src/bsp`.
