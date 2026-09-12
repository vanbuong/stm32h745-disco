set(UNITY_ROOT ${CMAKE_SOURCE_DIR}/third_party/unity)
if(NOT EXISTS ${UNITY_ROOT}/src/unity.c)
    message(FATAL_ERROR
        "Missing Unity at ${UNITY_ROOT}.\n"
        "Run: git submodule update --init --recursive")
endif()

include(${CMAKE_SOURCE_DIR}/cmake/Helix.cmake)

set(HOST_SRC
    ${UNITY_ROOT}/src/unity.c
    ${CMAKE_SOURCE_DIR}/firmware/src/ipc/ipc_ring.c
    ${CMAKE_SOURCE_DIR}/firmware/src/ipc/ipc_link.c
    ${CMAKE_SOURCE_DIR}/firmware/src/svc/vfs_path.c
    ${CMAKE_SOURCE_DIR}/firmware/src/svc/vfs_probe.c
    ${CMAKE_SOURCE_DIR}/firmware/src/svc/znp_mt.c
    ${CMAKE_SOURCE_DIR}/firmware/src/svc/media_probe.c
    ${CMAKE_SOURCE_DIR}/firmware/src/svc/text_view.c
    ${CMAKE_SOURCE_DIR}/firmware/src/svc/media_image.c
    ${CMAKE_SOURCE_DIR}/firmware/src/svc/media_bmp.c
    ${CMAKE_SOURCE_DIR}/firmware/src/svc/media_jpeg.c
    ${CMAKE_SOURCE_DIR}/firmware/src/svc/media_png.c
    ${CMAKE_SOURCE_DIR}/firmware/src/svc/media_audio.c
    ${CMAKE_SOURCE_DIR}/firmware/src/svc/audio_mix.c
    ${CMAKE_SOURCE_DIR}/firmware/src/svc/audio_pipe.c
    ${CMAKE_SOURCE_DIR}/firmware/src/svc/audio_engine.c
    ${CMAKE_SOURCE_DIR}/firmware/src/svc/audio.c
    ${CMAKE_SOURCE_DIR}/firmware/src/svc/net.c
    ${CMAKE_SOURCE_DIR}/firmware/src/svc/time.c
    ${CMAKE_SOURCE_DIR}/firmware/src/svc/vendor/tjpgd/tjpgd.c
    ${CMAKE_SOURCE_DIR}/firmware/src/svc/vendor/puff/puff.c
    ${HELIX_SRC}
    ${CMAKE_SOURCE_DIR}/firmware/src/svc/auto.c
    ${CMAKE_SOURCE_DIR}/firmware/src/svc/memtest.c
    ${CMAKE_SOURCE_DIR}/firmware/src/bsp/mpu_map.c
    ${CMAKE_SOURCE_DIR}/firmware/src/bsp/disp_geom.c
    ${CMAKE_SOURCE_DIR}/firmware/src/shell/nav.c
    ${CMAKE_SOURCE_DIR}/firmware/src/shell/shell.c
    ${CMAKE_SOURCE_DIR}/firmware/src/shell/launcher_geom.c
    ${CMAKE_SOURCE_DIR}/firmware/src/app/apps.c
    ${CMAKE_SOURCE_DIR}/firmware/src/app/files.c
    ${CMAKE_SOURCE_DIR}/firmware/src/app/image_view.c
    ${CMAKE_SOURCE_DIR}/firmware/src/app/player.c
    ${CMAKE_SOURCE_DIR}/firmware/src/app/network.c
    ${CMAKE_SOURCE_DIR}/firmware/src/osal/posix/osal.c
    ${CMAKE_SOURCE_DIR}/tests/host/test_main.c
    ${CMAKE_SOURCE_DIR}/tests/host/test_ipc.c
    ${CMAKE_SOURCE_DIR}/tests/host/test_vfs.c
    ${CMAKE_SOURCE_DIR}/tests/host/test_znp.c
    ${CMAKE_SOURCE_DIR}/tests/host/test_media_auto.c
    ${CMAKE_SOURCE_DIR}/tests/host/test_mem.c
    ${CMAKE_SOURCE_DIR}/tests/host/test_disp.c
    ${CMAKE_SOURCE_DIR}/tests/host/test_shell.c
    ${CMAKE_SOURCE_DIR}/tests/host/test_files.c
    ${CMAKE_SOURCE_DIR}/tests/host/test_text.c
    ${CMAKE_SOURCE_DIR}/tests/host/test_image.c
    ${CMAKE_SOURCE_DIR}/tests/host/test_audio.c
    ${CMAKE_SOURCE_DIR}/tests/host/test_net.c
    ${CMAKE_SOURCE_DIR}/tests/host/vfs_ram.c
)

add_executable(host_tests ${HOST_SRC})
target_include_directories(host_tests PRIVATE
    ${CMAKE_SOURCE_DIR}/firmware/include
    ${CMAKE_SOURCE_DIR}/firmware/src/svc
    ${CMAKE_SOURCE_DIR}/firmware/src/svc/vendor/tjpgd
    ${CMAKE_SOURCE_DIR}/firmware/src/svc/vendor/puff
    ${HELIX_ROOT}/pub
    ${HELIX_ROOT}/real
    ${CMAKE_SOURCE_DIR}/tests/host
    ${CMAKE_SOURCE_DIR}/tests/host/data
)
target_include_directories(host_tests SYSTEM PRIVATE ${UNITY_ROOT}/src)
target_compile_options(host_tests PRIVATE -Wall -Wextra -Werror -Wno-unused-parameter)
set_source_files_properties(
    ${CMAKE_SOURCE_DIR}/firmware/src/svc/vendor/tjpgd/tjpgd.c
    ${CMAKE_SOURCE_DIR}/firmware/src/svc/vendor/puff/puff.c
    ${UNITY_ROOT}/src/unity.c
    PROPERTIES COMPILE_FLAGS "-w"
)
target_link_libraries(host_tests PRIVATE pthread)
target_compile_definitions(host_tests PRIVATE _GNU_SOURCE)

if(COVERAGE)
    include(${CMAKE_SOURCE_DIR}/cmake/Coverage.cmake)
    target_append_coverage(host_tests)
endif()

enable_testing()
add_test(NAME host_tests COMMAND host_tests)
