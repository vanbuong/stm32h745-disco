# STMicroelectronics CubeH7 1.13.0 set, as git submodules:
#   third_party/stm32h7xx-hal-driver  HAL + LL  (v1.11.6)
#   third_party/cmsis-device-h7       device headers + system_stm32h7xx.c
#   third_party/cmsis_core            CMSIS-Core
#
# Clone with --recurse-submodules, or:
#   git submodule update --init --recursive

if(NOT DEFINED H745_ROOT)
    get_filename_component(H745_ROOT "${CMAKE_CURRENT_LIST_DIR}/.." ABSOLUTE)
endif()
set(ST_ROOT ${H745_ROOT}/third_party)
set(ST_HAL_DIR ${ST_ROOT}/stm32h7xx-hal-driver)
set(ST_CMSIS_DEV ${ST_ROOT}/cmsis-device-h7)
set(ST_CMSIS_CORE ${ST_ROOT}/cmsis_core)
set(CUBE ${H745_ROOT}/firmware/src/port/cube)

if(NOT EXISTS ${ST_HAL_DIR}/Inc/stm32h7xx_hal.h)
    message(FATAL_ERROR
        "Missing ST HAL/LL at ${ST_HAL_DIR} "
        "(https://github.com/STMicroelectronics/stm32h7xx-hal-driver).\n"
        "Run: git submodule update --init --recursive")
endif()
if(NOT EXISTS ${ST_CMSIS_DEV}/Include/stm32h745xx.h)
    message(FATAL_ERROR
        "Missing cmsis-device-h7 at ${ST_CMSIS_DEV}.\n"
        "Run: git submodule update --init --recursive")
endif()
if(NOT EXISTS ${ST_CMSIS_CORE}/Core/Include/core_cm7.h)
    message(FATAL_ERROR
        "Missing cmsis_core at ${ST_CMSIS_CORE}.\n"
        "Run: git submodule update --init --recursive")
endif()

function(cube_collect_sources CORE_ID out_var)
    if(CORE_ID STREQUAL "M7")
        set(ST_CMSIS_SRC
            ${ST_CMSIS_DEV}/Source/Templates/system_stm32h7xx.c
        )
    else()
        set(ST_CMSIS_SRC
            ${H745_ROOT}/firmware/src/bsp/stm32h745i_disco/system_cm4.c
        )
    endif()

    set(ST_HAL_SRC
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

    set(ST_LL_SRC
        ${ST_HAL_DIR}/Src/stm32h7xx_ll_gpio.c
        ${ST_HAL_DIR}/Src/stm32h7xx_ll_rcc.c
    )

    if(CORE_ID STREQUAL "M7")
        list(APPEND ST_HAL_SRC
            ${ST_HAL_DIR}/Src/stm32h7xx_hal_qspi.c
            ${ST_HAL_DIR}/Src/stm32h7xx_hal_sdram.c
            ${ST_HAL_DIR}/Src/stm32h7xx_hal_ltdc.c
            ${ST_HAL_DIR}/Src/stm32h7xx_hal_dma2d.c
            ${ST_HAL_DIR}/Src/stm32h7xx_hal_i2c.c
            ${ST_HAL_DIR}/Src/stm32h7xx_hal_i2c_ex.c
            ${ST_HAL_DIR}/Src/stm32h7xx_hal_mmc.c
            ${ST_HAL_DIR}/Src/stm32h7xx_hal_eth.c
            ${ST_HAL_DIR}/Src/stm32h7xx_hal_eth_ex.c
            ${ST_HAL_DIR}/Src/stm32h7xx_hal_rtc.c
            ${ST_HAL_DIR}/Src/stm32h7xx_hal_rtc_ex.c
        )
        list(APPEND ST_LL_SRC
            ${ST_HAL_DIR}/Src/stm32h7xx_ll_fmc.c
            ${ST_HAL_DIR}/Src/stm32h7xx_ll_usart.c
            ${ST_HAL_DIR}/Src/stm32h7xx_ll_sdmmc.c
        )
        set(src
            ${ST_CMSIS_SRC}
            ${ST_HAL_SRC}
            ${ST_LL_SRC}
            ${CUBE}/cube.c
            ${CUBE}/cube_it.c
        )
    else()
        list(APPEND ST_HAL_SRC
            ${ST_HAL_DIR}/Src/stm32h7xx_hal_sai.c
            ${ST_HAL_DIR}/Src/stm32h7xx_hal_sai_ex.c
        )
        set(src
            ${ST_CMSIS_SRC}
            ${ST_HAL_SRC}
            ${ST_LL_SRC}
            ${CUBE}/cube.c
        )
    endif()

    set_source_files_properties(${ST_CMSIS_SRC} ${ST_HAL_SRC} ${ST_LL_SRC}
        PROPERTIES COMPILE_FLAGS "-w")
    set(${out_var} ${src} PARENT_SCOPE)
endfunction()
