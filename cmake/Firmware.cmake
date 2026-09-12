if(NOT DEFINED H745_ROOT)
    get_filename_component(H745_ROOT "${CMAKE_CURRENT_LIST_DIR}/.." ABSOLUTE)
endif()
include_guard(GLOBAL)
include(${H745_ROOT}/cmake/Helix.cmake)
include(${H745_ROOT}/cmake/LwIP.cmake)
include(${H745_ROOT}/cmake/Cube.cmake)

function(stm32_add_firmware CORE_ID)
    set(BSP ${H745_ROOT}/firmware/src/bsp/stm32h745i_disco)
    cube_collect_sources(${CORE_ID} CUBE_SRC)

    if(CORE_ID STREQUAL "M7")
        set(CPU_FLAGS -mcpu=cortex-m7 -mthumb -mfpu=fpv5-d16 -mfloat-abi=hard)
        set(LINKER ${BSP}/stm32h745_m7.ld)
        set(CORE_DEFINE CORE_CM7)
        set(TGT ${CMAKE_PROJECT_NAME})
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
        set(FFCONF ${H745_ROOT}/firmware/src/port/fatfs/ffconf.h)
        set_source_files_properties(${FT5336_SRC} PROPERTIES COMPILE_FLAGS "-w")
        set_source_files_properties(${FATFS_SRC} PROPERTIES
            COMPILE_FLAGS "-w -include ${FFCONF}")
        set_source_files_properties(${LVGL_SRC} PROPERTIES COMPILE_FLAGS "-w")
        if(NOT EXISTS ${ST_ROOT}/stm32-wm8994/wm8994.c)
            message(FATAL_ERROR
                "Missing stm32-wm8994 at ${ST_ROOT}/stm32-wm8994.\n"
                "Run: git submodule update --init --recursive")
        endif()
        set(WM8994_SRC
            ${ST_ROOT}/stm32-wm8994/wm8994.c
            ${ST_ROOT}/stm32-wm8994/wm8994_reg.c
        )
        set_source_files_properties(${WM8994_SRC} PROPERTIES COMPILE_FLAGS "-w")
        if(NOT EXISTS ${ST_ROOT}/stm32-lan8742/lan8742.c)
            message(FATAL_ERROR
                "Missing stm32-lan8742 at ${ST_ROOT}/stm32-lan8742.\n"
                "Run: git submodule update --init --recursive")
        endif()
        set(LAN8742_SRC ${ST_ROOT}/stm32-lan8742/lan8742.c)
        set_source_files_properties(${LAN8742_SRC} PROPERTIES COMPILE_FLAGS "-w")
        set_source_files_properties(
            ${H745_ROOT}/firmware/src/svc/vendor/tjpgd/tjpgd.c
            ${H745_ROOT}/firmware/src/svc/vendor/puff/puff.c
            PROPERTIES COMPILE_FLAGS "-w")
        set_source_files_properties(
            ${BSP}/emmc.c
            ${H745_ROOT}/firmware/src/svc/vfs.c
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
            ${BSP}/hsem.c
            ${BSP}/ipc_host.c
            ${BSP}/codec.c
            ${BSP}/eth.c
            ${BSP}/rtc.c
            ${H745_ROOT}/firmware/src/port/lwip/ethernetif.c
            ${H745_ROOT}/firmware/src/bsp/mpu_map.c
            ${H745_ROOT}/firmware/src/bsp/disp_geom.c
            ${H745_ROOT}/firmware/src/svc/memtest.c
            ${H745_ROOT}/firmware/src/svc/vfs.c
            ${H745_ROOT}/firmware/src/svc/vfs_path.c
            ${H745_ROOT}/firmware/src/svc/media_probe.c
            ${H745_ROOT}/firmware/src/svc/text_view.c
            ${H745_ROOT}/firmware/src/svc/media_image.c
            ${H745_ROOT}/firmware/src/svc/media_bmp.c
            ${H745_ROOT}/firmware/src/svc/media_jpeg.c
            ${H745_ROOT}/firmware/src/svc/media_png.c
            ${H745_ROOT}/firmware/src/svc/media_audio.c
            ${H745_ROOT}/firmware/src/svc/audio_mix.c
            ${H745_ROOT}/firmware/src/svc/audio_pipe.c
            ${H745_ROOT}/firmware/src/svc/audio.c
            ${H745_ROOT}/firmware/src/svc/net.c
            ${H745_ROOT}/firmware/src/svc/time.c
            ${H745_ROOT}/firmware/src/svc/vendor/tjpgd/tjpgd.c
            ${H745_ROOT}/firmware/src/svc/vendor/puff/puff.c
            ${H745_ROOT}/firmware/src/shell/nav.c
            ${H745_ROOT}/firmware/src/shell/shell.c
            ${H745_ROOT}/firmware/src/shell/launcher_geom.c
            ${H745_ROOT}/firmware/src/app/apps.c
            ${H745_ROOT}/firmware/src/app/files.c
            ${H745_ROOT}/firmware/src/app/image_view.c
            ${H745_ROOT}/firmware/src/app/player.c
            ${H745_ROOT}/firmware/src/app/network.c
            ${H745_ROOT}/firmware/src/ui/backend_lvgl/lv_port.c
            ${H745_ROOT}/firmware/src/ui/backend_lvgl/ui_lvgl.c
            ${H745_ROOT}/firmware/src/ipc/ipc_ring.c
            ${H745_ROOT}/firmware/src/ipc/ipc_link.c
            ${FT5336_SRC}
            ${FATFS_SRC}
            ${LVGL_SRC}
            ${WM8994_SRC}
            ${LAN8742_SRC}
            ${LWIP_SRC}
        )
    elseif(CORE_ID STREQUAL "M4")
        set(CPU_FLAGS -mcpu=cortex-m4 -mthumb -mfpu=fpv4-sp-d16 -mfloat-abi=hard)
        set(LINKER ${BSP}/stm32h745_m4.ld)
        set(CORE_DEFINE CORE_CM4)
        set(TGT ${CMAKE_PROJECT_NAME})
        set(APP_SRC
            ${BSP}/startup.c
            ${BSP}/main_m4.c
            ${BSP}/hsem.c
            ${BSP}/sai_out.c
            ${BSP}/m4_heap.c
            ${H745_ROOT}/firmware/src/ipc/ipc_ring.c
            ${H745_ROOT}/firmware/src/ipc/ipc_link.c
            ${H745_ROOT}/firmware/src/svc/audio_mix.c
            ${H745_ROOT}/firmware/src/svc/audio_pipe.c
            ${H745_ROOT}/firmware/src/svc/audio_engine.c
            ${HELIX_SRC}
        )
    else()
        message(FATAL_ERROR "stm32_add_firmware expects M7 or M4, got ${CORE_ID}")
    endif()

    if(NOT TARGET ${TGT})
        add_executable(${TGT} ${APP_SRC} ${CUBE_SRC})
    else()
        target_sources(${TGT} PRIVATE ${APP_SRC} ${CUBE_SRC})
    endif()
    target_include_directories(${TGT} PRIVATE
        ${CUBE}
        ${H745_ROOT}/firmware/include
        ${H745_ROOT}/firmware/src/svc
        ${H745_ROOT}/firmware/src/svc/vendor/tjpgd
        ${H745_ROOT}/firmware/src/svc/vendor/puff
        ${H745_ROOT}/firmware/src/port/fatfs
        ${H745_ROOT}/firmware/src/port/lvgl
        ${H745_ROOT}/firmware/src/ui/backend_lvgl
        ${H745_ROOT}/firmware/src/port/lwip
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
        ${ST_ROOT}/stm32-wm8994
        ${ST_ROOT}/stm32-lan8742
        ${LWIP_DIR}/src/include
        ${HELIX_ROOT}/pub
        ${HELIX_ROOT}/real
    )
    target_compile_definitions(${TGT} PRIVATE
        STM32H745xx
        ${CORE_DEFINE}
        USE_FULL_LL_DRIVER
        USE_PWR_DIRECT_SMPS_SUPPLY
        HSE_VALUE=25000000U
        $<$<CONFIG:Debug>:DEBUG>
    )
    if(CORE_ID STREQUAL "M7")
        target_compile_definitions(${TGT} PRIVATE USE_HAL_DRIVER LV_CONF_INCLUDE_SIMPLE)
    else()
        target_compile_definitions(${TGT} PRIVATE USE_HAL_DRIVER)
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
    set_target_properties(${TGT} PROPERTIES
        SUFFIX ".elf"
        RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}"
    )
endfunction()
