# CubeMX / STM32 VS Code multi-context layout (dual-core H745).
# The configuration tool looks for ST_MULTICONTEXT and per-core CMakeLists.

if(NOT DEFINED H745_ROOT)
    get_filename_component(H745_ROOT "${CMAKE_CURRENT_LIST_DIR}/.." ABSOLUTE)
endif()

if(NOT CORE STREQUAL "" AND NOT CORE STREQUAL "BOTH" AND NOT CORE STREQUAL "M7" AND NOT CORE STREQUAL "M4")
    message(FATAL_ERROR "Configure with -DHOST_TESTS=ON, -DHOST_SIM=ON, or -DCORE=M7|M4|BOTH")
endif()

set(ST_MULTICONTEXT DUAL_CORE CACHE STRING "Type of multi-context")
set(BUILD_CONTEXT "" CACHE STRING "Cube build context: empty, CM7, CM4, or CM7+CM4")

set(_build_cm7 TRUE)
set(_build_cm4 TRUE)
if(CORE STREQUAL "M7")
    set(_build_cm4 FALSE)
elseif(CORE STREQUAL "M4")
    set(_build_cm7 FALSE)
endif()
if(DEFINED BUILD_CONTEXT AND NOT BUILD_CONTEXT STREQUAL "")
    if(NOT BUILD_CONTEXT MATCHES "CM7")
        set(_build_cm7 FALSE)
    endif()
    if(NOT BUILD_CONTEXT MATCHES "CM4")
        set(_build_cm4 FALSE)
    endif()
endif()

if(_build_cm7)
    message(STATUS "Build context: CM7")
    stm32_add_firmware(M7)
    set(ST_DUAL_CORE_CM7_PROJECT_BUILD_TARGET
        ${CMAKE_BINARY_DIR}/firmware-m7.elf
        CACHE FILEPATH "Path to CM7 project target" FORCE)
endif()
if(_build_cm4)
    message(STATUS "Build context: CM4")
    stm32_add_firmware(M4)
    set(ST_DUAL_CORE_CM4_PROJECT_BUILD_TARGET
        ${CMAKE_BINARY_DIR}/firmware-m4.elf
        CACHE FILEPATH "Path to CM4 project target" FORCE)
endif()

if(_build_cm7 AND _build_cm4)
    add_custom_target(firmware ALL DEPENDS firmware-m7 firmware-m4)
endif()
