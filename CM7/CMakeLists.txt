cmake_minimum_required(VERSION 3.22)

# STM32 VS Code maps this folder to Cortex-M7. Sources stay under firmware/.
if(CMAKE_SOURCE_DIR STREQUAL CMAKE_CURRENT_SOURCE_DIR)
    project(firmware-m7 C)
endif()

include("${CMAKE_CURRENT_LIST_DIR}/../cmake/Firmware.cmake")
stm32_add_firmware(M7)
