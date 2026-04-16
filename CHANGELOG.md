# Changelog

All notable changes to Chronos are documented here.
Format follows [Keep a Changelog](https://keepachangelog.com/en/1.1.0/).

## [Unreleased]

### Added
- Pomodoro mode for timer slots — right-click any untouched timer and choose Pomodoro to run an auto-cycling 25/5 × 4 + 15-minute sequence
- Configurable Pomodoro phase durations via `config.ini` keys
- Global hotkey `Ctrl+Shift+Space` to start/stop from any application
- Clipboard copy for stopwatch laps (`C` key) — same format as file export
- Title briefly shows "Copied — Chronos" for 1 s after clipboard copy
- `E` key shortcut to open the exported laps file

### Fixed
- Stopwatch lap count capped at 999 to prevent unbounded memory growth (#175)
- Negative durations clamped to zero in all formatting functions (#180)
- Timer progress bar fill width clamped to prevent negative `RECT` (#191)
- `RegisterClassExW` / `CreateWindowExW` return values now guarded (#192)
- Orphaned `.tmp` config file cleaned up when write fails
- Unhandled `WM_KEYDOWN` / `WM_CHAR` messages now propagate to `DefWindowProcW`
- Invalid UTF-8 lead bytes `0xF5`–`0xFF` rejected in `utf8_to_wide`
- `NOMINMAX` macro redefinition warnings fixed via CMake compile definitions
- Pomodoro: phase clamped to valid range on config load
- Pomodoro: scroll wheel disabled on Pomodoro timers to prevent value changes
- Pomodoro: arrow buttons hidden in idle mode to prevent title overlap
- Pomodoro: label cleared when switching back to a normal preset
- Pomodoro: idle time display expanded to fill space left by hidden buttons

## [1.0.2] — 2026-04-16

### Changed
- `Config` Pomodoro defaults and the write-skip guard now reference the named constants in `pomodoro.hpp` instead of duplicating literal `25 * 60` / `5 * 60` / `15 * 60` values, so the three call sites stay in sync if defaults ever change (#220)

## [1.0.1] — 2026-04-14

### Changed
- CI now runs all four test jobs (Windows, Linux release, Linux debug, sanitizer) on pull requests, not just pushes

## [1.0.0] — 2026-03-19

Initial public release.

### Added
- Clock showing system time (HH:MM:SS)
- Stopwatch with lap recording, split/cumulative display, and file export
- Up to 3 simultaneous countdown timers with custom labels
- Timer progress bars with color-coded warnings (orange < 10 s, red on expiry)
- Audio and visual notification on timer completion
- Pomodoro-ready timer preset menu via right-click
- Light / dark / auto theme following OS preference
- Minimize to system tray with balloon notifications
- Window position, timer state, and settings persisted to `%APPDATA%/Chronos/config.ini`
- Single-instance enforcement via named mutex
- Per-monitor DPI awareness
- Keyboard shortcuts (`Space`, `L`, `R`, `1`/`2`/`3`, `+`, `-`, `T`, `D`, `H`/`?`)
- Mouse-wheel timer adjustment (left third = hours, middle = minutes, right = seconds)
- Inline timer label editing via double-click
- Duration presets via right-click context menu
- Programmatic app icon for taskbar and title bar
- `--debug` flag writing diagnostic logs to `debug.log`
- MIT license

[Unreleased]: https://github.com/andreasnonslid/chronos/compare/v1.0.2...HEAD
[1.0.2]: https://github.com/andreasnonslid/chronos/compare/v1.0.1...v1.0.2
[1.0.1]: https://github.com/andreasnonslid/chronos/compare/v1.0.0...v1.0.1
[1.0.0]: https://github.com/andreasnonslid/chronos/releases/tag/v1.0.0
