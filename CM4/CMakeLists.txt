cmake_minimum_required(VERSION 3.22)

#
# Per-core CMake project (STM32CubeMX / STM32 VS Code dual-core layout).
# Firmware sources stay in ../firmware/; this folder is the Cortex-M4 context.
#

set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_C_EXTENSIONS ON)

if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE "Debug")
endif()

message("Build type: " ${CMAKE_BUILD_TYPE})

set(CMAKE_EXPORT_COMPILE_COMMANDS TRUE)

set(STM32_MCU_FLAGS "-mcpu=cortex-m4 -mfpu=fpv4-sp-d16 -mfloat-abi=hard")
set(STM32_LINKER_SCRIPT "../firmware/src/bsp/stm32h745i_disco/stm32h745_m4.ld")
set(STM32_LINKER_OPTION)

set(CMAKE_PROJECT_NAME stm32h745-disco_CM4)
project(${CMAKE_PROJECT_NAME})

set(CMAKE_CXX_COMPILER_FORCED "true")
enable_language(C ASM)

add_executable(${CMAKE_PROJECT_NAME})
include("mx-generated.cmake")
