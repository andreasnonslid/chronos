# Chronos

A lightweight Windows desktop clock, stopwatch, and timer app. Built with C++26 and the Win32 API.

## Features

- **Clock** — system time display (HH:MM:SS)
- **Stopwatch** — start/stop, lap recording with split and cumulative times, exports laps to a text file
- **Timers** — up to 3 simultaneous countdown timers with custom labels, progress bars, and color-coded warnings (orange <10s, red expired)

All sections can be toggled on/off. The window can be pinned always-on-top. Settings, window position, and timer durations persist across sessions via `%APPDATA%/Chronos/config.ini`.

## Usage

### Keyboard Shortcuts

| Key | Action |
|-----|--------|
| `Space` | Start/stop stopwatch (or timer 1 if stopwatch is hidden) |
| `Ctrl+Shift+Space` | Global start/stop (works from any application) |
| `L` | Lap (stopwatch) |
| `R` | Reset stopwatch (or timer 1 if stopwatch is hidden) |
| `T` | Toggle always-on-top |
| `D` | Cycle theme (auto / light / dark) |
| `1` / `2` / `3` | Start/pause timer by index |
| `+` | Add a timer (up to 3) |
| `-` | Remove last timer (minimum 1) |
| `E` | Open exported laps file |
| `C` | Copy laps to clipboard |
| `H` or `?` | Toggle help overlay |

### Mouse

- **Scroll wheel** on a timer: scroll over the left third of the timer to adjust hours, middle third for minutes, right third for seconds
- **Double-click** timer label: edit label
- **Right-click** timer: duration presets

### System Tray

Minimizing the window sends it to the system tray. Right-click the tray icon to show or exit.

## Building

Requires [MSYS2](https://www.msys2.org/) with the LLVM/MinGW64 toolchain:

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

### Tests

```
cmake -S . -B build -G Ninja -DCMAKE_CXX_COMPILER=clang++ -DBUILD_TESTS=ON
cmake --build build
./build/chronos_tests
```

## Debugging

Run with `--debug` to write logs to `debug.log` in the executable directory.

## Project Structure

```
src/
  main.cpp          — entry point, Win32 window class, message loop
  app.ixx           — app state (stopwatch, timers, visibility flags)
  window.ixx        — Win32 message handling (WM_PAINT, input, DPI, tray)
  actions.ixx       — action dispatch (button/key events → state changes)
  painting.ixx      — GDI rendering for all UI sections
  painting_timer.ixx — timer-specific rendering (progress bars, color warnings)
  paint_ctx.ixx     — paint context helpers
  layout.ixx        — DPI-scaled dimensions and client height calculations
  config.ixx        — settings struct (positions, timers, labels)
  config_io.ixx     — INI config read/write to %APPDATA%/Chronos/config.ini
  input.ixx         — keyboard/mouse input dispatch
  timer.ixx         — countdown timer logic (steady_clock based)
  stopwatch.ixx     — stopwatch logic and lap tracking
  formatting.ixx    — time display formatting (HH:MM:SS, MM:SS.mmm, etc.)
  theme.ixx         — OS dark/light mode detection and color schemes
  wndstate.ixx      — window state (fonts, GDI resources, double-buffer)
  tray.ixx          — system tray icon and context menu
  icon.ixx          — programmatic clock icon generation
  gdi.ixx           — RAII wrappers for GDI/HDC/HICON objects
  encoding.ixx      — UTF-8/UTF-16 string conversion utilities
  dpi.ixx           — DPI awareness setup
  polling.ixx       — window invalidation/timer polling loop
  debug.ixx         — debug logging to file
tests/
  test_config.cpp, test_timer.cpp, test_stopwatch.cpp,
  test_formatting.cpp, test_layout.cpp, test_actions.cpp,
  test_encoding.cpp
```

## License

MIT
