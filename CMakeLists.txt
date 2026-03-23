cmake_minimum_required(VERSION 3.28)
project(chronos LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 26)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

find_program(LLD NAMES ld.lld ld.lld.exe REQUIRED)

add_executable(chronos WIN32 src/main.cpp)

target_sources(chronos PRIVATE
    FILE_SET CXX_MODULES FILES
        src/config.ixx
        src/stopwatch.ixx
        src/timer.ixx
)

target_compile_options(chronos PRIVATE
    -Wall -Wextra -Wno-unused-command-line-argument
)

target_link_options(chronos PRIVATE
    "--ld-path=${LLD}"
    -static-libstdc++
    -static-libgcc
)

target_link_libraries(chronos PRIVATE
    -Wl,-Bstatic winpthread -Wl,-Bdynamic
    gdi32 user32 dwmapi shell32 advapi32
)
