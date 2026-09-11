include(${CMAKE_SOURCE_DIR}/cmake/Cube.cmake)

set(BSP ${CMAKE_SOURCE_DIR}/firmware/src/bsp/stm32h745i_disco)
set(CUBE ${CMAKE_SOURCE_DIR}/firmware/src/port/cube)

if(CORE STREQUAL "M7")
    set(CPU_FLAGS -mcpu=cortex-m7 -mthumb -mfpu=fpv5-d16 -mfloat-abi=hard)
    set(LINKER ${BSP}/stm32h745_m7.ld)
    set(CORE_DEFINE CORE_CM7)
    set(TGT firmware-m7)
    set(APP_SRC
        ${BSP}/startup.c
        ${BSP}/main_m7.c
        ${BSP}/clock.c
        ${BSP}/console.c
        ${BSP}/mpu.c
        ${BSP}/cache.c
        ${BSP}/sdram.c
        ${BSP}/qspi.c
        ${CMAKE_SOURCE_DIR}/firmware/src/bsp/mpu_map.c
        ${CMAKE_SOURCE_DIR}/firmware/src/svc/memtest.c
    )
else()
    set(CPU_FLAGS -mcpu=cortex-m4 -mthumb -mfpu=fpv4-sp-d16 -mfloat-abi=hard)
    set(LINKER ${BSP}/stm32h745_m4.ld)
    set(CORE_DEFINE CORE_CM4)
    set(TGT firmware-m4)
    set(APP_SRC
        ${BSP}/startup.c
        ${BSP}/main_m4.c
    )
endif()

add_executable(${TGT} ${APP_SRC} ${HAL_SRC})
target_include_directories(${TGT} PRIVATE
    ${CUBE}
    ${CMAKE_SOURCE_DIR}/firmware/include
    ${BSP}
)
target_include_directories(${TGT} SYSTEM PRIVATE
    ${ST_HAL_DIR}/Inc
    ${ST_CMSIS_DEV}/Include
    ${ST_CMSIS_CORE}/Core/Include
)
target_compile_definitions(${TGT} PRIVATE
    STM32H745xx
    ${CORE_DEFINE}
    USE_HAL_DRIVER
    USE_FULL_LL_DRIVER
    USE_PWR_SMPS_1V8_SUPPLIES_LDO
    HSE_VALUE=25000000U
)
target_compile_options(${TGT} PRIVATE
    ${CPU_FLAGS}
    -ffunction-sections -fdata-sections
    -Wall -Wextra
    -ffreestanding
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
