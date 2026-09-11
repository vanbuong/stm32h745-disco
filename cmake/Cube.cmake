include(FetchContent)

set(ST_ROOT ${CMAKE_SOURCE_DIR}/third_party)
set(ST_HAL_DIR ${ST_ROOT}/stm32h7xx-hal-driver)
set(ST_CMSIS_DEV ${ST_ROOT}/cmsis-device-h7)
set(ST_CMSIS_CORE ${ST_ROOT}/cmsis_core)

function(st_fetch name repo tag dest)
    if(EXISTS ${dest})
        return()
    endif()
    message(STATUS "Fetching ${name} ${tag}")
        FetchContent_Declare(${name}
        GIT_REPOSITORY ${repo}
        GIT_TAG ${tag}
        GIT_SHALLOW TRUE
        GIT_SUBMODULES ""
        SOURCE_DIR ${dest}
    )
    FetchContent_GetProperties(${name})
    if(NOT ${name}_POPULATED)
        FetchContent_Populate(${name})
    endif()
endfunction()

st_fetch(stm32h7xx_hal
    https://github.com/STMicroelectronics/stm32h7xx-hal-driver.git
    v1.11.6
    ${ST_HAL_DIR})
st_fetch(cmsis_device_h7
    https://github.com/STMicroelectronics/cmsis-device-h7.git
    v1.10.7
    ${ST_CMSIS_DEV})
st_fetch(cmsis_core
    https://github.com/STMicroelectronics/cmsis_core.git
    v5.9.0_20220705
    ${ST_CMSIS_CORE})

set(CUBE ${CMAKE_SOURCE_DIR}/firmware/src/port/cube)
set(ST_VENDOR_SRC
    ${ST_CMSIS_DEV}/Source/Templates/system_stm32h7xx.c
    ${ST_HAL_DIR}/Src/stm32h7xx_hal.c
    ${ST_HAL_DIR}/Src/stm32h7xx_hal_cortex.c
    ${ST_HAL_DIR}/Src/stm32h7xx_hal_rcc.c
    ${ST_HAL_DIR}/Src/stm32h7xx_hal_rcc_ex.c
    ${ST_HAL_DIR}/Src/stm32h7xx_hal_flash.c
    ${ST_HAL_DIR}/Src/stm32h7xx_hal_flash_ex.c
    ${ST_HAL_DIR}/Src/stm32h7xx_hal_gpio.c
    ${ST_HAL_DIR}/Src/stm32h7xx_hal_hsem.c
    ${ST_HAL_DIR}/Src/stm32h7xx_hal_dma.c
    ${ST_HAL_DIR}/Src/stm32h7xx_hal_mdma.c
    ${ST_HAL_DIR}/Src/stm32h7xx_hal_pwr.c
    ${ST_HAL_DIR}/Src/stm32h7xx_hal_pwr_ex.c
)
if(CORE STREQUAL "M7")
    list(APPEND ST_VENDOR_SRC
        ${ST_HAL_DIR}/Src/stm32h7xx_hal_qspi.c
        ${ST_HAL_DIR}/Src/stm32h7xx_hal_sdram.c
        ${ST_HAL_DIR}/Src/stm32h7xx_ll_fmc.c
        ${ST_HAL_DIR}/Src/stm32h7xx_ll_usart.c
        ${ST_HAL_DIR}/Src/stm32h7xx_ll_gpio.c
        ${ST_HAL_DIR}/Src/stm32h7xx_ll_rcc.c
    )
endif()
set_source_files_properties(${ST_VENDOR_SRC} PROPERTIES COMPILE_FLAGS "-w")
set(HAL_SRC ${ST_VENDOR_SRC} ${CUBE}/cube.c ${CUBE}/cube_it.c)
