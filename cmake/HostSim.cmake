# Host LVGL + SDL2 simulator (Sprint 5b). Not unit tests: those stay LVGL-free.

set(LVGL_ROOT ${CMAKE_SOURCE_DIR}/third_party/lvgl)
if(NOT EXISTS ${LVGL_ROOT}/src/lv_init.c)
    message(FATAL_ERROR
        "Missing LVGL at ${LVGL_ROOT}.\n"
        "Run: git submodule update --init --recursive")
endif()

function(host_sim_link_sdl tgt)
    find_package(SDL2 QUIET)
    if(TARGET SDL2::SDL2)
        target_link_libraries(${tgt} PRIVATE SDL2::SDL2)
    elseif(SDL2_FOUND)
        target_include_directories(${tgt} PRIVATE ${SDL2_INCLUDE_DIRS})
        target_link_libraries(${tgt} PRIVATE ${SDL2_LIBRARIES})
    else()
        find_package(PkgConfig QUIET)
        if(PKG_CONFIG_FOUND)
            pkg_check_modules(PC_SDL2 QUIET sdl2)
        endif()
        if(PC_SDL2_FOUND)
            target_include_directories(${tgt} PRIVATE ${PC_SDL2_INCLUDE_DIRS})
            target_link_libraries(${tgt} PRIVATE ${PC_SDL2_LIBRARIES})
        else()
            include(FetchContent)
            set(SDL_SHARED OFF CACHE BOOL "" FORCE)
            set(SDL_STATIC ON CACHE BOOL "" FORCE)
            set(SDL_TEST OFF CACHE BOOL "" FORCE)
            set(SDL_TESTS OFF CACHE BOOL "" FORCE)
            FetchContent_Declare(SDL2
                GIT_REPOSITORY https://github.com/libsdl-org/SDL.git
                GIT_TAG release-2.30.8
                GIT_SHALLOW TRUE
            )
            FetchContent_MakeAvailable(SDL2)
            if(TARGET SDL2::SDL2-static)
                target_link_libraries(${tgt} PRIVATE SDL2::SDL2-static)
            elseif(TARGET SDL2-static)
                target_link_libraries(${tgt} PRIVATE SDL2-static)
            elseif(TARGET SDL2::SDL2)
                target_link_libraries(${tgt} PRIVATE SDL2::SDL2)
            else()
                message(FATAL_ERROR "SDL2 FetchContent did not create a link target")
            endif()
        endif()
    endif()
    if(EXISTS "/usr/include/SDL2/SDL.h")
        target_include_directories(${tgt} PRIVATE /usr/include/SDL2)
    endif()
endfunction()

file(GLOB_RECURSE LVGL_SRC CONFIGURE_DEPENDS ${LVGL_ROOT}/src/*.c)
list(FILTER LVGL_SRC EXCLUDE REGEX "/drivers/")
list(FILTER LVGL_SRC EXCLUDE REGEX "/libs/")
list(APPEND LVGL_SRC
    ${LVGL_ROOT}/src/libs/bin_decoder/lv_bin_decoder.c
    ${LVGL_ROOT}/src/drivers/sdl/lv_sdl_window.c
    ${LVGL_ROOT}/src/drivers/sdl/lv_sdl_mouse.c
    ${LVGL_ROOT}/src/drivers/sdl/lv_sdl_keyboard.c
    ${LVGL_ROOT}/src/drivers/sdl/lv_sdl_mousewheel.c
    ${LVGL_ROOT}/src/drivers/sdl/lv_sdl_sw.c
)

set(SIM_SRC
    ${CMAKE_SOURCE_DIR}/firmware/src/svc/vfs_path.c
    ${CMAKE_SOURCE_DIR}/firmware/src/svc/media_probe.c
    ${CMAKE_SOURCE_DIR}/firmware/src/shell/nav.c
    ${CMAKE_SOURCE_DIR}/firmware/src/shell/shell.c
    ${CMAKE_SOURCE_DIR}/firmware/src/shell/launcher_geom.c
    ${CMAKE_SOURCE_DIR}/firmware/src/app/apps.c
    ${CMAKE_SOURCE_DIR}/firmware/src/app/files.c
    ${CMAKE_SOURCE_DIR}/firmware/src/ui/backend_lvgl/ui_lvgl.c
    ${CMAKE_SOURCE_DIR}/firmware/src/ui/backend_lvgl/lv_port_sim.c
    ${CMAKE_SOURCE_DIR}/tests/host/sim/sim_main.c
    ${CMAKE_SOURCE_DIR}/tests/host/sim/sim_board.c
    ${CMAKE_SOURCE_DIR}/tests/host/sim/vfs_host.c
    ${LVGL_SRC}
)

add_executable(host_sim ${SIM_SRC})
target_include_directories(host_sim PRIVATE
    ${CMAKE_SOURCE_DIR}/firmware/include
    ${CMAKE_SOURCE_DIR}/firmware/src/port/lvgl_sim
    ${CMAKE_SOURCE_DIR}/firmware/src/ui/backend_lvgl
    ${CMAKE_SOURCE_DIR}/tests/host/sim
)
target_include_directories(host_sim SYSTEM PRIVATE ${LVGL_ROOT})
target_compile_definitions(host_sim PRIVATE LV_CONF_INCLUDE_SIMPLE)
if(MSVC)
    target_compile_definitions(host_sim PRIVATE _CRT_SECURE_NO_WARNINGS)
    target_compile_options(host_sim PRIVATE /W3)
    set_source_files_properties(${LVGL_SRC} PROPERTIES COMPILE_FLAGS "/w")
else()
    target_compile_options(host_sim PRIVATE -Wall -Wextra)
    set_source_files_properties(${LVGL_SRC} PROPERTIES COMPILE_FLAGS "-w")
endif()
if(UNIX)
    target_link_libraries(host_sim PRIVATE m)
endif()
host_sim_link_sdl(host_sim)

set(SIM_USER_SRC ${CMAKE_SOURCE_DIR}/tests/host/sim_user)
add_custom_command(TARGET host_sim POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_directory ${SIM_USER_SRC} $<TARGET_FILE_DIR:host_sim>/user
    COMMAND ${CMAKE_COMMAND} -E make_directory $<TARGET_FILE_DIR:host_sim>/user/empty
    COMMENT "Copy demo /user tree next to host_sim"
)
