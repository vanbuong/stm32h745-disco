set(BSP ${CMAKE_SOURCE_DIR}/firmware/src/bsp/stm32h745i_disco)

if(CORE STREQUAL "M7")
    set(CPU_FLAGS -mcpu=cortex-m7 -mthumb -mfpu=fpv5-d16 -mfloat-abi=hard)
    set(LINKER ${BSP}/stm32h745_m7.ld)
    set(MAIN ${BSP}/main_m7.c)
    set(TGT firmware-m7)
else()
    set(CPU_FLAGS -mcpu=cortex-m4 -mthumb -mfpu=fpv4-sp-d16 -mfloat-abi=hard)
    set(LINKER ${BSP}/stm32h745_m4.ld)
    set(MAIN ${BSP}/main_m4.c)
    set(TGT firmware-m4)
endif()

add_executable(${TGT} ${BSP}/startup.c ${MAIN})
target_include_directories(${TGT} PRIVATE ${BSP} ${CMAKE_SOURCE_DIR}/firmware/include)
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
