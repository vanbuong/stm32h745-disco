if(NOT DEFINED H745_ROOT)
    get_filename_component(H745_ROOT "${CMAKE_CURRENT_LIST_DIR}/.." ABSOLUTE)
endif()
include_guard(GLOBAL)

set(FREERTOS_KERNEL ${H745_ROOT}/third_party/freertos-kernel)
set(FREERTOS_PORT ${H745_ROOT}/firmware/src/port/freertos)

if(NOT EXISTS ${FREERTOS_KERNEL}/tasks.c)
    message(FATAL_ERROR
        "Missing FreeRTOS-Kernel at ${FREERTOS_KERNEL}.\n"
        "Run: git submodule update --init --recursive")
endif()

set(FREERTOS_SRC
    ${FREERTOS_KERNEL}/tasks.c
    ${FREERTOS_KERNEL}/queue.c
    ${FREERTOS_KERNEL}/list.c
    ${FREERTOS_KERNEL}/timers.c
    ${FREERTOS_KERNEL}/event_groups.c
    ${FREERTOS_KERNEL}/stream_buffer.c
    ${FREERTOS_KERNEL}/portable/MemMang/heap_4.c
    ${FREERTOS_KERNEL}/portable/GCC/ARM_CM4F/port.c
    ${FREERTOS_PORT}/heap.c
    ${FREERTOS_PORT}/hooks.c
    ${H745_ROOT}/firmware/src/osal/freertos/osal.c
)

function(stm32_add_freertos TGT)
    target_sources(${TGT} PRIVATE ${FREERTOS_SRC})
    target_include_directories(${TGT} PRIVATE
        ${FREERTOS_PORT}
        ${FREERTOS_KERNEL}/include
        ${FREERTOS_KERNEL}/portable/GCC/ARM_CM4F
    )
    set_source_files_properties(${FREERTOS_SRC} PROPERTIES COMPILE_FLAGS "-w")
endfunction()
