cmake_minimum_required(VERSION 3.28)
project(chronos LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 26)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

find_program(LLD NAMES ld.lld ld.lld.exe)

# Shared (platform-independent) sources
set(CHRONOS_SOURCES
    src/main.cpp
    src/actions.cpp
    src/config_serial.cpp
    src/config_io.cpp
    src/dialog_style.cpp
    src/encoding.cpp
    src/formatting.cpp
    src/input_core.cpp
    src/input_keyboard.cpp
    src/input_label_edit.cpp
    src/input_mouse.cpp
    src/painting.cpp
    src/painting_analog.cpp
    src/painting_timer.cpp
    src/polling.cpp
    src/window.cpp
)

if(WIN32)
    list(APPEND CHRONOS_SOURCES
        src/alarm_dialog.cpp
        src/pomodoro_dialog.cpp
        src/ui_windows_app.cpp
        src/ui_windows_settings_dialog.cpp
    )
    add_executable(chronos WIN32 ${CHRONOS_SOURCES})

    # Copy compile_commands.json to repo root after configure so editors find it automatically
    add_custom_target(copy_compile_commands ALL
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
            ${CMAKE_BINARY_DIR}/compile_commands.json
            ${CMAKE_SOURCE_DIR}/compile_commands.json
        COMMENT "Copying compile_commands.json to repo root"
    )

    target_include_directories(chronos PRIVATE src)
    set_target_properties(chronos PROPERTIES CXX_SCAN_FOR_MODULES OFF)

    target_compile_definitions(chronos PRIVATE
        WIN32_LEAN_AND_MEAN
        NOMINMAX
        UNICODE
        _UNICODE
    )

    target_compile_options(chronos PRIVATE
        -Wall -Wextra -Werror -Wno-unused-command-line-argument
    )

    target_link_options(chronos PRIVATE
        -static-libstdc++
        -static-libgcc
    )
    if(LLD)
        target_link_options(chronos PRIVATE "--ld-path=${LLD}")
    endif()

    target_link_libraries(chronos PRIVATE
        -Wl,-Bstatic winpthread -Wl,-Bdynamic
        gdi32 user32 dwmapi shell32 advapi32
    )
elseif(UNIX AND NOT APPLE)
    find_package(X11 REQUIRED)
    list(APPEND CHRONOS_SOURCES src/ui_linux.cpp)
    add_executable(chronos ${CHRONOS_SOURCES})
    target_include_directories(chronos PRIVATE src)
    set_target_properties(chronos PROPERTIES CXX_SCAN_FOR_MODULES OFF)
    target_compile_options(chronos PRIVATE
        -Wall -Wextra -Werror -Wno-unused-command-line-argument
    )
    target_link_libraries(chronos PRIVATE X11::X11)
endif()

option(BUILD_TESTS "Build unit tests" OFF)
if(BUILD_TESTS)
    enable_testing()
    include(FetchContent)
    FetchContent_Declare(Catch2
        URL      https://github.com/catchorg/Catch2/archive/refs/tags/v3.5.2.tar.gz
        URL_HASH SHA256=269543a49eb76f40b3f93ff231d4c24c27a7e16c90e47d2e45bcc564de470c6e
    )
    FetchContent_MakeAvailable(Catch2)

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
        src/actions.cpp
        src/config_serial.cpp
        src/encoding.cpp
        src/formatting.cpp
    )
    target_include_directories(chronos_tests PRIVATE src)
    set_target_properties(chronos_tests PROPERTIES CXX_SCAN_FOR_MODULES OFF)
    target_link_libraries(chronos_tests PRIVATE Catch2::Catch2WithMain)
    target_compile_options(chronos_tests PRIVATE
        -Wall -Wextra -Werror -Wno-unused-command-line-argument
    )
    if(LLD)
        target_link_options(chronos_tests PRIVATE "--ld-path=${LLD}")
    endif()
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
