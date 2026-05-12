# Changelog

All notable changes to Chronos are documented here.
Format follows [Keep a Changelog](https://keepachangelog.com/en/1.1.0/).

## [Unreleased]

## [1.11.0] — 2026-05-12

### Added
- Settings panel **Clock** tab now uses a scrollable dropdown instead of standalone format buttons (#294)

### Fixed
- Light and dark mode buttons in the settings panel now select the correct theme (#292)
- Corrected scroll range calculation and `clock_names` array size in the settings panel

## [1.10.0] — 2026-05-12

### Added
- **Analog clock view** — fifth `ClockView` option drawn entirely with GDI: outer circle, hour/minute tick marks, hour/minute/second hands, and centre dot (#271)
- Analog clock uses a taller layout (`BASE_ANALOG_CLK_H = 120`); window height adjusts automatically when cycling to/from Analog
- Configurable analog appearance: hand colors, hand lengths (% of radius), hand thickness, tick lengths, show/hide minute ticks, hour labels (None / Sparse / Full)
- Analog options surfaced in **Clock** tab of the settings panel when Analog format is selected
- All analog style fields persisted to `config.ini` (prefixed `analog_*`); defaults derive from the active theme for a clean look out of the box
- Analog clock accessible via click-to-cycle on the clock area or the Clock tab format selector in settings

## [1.9.0] — 2026-05-12

### Added
- **Custom timer presets** — define up to 5 personal presets (in minutes) via the new **Timers** tab in the settings panel; presets appear in the right-click timer menu below the built-in durations (#270)
- Custom presets persisted to `config.ini` (`num_custom_presets`, `custom_preset0`–`custom_preset4`)

## [1.8.0] — 2026-05-12

### Added
- Configurable **long-break cadence** — set how many work sessions (1–10) before a long break via **Pomodoro** tab in the settings panel (default: 4) (#268)
- **Auto-start toggle** — when disabled, the timer pauses at 0 after each Pomodoro phase and waits for the user to manually start the next phase (#268)
- Both settings persisted to `config.ini` (`pomodoro_cadence`, `pomodoro_auto_start`)

### Changed
- Replaced the hardcoded 8-phase `PomodoroPhase` enum with a dynamic integer-based cycle driven by the cadence setting — phase labels adapt automatically (e.g. cadence 2 shows "Work 1/2", cadence 6 shows "Work 1/6")

### Fixed
- Fix Windows build failure caused by leftover reference to removed `PomodoroPhase` enum in right-click Pomodoro menu (#283)

## [1.7.0] — 2026-05-12

### Added
- Configurable notification sound on timer expiry — toggle on/off via **Appearance → Notifications** in the settings panel (default: on) (#267)
- Sound setting persisted to `config.ini` and respected by all timers including Pomodoro phase transitions

## [1.6.0] — 2026-05-12

### Added
- Settings button (⚙) in the top bar opens a categorised config panel with left sidebar navigation and right content pane (#266)
- **Appearance** tab: theme selector (Auto / Light / Dark) replaces the hidden `D` key shortcut for discoverability
- **Clock** tab: format selector (24h + seconds / 24h / 12h + seconds / 12h) replaces the hidden click-to-cycle interaction
- **Pomodoro** tab: work / short break / long break duration fields (same as the right-click dialog, now also accessible from the panel)
- Panel is extensible — new categories can be added without restructuring the dialog

## [1.5.2] — 2026-05-11

### Fixed
- Prevent extra empty space below the bottom widget when the window is snapped or resized on portrait (vertical) monitors — lock maximum window height to content height via `WM_GETMINMAXINFO` and correct oversized frames in `WM_SIZE` (#263)

## [1.5.1] — 2026-05-11

### Fixed
- Pomodoro dialog now opens centered on the parent window instead of the screen center (#273)
- Eliminated white outline artifacts on owner-drawn OK/Cancel buttons by clearing the button background before drawing rounded corners (#273)

### Added
- Pomodoro dialog title area is now draggable, allowing the user to reposition the popup (#273)

## [1.5.0] — 2026-05-11

### Changed
- Restyle Pomodoro interval dialog to match the app's dark/light theme — custom-painted background, themed labels, owner-drawn rounded buttons, and flat dark edit fields replace the default Windows dialog chrome (#265)

### Added
- Introduce reusable `DlgStyle` struct (`dialog_style.hpp`) for consistent owner-drawn dialog styling across future config panels

## [1.4.3] — 2026-04-29

### Changed
- Split `tests/test_actions.cpp` (565 lines) into four focused files: `test_actions_dispatch.cpp`, `test_actions_stopwatch.cpp`, `test_actions_timer.cpp`, `test_actions_adjust.cpp` — no tests modified, pure file split (#238)

## [1.4.2] — 2026-04-28

### Changed
- Extract H/M/S adjustment logic from `dispatch_action` into standalone `apply_timer_hms_adjust` function, shrinking the main dispatch to a switch-of-thin-handlers (#237)

## [1.4.1] — 2026-04-27

### Fixed
- Replace Unicode arrow (`→`) in four Pomodoro advance test names with ASCII `->` — the multi-byte character was mangled by Windows console encoding, causing CTest to fail when passing the name as a filter to the Catch2 executable (#261, #262)

## [1.4.0] — 2026-04-27

### Added
- Skip the current Pomodoro phase early with `N` key or the new **Skip** button — work phases credit actual elapsed time, breaks simply advance to the next phase without losing cycle position (#239)
- **Shift+R** keyboard shortcut to reset all active timers at once (#240)
- Raised `MAX_TIMERS` from 3 to 20; keyboard shortcuts now reach individual timers 1–9
- Pomodoro interval configuration dialog for adjusting work / short-break / long-break durations

### Changed
- Replaced bare-int Pomodoro phase tracking with a typed `PomodoroPhase` enum class, eliminating implicit-conversion bugs

### Fixed
- Corrected bare-int `PomodoroPhase` assignments in `input_mouse.hpp` left over from the enum migration
- Check `GetModuleFileNameW` return value in `main.cpp` and `config_io.hpp`

## [1.3.0] — 2026-04-26

### Added
- Stopwatch display now preserves millisecond precision (`HH:MM:SS.mmm`) past 1 hour — previously the display silently dropped sub-second resolution at the 1-hour mark; lap row cumulative totals also switch to the long format when they exceed 1 hour (#241)

## [1.2.0] — 2026-04-25

### Added
- Title bar and tray balloon now identify which timer expired when multiple timers are active — e.g. `Tea (2/3) 04:32 — Chronos` while running, `EXPIRED · Tea (2/3) — Chronos` on expiry; single-timer setups are unchanged (#242)

## [1.1.4] — 2026-04-25

### Fixed
- `A_SW_GET` now validates that the lap file exists on disk before opening it — stale paths from a previous session no longer trigger a silent `ShellExecuteW` failure; the path is cleared and the "Get Laps" button dims instead (#235)

## [1.1.3] — 2026-04-25

### Fixed
- `RegisterHotKey` return value is now checked — when `Ctrl+Shift+Space` is already claimed by another application, the help overlay shows "(unavailable)" instead of silently advertising a non-functional shortcut (#236)

## [1.1.2] — 2026-04-25

### Fixed
- `try_start_label_edit` now guards against `CreateWindowExW` returning null, preventing undefined behavior from calling `SendMessageW`, `SetWindowLongPtrW`, and `CallWindowProcW` on a null HWND (#233)

## [1.1.1] — 2026-04-24

### Fixed
- Letter-key shortcuts (L, E, C, R, T, D, P, H) and digit/symbol keys (1-3, +, −) no longer fire when Ctrl, Alt, or Win modifiers are held — prevents accidental state changes from system shortcuts like Ctrl+C (#234)

## [1.1.0] — 2026-04-23

### Added
- **P** keyboard shortcut to toggle Pomodoro mode on the first idle timer (#219)
- **Shift+1** / **Shift+2** / **Shift+3** keyboard shortcuts to reset individual timers

## [1.0.12] — 2026-04-22

### Removed
- Deleted `auto-tag.yml` workflow — tags are now created manually and `release.yml` already handles tag-triggered releases (#232)

## [1.0.11] — 2026-04-22

### Changed
- Split `config.hpp` into struct definition (`config.hpp`) and serialization (`config_serial.hpp`) so headers that only need the `Config` type no longer pull in `<istream>`, `<ostream>`, `<format>`, and `<charconv>` (#225)

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

[Unreleased]: https://github.com/andreasnonslid/chronos/compare/v1.11.0...HEAD
[1.11.0]: https://github.com/andreasnonslid/chronos/compare/v1.10.0...v1.11.0
[1.10.0]: https://github.com/andreasnonslid/chronos/compare/v1.9.0...v1.10.0
[1.9.0]: https://github.com/andreasnonslid/chronos/compare/v1.8.0...v1.9.0
[1.8.0]: https://github.com/andreasnonslid/chronos/compare/v1.7.0...v1.8.0
[1.7.0]: https://github.com/andreasnonslid/chronos/compare/v1.6.0...v1.7.0
[1.6.0]: https://github.com/andreasnonslid/chronos/compare/v1.5.2...v1.6.0
[1.5.2]: https://github.com/andreasnonslid/chronos/compare/v1.5.1...v1.5.2
[1.5.1]: https://github.com/andreasnonslid/chronos/compare/v1.5.0...v1.5.1
[1.5.0]: https://github.com/andreasnonslid/chronos/compare/v1.4.3...v1.5.0
[1.4.3]: https://github.com/andreasnonslid/chronos/compare/v1.4.2...v1.4.3
[1.4.2]: https://github.com/andreasnonslid/chronos/compare/v1.4.1...v1.4.2
[1.4.1]: https://github.com/andreasnonslid/chronos/compare/v1.4.0...v1.4.1
[1.4.0]: https://github.com/andreasnonslid/chronos/compare/v1.3.0...v1.4.0
[1.3.0]: https://github.com/andreasnonslid/chronos/compare/v1.2.0...v1.3.0
[1.2.0]: https://github.com/andreasnonslid/chronos/compare/v1.1.4...v1.2.0
[1.1.4]: https://github.com/andreasnonslid/chronos/compare/v1.1.3...v1.1.4
[1.1.3]: https://github.com/andreasnonslid/chronos/compare/v1.1.2...v1.1.3
[1.1.2]: https://github.com/andreasnonslid/chronos/compare/v1.1.1...v1.1.2
[1.1.1]: https://github.com/andreasnonslid/chronos/compare/v1.1.0...v1.1.1
[1.1.0]: https://github.com/andreasnonslid/chronos/compare/v1.0.12...v1.1.0
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
