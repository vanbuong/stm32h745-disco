# Dual-core superbuild in the STM32CubeMX layout (see vanbuong/stm32h745-blinky).
# Each core is a separate CMake project under CM7/ and CM4/.
include(ExternalProject)

set(ST_MULTICONTEXT DUAL_CORE CACHE STRING "Type of multi-context")
if(NOT CMAKE_EXECUTABLE_SUFFIX_CXX)
    set(CMAKE_EXECUTABLE_SUFFIX_CXX ".elf")
endif()

# Map our CORE cache (m7-debug / m4-debug) onto Cube's BUILD_CONTEXT.
# Do not CACHE an empty BUILD_CONTEXT — that makes DEFINED true and neither
# core would build (Cube's gate is `NOT DEFINED BUILD_CONTEXT`).
if(NOT DEFINED BUILD_CONTEXT)
    if(CORE STREQUAL "M7")
        set(BUILD_CONTEXT "CM7")
    elseif(CORE STREQUAL "M4")
        set(BUILD_CONTEXT "CM4")
    endif()
endif()

#-----------------------Build CM4 Project-----------------------#
if(("${BUILD_CONTEXT}" MATCHES "CM4") OR (NOT DEFINED BUILD_CONTEXT))
    message("   Build context: " CM4)
    ExternalProject_Add(stm32h745-disco_CM4
        BINARY_DIR                  ${CMAKE_SOURCE_DIR}/CM4/build
        SOURCE_DIR                  ${PROJECT_SOURCE_DIR}/CM4
        PREFIX                      CM4
        CONFIGURE_HANDLED_BY_BUILD  true
        INSTALL_COMMAND             ""
        CMAKE_ARGS                  -DCMAKE_EXPORT_COMPILE_COMMANDS=ON  -DCMAKE_TOOLCHAIN_FILE=${CMAKE_TOOLCHAIN_FILE}  -DCMAKE_BUILD_TYPE=${CMAKE_BUILD_TYPE}
        BUILD_ALWAYS                true
    )
    set_property(DIRECTORY APPEND PROPERTY ADDITIONAL_CLEAN_FILES "${CMAKE_SOURCE_DIR}/CM4/build")
    set(ST_DUAL_CORE_CM4_PROJECT_BUILD_TARGET
        ${CMAKE_SOURCE_DIR}/CM4/build/stm32h745-disco_CM4${CMAKE_EXECUTABLE_SUFFIX_CXX}
        CACHE FILEPATH "Path to cm4 project target")
    # STM32 VS Code may invoke the child executable name at the superbuild.
    add_custom_target(firmware-m4 DEPENDS stm32h745-disco_CM4)
endif()

#-----------------------Build CM7 Project-----------------------#
if(("${BUILD_CONTEXT}" MATCHES "CM7") OR (NOT DEFINED BUILD_CONTEXT))
    message("   Build context: " CM7)
    ExternalProject_Add(stm32h745-disco_CM7
        BINARY_DIR                  ${CMAKE_SOURCE_DIR}/CM7/build
        SOURCE_DIR                  ${PROJECT_SOURCE_DIR}/CM7
        PREFIX                      CM7
        CONFIGURE_HANDLED_BY_BUILD  true
        INSTALL_COMMAND             ""
        CMAKE_ARGS                  -DCMAKE_EXPORT_COMPILE_COMMANDS=ON  -DCMAKE_TOOLCHAIN_FILE=${CMAKE_TOOLCHAIN_FILE}  -DCMAKE_BUILD_TYPE=${CMAKE_BUILD_TYPE}
        BUILD_ALWAYS                true
    )
    set_property(DIRECTORY APPEND PROPERTY ADDITIONAL_CLEAN_FILES "${CMAKE_SOURCE_DIR}/CM7/build")
    set(ST_DUAL_CORE_CM7_PROJECT_BUILD_TARGET
        ${CMAKE_SOURCE_DIR}/CM7/build/stm32h745-disco_CM7${CMAKE_EXECUTABLE_SUFFIX_CXX}
        CACHE FILEPATH "Path to cm7 project target")
    add_custom_target(firmware-m7 DEPENDS stm32h745-disco_CM7)
endif()
