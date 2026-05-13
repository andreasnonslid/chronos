# Chronos

A lightweight Windows desktop clock, stopwatch, and timer app. Built with C++26 and the Win32 API — no frameworks, no runtime dependencies.

## Features

- **Clock** — system time (HH:MM:SS)
- **Stopwatch** — start/stop, lap recording with split and cumulative times, export laps to file or clipboard
- **Timers** — up to 20 simultaneous countdown timers with custom labels, progress bars, color-coded warnings (orange <10 s, red on expiry), and audio/visual notification on completion
- **Pomodoro** — per-timer pomodoro mode cycling Work (25 m) / Short Break (5 m) x4, then Long Break (15 m)
- **Themes** — auto / light / dark, follows OS preference in auto mode
- **System tray** — minimizes to tray with balloon notifications
- **Persistence** — window position, timer durations, labels, and settings saved to `%APPDATA%/Chronos/config.ini`
- **Single instance** — only one window at a time, via mutex
- **DPI aware** — scales to per-monitor DPI

## Usage

### Keyboard

| Key | Action |
|-----|--------|
| `Space` | Start/stop stopwatch (or timer 1 if stopwatch hidden) |
| `Ctrl+Shift+Space` | Global start/stop (works from any application) |
| `L` | Record lap |
| `R` | Reset stopwatch (or timer 1 if stopwatch hidden) |
| `1`–`9` | Start/pause timer by index (first 9 timers) |
| `+` | Add a timer (max 20) |
| `-` | Remove last timer (min 1) |
| `E` | Open exported laps file |
| `C` | Copy laps to clipboard |
| `T` | Toggle always-on-top |
| `D` | Cycle theme |
| `H` or `?` | Toggle help overlay |

### Mouse

- **Scroll wheel** on a timer — left third adjusts hours, middle third minutes, right third seconds
- **Double-click** timer label — edit label
- **Right-click** untouched timer — duration presets or enable Pomodoro mode

### System Tray

Minimizing sends the window to the tray. Right-click the tray icon to show or exit.

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

Or directly with CMake (using [presets](CMakePresets.json)):

```
cmake --preset release
cmake --build build
```

Output: `build/chronos.exe`

### Tests

Unit tests use [Catch2](https://github.com/catchorg/Catch2) (fetched automatically):

```
cmake --preset test
cmake --build build
./build/chronos_tests
```

Other presets for development:

| Preset | Purpose |
|--------|---------|
| `debug` | Debug build with tests |
| `sanitize` | Debug + AddressSanitizer / UndefinedBehaviorSanitizer |

### Platform notes

The primary application is the **Windows** desktop UI, implemented with Win32 and GDI.

Linux builds use an experimental X11/Xlib backend that opens a minimal Chronos window and renders the current clock. It is intentionally smaller than the Windows UI while the platform layer is being split out. The logic-portable layer (timers, stopwatch, config serialization, Pomodoro state machine, formatting) has no Win32 dependencies and compiles on Linux. CI runs the unit test suite on Linux to verify this layer independently of the Windows UI.

### Debugging

Run with `--debug` to write diagnostic logs to `debug.log` next to the executable.

## Project Structure

```
src/
  main.cpp            entry point, Win32 window class, message loop
  app.hpp             app state: stopwatch, timer slots, visibility flags, theme
  window.hpp          WndProc — create/destroy, paint, sizing, DPI, tray, hotkeys
  actions.hpp         action enum and dispatch (key/button events → state changes)
  input.hpp           top-level input dispatch
  input_keyboard.hpp  keyboard handling
  input_mouse.hpp     mouse and hit-test handling
  input_core.hpp      shared input helpers
  painting.hpp        GDI rendering for clock, stopwatch, help overlay
  painting_timer.hpp  timer row rendering (progress bars, color warnings)
  paint_ctx.hpp       paint context and button helpers
  layout.hpp          DPI-scaled dimensions, client height calculations
  geometry.hpp        window sizing and non-client height utilities
  wndstate.hpp        window state: fonts, brushes, double-buffer, tray icon
  config.hpp          Config struct and INI serialization
  config_io.hpp       config file path resolution and App ↔ Config sync
  timer.hpp           countdown timer (steady_clock based)
  stopwatch.hpp       stopwatch with lap tracking
  pomodoro.hpp        pomodoro phase durations and labels
  polling.hpp         WM_TIMER polling, timer expiry notifications
  theme.hpp           light/dark palettes, DWM dark-mode integration
  formatting.hpp      time display formatting (HH:MM:SS, MM:SS.mmm, etc.)
  tray.hpp            system tray icon, context menu, balloon notifications
  icon.hpp            programmatic clock icon generation
  gdi.hpp             RAII wrappers for GDI objects, HDC, HICON
  dpi.hpp             per-monitor DPI awareness setup
  dwm_fwd.hpp         DWM forward declarations (MinGW header workaround)
  encoding.hpp        UTF-8 / UTF-16 conversion
  debug.hpp           optional file logging (--debug flag)
tests/
  test_timer.cpp      test_stopwatch.cpp   test_config.cpp
  test_encoding.cpp   test_formatting.cpp  test_layout.cpp
  test_actions.cpp
```

## License

MIT
