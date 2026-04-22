# Changelog

All notable changes to Chronos are documented here.
Format follows [Keep a Changelog](https://keepachangelog.com/en/1.1.0/).

## [1.0.12] — 2026-04-22

### Removed
- Deleted `auto-tag.yml` workflow — tags are now created manually and `release.yml` already handles tag-triggered releases (#232)

## [1.0.11] — 2026-04-22

### Changed
- Split `config.hpp` into struct definition (`config.hpp`) and serialization (`config_serial.hpp`) so headers that only need the `Config` type no longer pull in `<istream>`, `<ostream>`, `<format>`, and `<charconv>` (#225)

## [Unreleased]

### Added
- Pomodoro mode for timer slots — right-click any untouched timer and choose Pomodoro to run an auto-cycling 25/5 × 4 + 15-minute sequence
- Configurable Pomodoro phase durations via `config.ini` keys
- Global hotkey `Ctrl+Shift+Space` to start/stop from any application
- Clipboard copy for stopwatch laps (`C` key) — same format as file export
- Title briefly shows "Copied — Chronos" for 1 s after clipboard copy
- `E` key shortcut to open the exported laps file
- Offensive programming assertions across internal modules — `Timer`, `Stopwatch`, `Pomodoro`, layout, painting, and action dispatch now enforce preconditions via `CHRONOS_ASSERT` / `CHRONOS_UNREACHABLE` in debug builds (#215)
- `static_assert` tying `CLOCK_VIEW_COUNT` to `ClockView` enum to catch desync at compile time
- `POMODORO_PHASE_COUNT` constant replacing magic number `8` in phase arithmetic

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

## [1.0.10] — 2026-04-21

### Changed
- Internal modules (`Timer`, `Stopwatch`, `Pomodoro`, layout, painting, action dispatch) now enforce preconditions via `CHRONOS_ASSERT` / `CHRONOS_UNREACHABLE` in debug builds — offensive programming philosophy (#215)
- `POMODORO_PHASE_COUNT` constant replaces magic number `8` in phase arithmetic
- `static_assert` ties `CLOCK_VIEW_COUNT` to `ClockView` enum to catch desync at compile time

## [1.0.9] — 2026-04-20

### Changed
- App icon rewritten with 32-bit ARGB for proper per-pixel alpha transparency (#231)
- Cleaner minimal clock design with anti-aliased ring, tick marks, and hands

## [1.0.8] — 2026-04-19

### Fixed
- Config parser now clamps numeric values at `long long` before narrowing to `int`, preventing silent wrap-around on values like 2^32+60 that previously bypassed range checks (#227)

### Added
- Nine new test cases for config parsing edge cases: integer overflow, int-wrapping values, epoch zero with running state, duplicate keys, empty labels, and whitespace around delimiters (#227)

## [1.0.7] — 2026-04-19

### Added
- Test coverage for `A_THEME` action dispatch — verifies theme mode cycling (Auto→Dark→Light→Auto) and correct `HandleResult` flags (#217)

## [1.0.6] — 2026-04-19

### Fixed
- Release workflow now warns and uses a fallback body when no changelog section matches the tag version or `[Unreleased]`, instead of silently creating a release with an empty description (#218)

## [1.0.5] — 2026-04-19

### Fixed
- Added `static_assert` in non-Windows encoding path to catch platforms where `wchar_t` is not 4 bytes at compile time, preventing silent UTF-8 corruption (#223)

## [1.0.4] — 2026-04-17

### Changed
- CI log artifact retention reduced from 90 days (default) to 7 days to save storage; the release binary artifact keeps its default retention (#222)

## [1.0.3] — 2026-04-16

### Fixed
- `config_read` now clamps runtime-state `sw_elapsed_ms`, `sw_start_epoch_ms`, `timer{i}_elapsed_ms`, and `timer{i}_start_epoch_ms` to `>= 0`, so a hand-edited or corrupted config cannot produce negative epoch or elapsed values that cascade into bogus restored timing state (#224)

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

[Unreleased]: https://github.com/andreasnonslid/chronos/compare/v1.0.12...HEAD
[1.0.12]: https://github.com/andreasnonslid/chronos/compare/v1.0.11...v1.0.12
[1.0.11]: https://github.com/andreasnonslid/chronos/compare/v1.0.10...v1.0.11
[1.0.10]: https://github.com/andreasnonslid/chronos/compare/v1.0.9...v1.0.10
[1.0.9]: https://github.com/andreasnonslid/chronos/compare/v1.0.8...v1.0.9
[1.0.8]: https://github.com/andreasnonslid/chronos/compare/v1.0.7...v1.0.8
[1.0.7]: https://github.com/andreasnonslid/chronos/compare/v1.0.6...v1.0.7
[1.0.6]: https://github.com/andreasnonslid/chronos/compare/v1.0.5...v1.0.6
[1.0.5]: https://github.com/andreasnonslid/chronos/compare/v1.0.4...v1.0.5
[1.0.4]: https://github.com/andreasnonslid/chronos/compare/v1.0.3...v1.0.4
[1.0.3]: https://github.com/andreasnonslid/chronos/compare/v1.0.2...v1.0.3
[1.0.2]: https://github.com/andreasnonslid/chronos/compare/v1.0.1...v1.0.2
[1.0.1]: https://github.com/andreasnonslid/chronos/compare/v1.0.0...v1.0.1
[1.0.0]: https://github.com/andreasnonslid/chronos/releases/tag/v1.0.0
