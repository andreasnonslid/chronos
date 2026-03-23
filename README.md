# Chronos

A lightweight Windows desktop clock, stopwatch, and timer app. Built with C++26 and the Win32 API.

## Features

- **Clock** — system time display (HH:MM:SS)
- **Stopwatch** — start/stop, lap recording with split and cumulative times, exports laps to a text file
- **Timers** — up to 3 simultaneous countdown timers with progress bars and color-coded warnings

All sections can be toggled on/off. The window can be pinned always-on-top. Settings and timer durations persist across sessions via a `config.ini` file stored next to the executable.

## Building

Requires [MSYS2](https://www.msys2.org/) with the MinGW64 toolchain:

```
pacman -S mingw-w64-x86_64-clang mingw-w64-x86_64-lld mingw-w64-x86_64-gcc mingw-w64-x86_64-cmake mingw-w64-x86_64-ninja
```

Build with [Just](https://github.com/casey/just):

```
just build
just run
```

Or directly with CMake:

```
cmake -S . -B build -G Ninja -DCMAKE_BUILD_TYPE=Release -DCMAKE_CXX_COMPILER=clang++
cmake --build build
```

Output: `build/chronos.exe`

## Debugging

Run with `--debug` to write logs to `debug.log` in the executable directory.

## Project Structure

```
src/
  main.cpp       — entry point, window creation, rendering (Win32 GDI)
  config.ixx     — config persistence (INI format)
  formatting.ixx — shared stopwatch/timer/lap display formatting helpers
  layout.ixx     — shared layout and client-height calculations
  stopwatch.ixx  — stopwatch logic and lap tracking
  timer.ixx      — countdown timer logic
```

## License

MIT
