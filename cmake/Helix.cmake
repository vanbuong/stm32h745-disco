set(HELIX_ROOT ${CMAKE_SOURCE_DIR}/third_party/helix)
if(NOT EXISTS ${HELIX_ROOT}/mp3dec.c)
    message(FATAL_ERROR
        "Missing Helix at ${HELIX_ROOT}.\n"
        "Run: git submodule update --init --recursive")
endif()

set(HELIX_SRC
    ${HELIX_ROOT}/mp3dec.c
    ${HELIX_ROOT}/mp3tabs.c
    ${HELIX_ROOT}/real/bitstream.c
    ${HELIX_ROOT}/real/buffers.c
    ${HELIX_ROOT}/real/dct32.c
    ${HELIX_ROOT}/real/dequant.c
    ${HELIX_ROOT}/real/dqchan.c
    ${HELIX_ROOT}/real/huffman.c
    ${HELIX_ROOT}/real/hufftabs.c
    ${HELIX_ROOT}/real/imdct.c
    ${HELIX_ROOT}/real/polyphase.c
    ${HELIX_ROOT}/real/scalfact.c
    ${HELIX_ROOT}/real/stproc.c
    ${HELIX_ROOT}/real/subband.c
    ${HELIX_ROOT}/real/trigtabs.c
)
set(HELIX_GENERIC_ASM ${CMAKE_SOURCE_DIR}/firmware/src/svc/vendor/helix_generic_asm.h)
if(MSVC)
    set_source_files_properties(${HELIX_SRC} PROPERTIES
        COMPILE_FLAGS "/w /FI${HELIX_GENERIC_ASM}")
else()
    set_source_files_properties(${HELIX_SRC} PROPERTIES
        COMPILE_FLAGS "-w -include ${HELIX_GENERIC_ASM}")
endif()
