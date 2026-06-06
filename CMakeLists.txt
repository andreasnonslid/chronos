cmake_minimum_required(VERSION 3.28)
project(chronos LANGUAGES C CXX)

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
        if(CMAKE_CXX_COMPILER_ID MATCHES "Clang" AND (MINGW OR NOT WIN32))
            target_link_options(${target} PRIVATE "--ld-path=${LLD}")
        elseif(CMAKE_CXX_COMPILER_ID STREQUAL "GNU")
            target_link_options(${target} PRIVATE "-fuse-ld=lld")
        endif()
    endif()
endfunction()

# ─── Include directories ──────────────────────────────────────────────────────

set(CHRONOS_INCLUDE_DIRS
    src
    src/core
    src/ui
    src/platform
)

# ─── Core library (pure logic, no platform deps) ──────────────────────────────

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

option(CHRONOS_BUILD_APP "Build the SDL2/OpenGL ImGui desktop application" ON)

if(CHRONOS_BUILD_APP)
    # ─── Dear ImGui ───────────────────────────────────────────────────────────

    set(IMGUI_DIR "${CMAKE_SOURCE_DIR}/third_party/imgui")
    set(IMGUI_SOURCES
        ${IMGUI_DIR}/imgui.cpp
        ${IMGUI_DIR}/imgui_draw.cpp
        ${IMGUI_DIR}/imgui_tables.cpp
        ${IMGUI_DIR}/imgui_widgets.cpp
        ${IMGUI_DIR}/backends/imgui_impl_sdl2.cpp
        ${IMGUI_DIR}/backends/imgui_impl_opengl3.cpp
    )

    add_library(imgui STATIC ${IMGUI_SOURCES})
    target_include_directories(imgui PUBLIC
        ${IMGUI_DIR}
        ${IMGUI_DIR}/backends
    )

    # ─── SDL2 + OpenGL ───────────────────────────────────────────────────────

    option(CHRONOS_FETCH_SDL2 "Allow downloading SDL2 when not found as a CMake package" ON)
    set(CHRONOS_SDL2_VERSION "release-2.32.10" CACHE STRING "SDL2 git tag to fetch when CHRONOS_FETCH_SDL2 is ON")

    find_package(SDL2 CONFIG QUIET COMPONENTS SDL2)
    if(NOT TARGET SDL2::SDL2 AND CHRONOS_FETCH_SDL2)
        include(FetchContent)
        set(SDL_SHARED OFF CACHE BOOL "" FORCE)
        set(SDL_STATIC ON CACHE BOOL "" FORCE)
        set(SDL_TEST OFF CACHE BOOL "" FORCE)
        set(SDL_TESTS OFF CACHE BOOL "" FORCE)
        FetchContent_Declare(SDL2
            GIT_REPOSITORY https://github.com/libsdl-org/SDL.git
            GIT_TAG        ${CHRONOS_SDL2_VERSION}
            GIT_SHALLOW    TRUE
        )
        FetchContent_MakeAvailable(SDL2)
    endif()
    if(NOT TARGET SDL2::SDL2)
        message(FATAL_ERROR "SDL2 not found. Install SDL2, set SDL2_DIR/CMAKE_PREFIX_PATH, or enable CHRONOS_FETCH_SDL2.")
    endif()
    set(CHRONOS_SDL2_TARGET SDL2::SDL2)
    if(TARGET SDL2::SDL2-static)
        set(CHRONOS_SDL2_TARGET SDL2::SDL2-static)
    endif()
    find_package(OpenGL REQUIRED)

    target_link_libraries(imgui PUBLIC ${CHRONOS_SDL2_TARGET} OpenGL::GL)
    if(WIN32 AND CMAKE_CXX_COMPILER_ID MATCHES "Clang")
        target_compile_definitions(imgui PUBLIC __PRFCHWINTRIN_H)
    endif()

    # Suppress warnings in vendored ImGui
    target_compile_options(imgui PRIVATE -w)

    # ─── Platform app sources (cross-platform ImGui layer) ────────────────────

    set(CHRONOS_APP_SOURCES
        src/platform/config_io.cpp
        src/platform/analog_clock_imgui.cpp
        src/platform/ui_app.cpp
        src/platform/ui_titlebar.cpp
        src/platform/ui_clock.cpp
        src/platform/ui_stopwatch.cpp
        src/platform/ui_timers.cpp
        src/platform/ui_alarms.cpp
        src/platform/ui_settings.cpp
        src/platform/main_loop.cpp
    )
    if(WIN32)
        list(APPEND CHRONOS_APP_SOURCES src/platform/platform_window_win32.cpp)
    else()
        list(APPEND CHRONOS_APP_SOURCES src/platform/platform_window_linux.cpp)
    endif()

    # Screenshot support (opt-in via -DCHRONOS_SCREENSHOT=ON)
    option(CHRONOS_SCREENSHOT "Enable --screenshot flag to save a PNG frame" ON)
    option(CHRONOS_DEBUG_UI_OVERLAY "Draw colored debug outlines for UI and resize regions" OFF)

    if(WIN32)
        add_executable(chronos WIN32 ${CHRONOS_APP_SOURCES})
        target_compile_definitions(chronos PRIVATE
            WIN32_LEAN_AND_MEAN NOMINMAX UNICODE _UNICODE
        )
        if(MINGW)
            target_link_options(chronos PRIVATE
                -static-libstdc++ -static-libgcc
            )
        endif()
        chronos_use_lld(chronos)
        if(TARGET SDL2::SDL2main)
            target_link_libraries(chronos PRIVATE SDL2::SDL2main)
        endif()
        target_link_libraries(chronos PRIVATE chronos_core imgui)
        if(MINGW)
            target_link_libraries(chronos PRIVATE -Wl,-Bstatic winpthread -Wl,-Bdynamic)
        endif()
        target_link_libraries(chronos PRIVATE shell32 advapi32)
    else()
        add_executable(chronos ${CHRONOS_APP_SOURCES})
        target_link_libraries(chronos PRIVATE chronos_core imgui)
        chronos_use_lld(chronos)
    endif()

    set_target_properties(chronos PROPERTIES CXX_SCAN_FOR_MODULES OFF)
    target_include_directories(chronos PRIVATE
        ${CHRONOS_INCLUDE_DIRS}
        ${IMGUI_DIR}
        ${IMGUI_DIR}/backends
        third_party
    )

    target_compile_definitions(chronos PRIVATE
        CHRONOS_CODICON_TTF="${CMAKE_SOURCE_DIR}/third_party/codicons/codicon.ttf"
        CHRONOS_PROGGYCLEAN_TTF="${CMAKE_SOURCE_DIR}/third_party/imgui/misc/fonts/ProggyClean.ttf"
    )
    if(CHRONOS_SCREENSHOT)
        target_compile_definitions(chronos PRIVATE CHRONOS_SCREENSHOT)
    endif()
    if(CHRONOS_DEBUG_UI_OVERLAY)
        target_compile_definitions(chronos PRIVATE CHRONOS_DEBUG_UI_OVERLAY)
    endif()
endif()

# Copy compile_commands.json to repo root after build so all editors find it
add_custom_target(copy_compile_commands ALL
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
        ${CMAKE_BINARY_DIR}/compile_commands.json
        ${CMAKE_SOURCE_DIR}/compile_commands.json
    COMMENT "Copying compile_commands.json to repo root"
)

# ─── Tests ────────────────────────────────────────────────────────────────────

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
                message(FATAL_ERROR "Catch2 not found.")
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
        tests/test_toolbar_strip.cpp
        tests/test_codicon_range.cpp
    )
    set_target_properties(chronos_tests PROPERTIES CXX_SCAN_FOR_MODULES OFF)
    target_include_directories(chronos_tests PRIVATE third_party)
    target_link_libraries(chronos_tests PRIVATE chronos_core Catch2::Catch2WithMain)
    chronos_use_lld(chronos_tests)
    if(WIN32)
        target_compile_definitions(chronos_tests PRIVATE
            WIN32_LEAN_AND_MEAN NOMINMAX UNICODE _UNICODE
        )
        if(MINGW)
            target_link_options(chronos_tests PRIVATE
                -static-libstdc++ -static-libgcc
            )
        endif()
    endif()

    list(APPEND CMAKE_MODULE_PATH ${catch2_SOURCE_DIR}/extras)
    include(Catch OPTIONAL)
    if(COMMAND catch_discover_tests)
        catch_discover_tests(chronos_tests)
    else()
        add_test(NAME chronos_tests COMMAND chronos_tests)
    endif()
endif()
