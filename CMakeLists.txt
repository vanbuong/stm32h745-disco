cmake_minimum_required(VERSION 3.22)
project(stm32h745-disco C)

set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

option(HOST_TESTS "Build host unit tests" OFF)
option(COVERAGE "Enable gcov on host tests" OFF)
set(CORE "" CACHE STRING "Firmware core: M7 or M4")

if(HOST_TESTS)
    include(${CMAKE_SOURCE_DIR}/cmake/HostTests.cmake)
elseif(CORE STREQUAL "M7" OR CORE STREQUAL "M4")
    include(${CMAKE_SOURCE_DIR}/cmake/Firmware.cmake)
else()
    message(FATAL_ERROR "Configure with -DHOST_TESTS=ON or -DCORE=M7|M4 and an ARM toolchain")
endif()
