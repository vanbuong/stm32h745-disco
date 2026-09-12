cmake_minimum_required(VERSION 3.22)
include(ExternalProject)

# Host tests/sim stay at the repo root. Firmware follows CubeMX blinky:
# CMAKE_CXX_COMPILER_FORCED + project() with no languages so the STM32 VS Code
# superbuild configures without a CLI arm-none-eabi-gcc. CubeCLT is on PATH
# only inside the extension.

option(HOST_TESTS "Build host unit tests" OFF)
option(HOST_SIM "Build host LVGL SDL simulator" OFF)
option(COVERAGE "Enable gcov on host tests" OFF)
set(CORE "" CACHE STRING "Firmware core: M7, M4, BOTH, or empty for both")

if(HOST_TESTS AND HOST_SIM)
    message(FATAL_ERROR "HOST_TESTS and HOST_SIM are mutually exclusive")
endif()

if(CORE AND NOT CORE STREQUAL "M7" AND NOT CORE STREQUAL "M4" AND NOT CORE STREQUAL "BOTH")
    message(FATAL_ERROR "CORE must be M7, M4, BOTH, or empty")
endif()

set(CMAKE_PROJECT_NAME stm32h745-disco)

if(HOST_TESTS)
    project(${CMAKE_PROJECT_NAME} C)
    set(CMAKE_C_STANDARD 11)
    set(CMAKE_C_STANDARD_REQUIRED ON)
    set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
    include(${CMAKE_SOURCE_DIR}/cmake/HostTests.cmake)
elseif(HOST_SIM)
    project(${CMAKE_PROJECT_NAME} C)
    set(CMAKE_C_STANDARD 11)
    set(CMAKE_C_STANDARD_REQUIRED ON)
    set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
    include(${CMAKE_SOURCE_DIR}/cmake/HostSim.cmake)
else()
    set(CMAKE_CXX_COMPILER_FORCED "true")
    project(${CMAKE_PROJECT_NAME} LANGUAGES NONE)
    include("mx-generated.cmake")
endif()
