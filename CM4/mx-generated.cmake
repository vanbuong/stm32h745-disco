# Cortex-M4 sources (our firmware, not CubeMX-generated HAL lists).
include("${CMAKE_CURRENT_LIST_DIR}/../cmake/Firmware.cmake")
stm32_add_firmware(M4)
