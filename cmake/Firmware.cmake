include(${CMAKE_SOURCE_DIR}/cmake/Cube.cmake)

function(stm32_add_firmware CORE_ID)
    set(BSP ${CMAKE_SOURCE_DIR}/firmware/src/bsp/stm32h745i_disco)
    cube_collect_sources(${CORE_ID} CUBE_SRC)

    if(CORE_ID STREQUAL "M7")
        set(CPU_FLAGS -mcpu=cortex-m7 -mthumb -mfpu=fpv5-d16 -mfloat-abi=hard)
        set(LINKER ${BSP}/stm32h745_m7.ld)
        set(CORE_DEFINE CORE_CM7)
        set(TGT firmware-m7)
        if(NOT EXISTS ${ST_ROOT}/stm32-rk043fn48h/rk043fn48h.h)
            message(FATAL_ERROR
                "Missing stm32-rk043fn48h at ${ST_ROOT}/stm32-rk043fn48h.\n"
                "Run: git submodule update --init --recursive")
        endif()
        if(NOT EXISTS ${ST_ROOT}/stm32-ft5336/ft5336.h)
            message(FATAL_ERROR
                "Missing stm32-ft5336 at ${ST_ROOT}/stm32-ft5336.\n"
                "Run: git submodule update --init --recursive")
        endif()
        if(NOT EXISTS ${ST_ROOT}/fatfs/source/ff.c)
            message(FATAL_ERROR
                "Missing FatFs at ${ST_ROOT}/fatfs.\n"
                "Run: git submodule update --init --recursive")
        endif()
        if(NOT EXISTS ${ST_ROOT}/lvgl/src/lv_init.c)
            message(FATAL_ERROR
                "Missing LVGL at ${ST_ROOT}/lvgl.\n"
                "Run: git submodule update --init --recursive")
        endif()
        set(FT5336_SRC
            ${ST_ROOT}/stm32-ft5336/ft5336.c
            ${ST_ROOT}/stm32-ft5336/ft5336_reg.c
        )
        set(FATFS_SRC
            ${ST_ROOT}/fatfs/source/ff.c
            ${ST_ROOT}/fatfs/source/ffunicode.c
        )
        file(GLOB_RECURSE LVGL_SRC CONFIGURE_DEPENDS ${ST_ROOT}/lvgl/src/*.c)
        list(FILTER LVGL_SRC EXCLUDE REGEX "/drivers/")
        list(FILTER LVGL_SRC EXCLUDE REGEX "/libs/")
        list(APPEND LVGL_SRC ${ST_ROOT}/lvgl/src/libs/bin_decoder/lv_bin_decoder.c)
        set(FFCONF ${CMAKE_SOURCE_DIR}/firmware/src/port/fatfs/ffconf.h)
        set_source_files_properties(${FT5336_SRC} PROPERTIES COMPILE_FLAGS "-w")
        set_source_files_properties(${FATFS_SRC} PROPERTIES
            COMPILE_FLAGS "-w -include ${FFCONF}")
        set_source_files_properties(${LVGL_SRC} PROPERTIES COMPILE_FLAGS "-w")
        set_source_files_properties(
            ${BSP}/emmc.c
            ${CMAKE_SOURCE_DIR}/firmware/src/svc/vfs.c
            PROPERTIES COMPILE_FLAGS "-include ${FFCONF}")
        set(APP_SRC
            ${BSP}/startup.c
            ${BSP}/main_m7.c
            ${BSP}/clock.c
            ${BSP}/console.c
            ${BSP}/mpu.c
            ${BSP}/cache.c
            ${BSP}/sdram.c
            ${BSP}/qspi.c
            ${BSP}/lcd.c
            ${BSP}/i2c4.c
            ${BSP}/input.c
            ${BSP}/emmc.c
            ${CMAKE_SOURCE_DIR}/firmware/src/bsp/mpu_map.c
            ${CMAKE_SOURCE_DIR}/firmware/src/bsp/disp_geom.c
            ${CMAKE_SOURCE_DIR}/firmware/src/svc/memtest.c
            ${CMAKE_SOURCE_DIR}/firmware/src/svc/vfs.c
            ${CMAKE_SOURCE_DIR}/firmware/src/svc/vfs_path.c
            ${CMAKE_SOURCE_DIR}/firmware/src/svc/media_probe.c
            ${CMAKE_SOURCE_DIR}/firmware/src/shell/nav.c
            ${CMAKE_SOURCE_DIR}/firmware/src/shell/shell.c
            ${CMAKE_SOURCE_DIR}/firmware/src/shell/launcher_geom.c
            ${CMAKE_SOURCE_DIR}/firmware/src/app/apps.c
            ${CMAKE_SOURCE_DIR}/firmware/src/app/files.c
            ${CMAKE_SOURCE_DIR}/firmware/src/ui/backend_lvgl/lv_port.c
            ${CMAKE_SOURCE_DIR}/firmware/src/ui/backend_lvgl/ui_lvgl.c
            ${FT5336_SRC}
            ${FATFS_SRC}
            ${LVGL_SRC}
        )
    elseif(CORE_ID STREQUAL "M4")
        set(CPU_FLAGS -mcpu=cortex-m4 -mthumb -mfpu=fpv4-sp-d16 -mfloat-abi=hard)
        set(LINKER ${BSP}/stm32h745_m4.ld)
        set(CORE_DEFINE CORE_CM4)
        set(TGT firmware-m4)
        set(APP_SRC
            ${BSP}/startup.c
            ${BSP}/main_m4.c
        )
    else()
        message(FATAL_ERROR "stm32_add_firmware expects M7 or M4, got ${CORE_ID}")
    endif()

    add_executable(${TGT} ${APP_SRC} ${CUBE_SRC})
    target_include_directories(${TGT} PRIVATE
        ${CUBE}
        ${CMAKE_SOURCE_DIR}/firmware/include
        ${CMAKE_SOURCE_DIR}/firmware/src/port/fatfs
        ${CMAKE_SOURCE_DIR}/firmware/src/port/lvgl
        ${CMAKE_SOURCE_DIR}/firmware/src/ui/backend_lvgl
        ${BSP}
    )
    target_include_directories(${TGT} SYSTEM PRIVATE
        ${ST_HAL_DIR}/Inc
        ${ST_CMSIS_DEV}/Include
        ${ST_CMSIS_CORE}/Core/Include
        ${ST_ROOT}/stm32-rk043fn48h
        ${ST_ROOT}/stm32-ft5336
        ${ST_ROOT}/fatfs/source
        ${ST_ROOT}/lvgl
    )
    target_compile_definitions(${TGT} PRIVATE
        STM32H745xx
        ${CORE_DEFINE}
        USE_FULL_LL_DRIVER
        USE_PWR_SMPS_1V8_SUPPLIES_LDO
        HSE_VALUE=25000000U
        $<$<CONFIG:Debug>:DEBUG>
    )
    if(CORE_ID STREQUAL "M7")
        target_compile_definitions(${TGT} PRIVATE USE_HAL_DRIVER LV_CONF_INCLUDE_SIMPLE)
    endif()
    target_compile_options(${TGT} PRIVATE
        ${CPU_FLAGS}
        -ffunction-sections -fdata-sections
        -Wall -Wextra
        -ffreestanding
        $<$<CONFIG:Debug>:-g3 -O0>
        $<$<CONFIG:Release>:-Os>
        $<$<CONFIG:MinSizeRel>:-Os>
        $<$<CONFIG:RelWithDebInfo>:-g -O2>
    )
    target_link_options(${TGT} PRIVATE
        ${CPU_FLAGS}
        -T${LINKER}
        -nostartfiles
        -Wl,--gc-sections
        -Wl,-Map=$<TARGET_FILE_DIR:${TGT}>/${TGT}.map
        --specs=nosys.specs
    )
    set_target_properties(${TGT} PROPERTIES SUFFIX ".elf")
endfunction()
