cmake_minimum_required(VERSION 3.28)
project(chronos LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 26)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

find_program(LLD NAMES ld.lld ld.lld.exe)

if(WIN32)
    add_executable(chronos WIN32 src/main.cpp)

    target_sources(chronos PRIVATE
        FILE_SET CXX_MODULES FILES
            src/actions.ixx
            src/app.ixx
            src/config.ixx
            src/config_io.ixx
            src/debug.ixx
            src/dpi.ixx
            src/encoding.ixx
            src/formatting.ixx
            src/gdi.ixx
            src/icon.ixx
            src/input.ixx
            src/layout.ixx
            src/painting.ixx
            src/painting_timer.ixx
            src/polling.ixx
            src/stopwatch.ixx
            src/theme.ixx
            src/timer.ixx
            src/tray.ixx
            src/window.ixx
            src/wndstate.ixx
    )

    target_compile_options(chronos PRIVATE
        -Wall -Wextra -Wno-unused-command-line-argument
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
        tests/test_actions.cpp
    )
    target_sources(chronos_tests PRIVATE
        FILE_SET CXX_MODULES FILES
            src/actions.ixx
            src/app.ixx
            src/config.ixx
            src/encoding.ixx
            src/formatting.ixx
            src/layout.ixx
            src/stopwatch.ixx
            src/timer.ixx
    )
    target_link_libraries(chronos_tests PRIVATE Catch2::Catch2WithMain)
    target_compile_options(chronos_tests PRIVATE
        -Wall -Wextra -Wno-unused-command-line-argument
    )
    if(LLD)
        target_link_options(chronos_tests PRIVATE "--ld-path=${LLD}")
    endif()
    if(WIN32)
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
