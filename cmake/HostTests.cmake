set(HOST_SRC
    ${CMAKE_SOURCE_DIR}/firmware/src/ipc/ipc_ring.c
    ${CMAKE_SOURCE_DIR}/firmware/src/svc/vfs_path.c
    ${CMAKE_SOURCE_DIR}/firmware/src/svc/znp_mt.c
    ${CMAKE_SOURCE_DIR}/firmware/src/svc/media_probe.c
    ${CMAKE_SOURCE_DIR}/firmware/src/svc/auto.c
    ${CMAKE_SOURCE_DIR}/firmware/src/osal/posix/osal.c
    ${CMAKE_SOURCE_DIR}/tests/host/test_main.c
    ${CMAKE_SOURCE_DIR}/tests/host/test_ipc.c
    ${CMAKE_SOURCE_DIR}/tests/host/test_vfs.c
    ${CMAKE_SOURCE_DIR}/tests/host/test_znp.c
    ${CMAKE_SOURCE_DIR}/tests/host/test_media_auto.c
)

add_executable(host_tests ${HOST_SRC})
target_include_directories(host_tests PRIVATE
    ${CMAKE_SOURCE_DIR}/firmware/include
    ${CMAKE_SOURCE_DIR}/tests/host
)
target_compile_options(host_tests PRIVATE -Wall -Wextra -Werror -Wno-unused-parameter)
target_link_libraries(host_tests PRIVATE pthread)
target_compile_definitions(host_tests PRIVATE _GNU_SOURCE)

if(COVERAGE)
    include(${CMAKE_SOURCE_DIR}/cmake/Coverage.cmake)
    target_append_coverage(host_tests)
endif()

enable_testing()
add_test(NAME host_tests COMMAND host_tests)
