cmake_minimum_required(VERSION 3.28)
project(chronos LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 26)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

find_program(LLD NAMES ld.lld ld.lld.exe)

add_library(chronos_project_options INTERFACE)
target_compile_options(chronos_project_options INTERFACE
    -Wall -Wextra -Werror $<$<CXX_COMPILER_ID:Clang>:-Wno-unused-command-line-argument>
)

function(chronos_use_lld target)
    if(LLD)
        if(CMAKE_CXX_COMPILER_ID MATCHES "Clang")
            target_link_options(${target} PRIVATE "--ld-path=${LLD}")
        elseif(CMAKE_CXX_COMPILER_ID STREQUAL "GNU")
            target_link_options(${target} PRIVATE "-fuse-ld=lld")
        endif()
    endif()
endfunction()

# All sources use bare #include "foo.hpp" names; the include directories
# below let the preprocessor find headers regardless of which subdir they
# live in. Adding a new file = drop it in the right subdir, no include edits.
set(CHRONOS_INCLUDE_DIRS
    src
    src/core
    src/ui
    src/input
    src/painting
    src/win32
    src/linux
)

# Logic-portable sources shared by the desktop app and unit tests.
# Keep Windows.h-dependent files in the platform sections below.
set(CHRONOS_CORE_SOURCES
    src/core/actions.cpp
    src/core/config_serial.cpp
    src/core/encoding.cpp
    src/core/formatting.cpp
)

add_library(chronos_core STATIC ${CHRONOS_CORE_SOURCES})
target_include_directories(chronos_core PUBLIC ${CHRONOS_INCLUDE_DIRS})
target_link_libraries(chronos_core PUBLIC chronos_project_options)
set_target_properties(chronos_core PROPERTIES CXX_SCAN_FOR_MODULES OFF)

set(CHRONOS_APP_SOURCES src/main.cpp)

if(WIN32)
    list(APPEND CHRONOS_APP_SOURCES
        src/win32/config_io.cpp
        src/win32/dialog_style.cpp
        src/input/input_core.cpp
        src/input/input_keyboard.cpp
        src/input/input_label_edit.cpp
        src/input/input_mouse.cpp
        src/painting/painting_analog.cpp
        src/painting/painting_scene.cpp
        src/win32/polling.cpp
        src/win32/window.cpp
        src/win32/alarm_dialog.cpp
        src/win32/pomodoro_dialog.cpp
        src/win32/ui_windows_app.cpp
        src/win32/ui_windows_settings_dialog.cpp
    )
    add_executable(chronos WIN32 ${CHRONOS_APP_SOURCES})

    target_link_libraries(chronos PRIVATE chronos_core)
    set_target_properties(chronos PROPERTIES CXX_SCAN_FOR_MODULES OFF)

    target_compile_definitions(chronos PRIVATE
        WIN32_LEAN_AND_MEAN
        NOMINMAX
        UNICODE
        _UNICODE
    )

    target_link_options(chronos PRIVATE
        -static-libstdc++
        -static-libgcc
    )
    chronos_use_lld(chronos)

    target_link_libraries(chronos PRIVATE
        -Wl,-Bstatic winpthread -Wl,-Bdynamic
        gdi32 user32 dwmapi shell32 advapi32
    )
elseif(UNIX AND NOT APPLE)
    find_package(X11 REQUIRED)
    list(APPEND CHRONOS_APP_SOURCES src/linux/ui_linux.cpp)
    add_executable(chronos ${CHRONOS_APP_SOURCES})
    target_link_libraries(chronos PRIVATE chronos_core X11::X11)
    set_target_properties(chronos PROPERTIES CXX_SCAN_FOR_MODULES OFF)
endif()

# Copy compile_commands.json to repo root after build so all editors find it
add_custom_target(copy_compile_commands ALL
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
        ${CMAKE_BINARY_DIR}/compile_commands.json
        ${CMAKE_SOURCE_DIR}/compile_commands.json
    COMMENT "Copying compile_commands.json to repo root"
)

option(BUILD_TESTS "Build unit tests" OFF)
option(CHRONOS_USE_BUNDLED_CATCH2 "Prefer third_party/Catch2 when present" ON)
option(CHRONOS_FETCH_CATCH2 "Allow downloading Catch2 when not vendored" ON)

if(BUILD_TESTS)
    enable_testing()

    set(CHRONOS_CATCH2_SOURCE_DIR "${CMAKE_SOURCE_DIR}/third_party/Catch2")

    if(CHRONOS_USE_BUNDLED_CATCH2 AND EXISTS "${CHRONOS_CATCH2_SOURCE_DIR}/CMakeLists.txt")
        message(STATUS "Using bundled Catch2 from third_party/Catch2")
        add_subdirectory("${CHRONOS_CATCH2_SOURCE_DIR}" "${CMAKE_BINARY_DIR}/_deps/catch2-build" EXCLUDE_FROM_ALL)
        set(catch2_SOURCE_DIR "${CHRONOS_CATCH2_SOURCE_DIR}")
    else()
        find_package(Catch2 3 CONFIG QUIET)
        if(NOT Catch2_FOUND)
            if(CHRONOS_FETCH_CATCH2)
                include(FetchContent)
                FetchContent_Declare(Catch2
                    URL      https://github.com/catchorg/Catch2/archive/refs/tags/v3.5.2.tar.gz
                    URL_HASH SHA256=269543a49eb76f40b3f93ff231d4c24c27a7e16c90e47d2e45bcc564de470c6e
                )
                FetchContent_MakeAvailable(Catch2)
            else()
                message(FATAL_ERROR "Catch2 not found. Provide third_party/Catch2, install Catch2 via package manager, or set CHRONOS_FETCH_CATCH2=ON")
            endif()
        endif()
    endif()

    add_executable(chronos_tests
        tests/test_stopwatch.cpp
        tests/test_timer.cpp
        tests/test_config.cpp
        tests/test_encoding.cpp
        tests/test_formatting.cpp
        tests/test_layout.cpp
        tests/test_actions_dispatch.cpp
        tests/test_actions_stopwatch.cpp
        tests/test_actions_timer.cpp
        tests/test_actions_adjust.cpp
        tests/test_pomodoro.cpp
        tests/test_alarm.cpp
    )
    set_target_properties(chronos_tests PROPERTIES CXX_SCAN_FOR_MODULES OFF)
    target_link_libraries(chronos_tests PRIVATE chronos_core Catch2::Catch2WithMain)
    chronos_use_lld(chronos_tests)
    if(WIN32)
        target_compile_definitions(chronos_tests PRIVATE
            WIN32_LEAN_AND_MEAN
            NOMINMAX
            UNICODE
            _UNICODE
        )
        target_link_options(chronos_tests PRIVATE
            -static-libstdc++
            -static-libgcc
        )
    endif()

    list(APPEND CMAKE_MODULE_PATH ${catch2_SOURCE_DIR}/extras)
    include(Catch OPTIONAL)
    if(COMMAND catch_discover_tests)
        catch_discover_tests(chronos_tests)
    else()
        add_test(NAME chronos_tests COMMAND chronos_tests)
    endif()
endif()
