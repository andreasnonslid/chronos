#!/usr/bin/env bash
set -euo pipefail

# Chronos — curated GitHub issues
# Run: gh auth login && bash create_issues.sh
# Each issue is idempotent-safe (gh will warn if title already exists)

REPO="andreasnonslid/chronos"

echo "Creating issues for $REPO..."
echo ""

# ── 1. Bug: Timer adjustment silently clamped at the 24-hour boundary ─────────
gh issue create --repo "$REPO" \
  --title "Bug: Timer adjustment silently clamped at the 24-hour boundary" \
  --label "bug,good first issue" \
  --body "$(cat <<'EOF'
## Problem

When hours = 24 and a user scrolls the minutes or seconds column, the field visually increments but `std::clamp` in `actions.hpp:201` silently forces the total back to 86 400 s (24:00:00). The scroll appears to do nothing, which is confusing.

The same occurs in reverse: scrolling hours down from 0 wraps to 24, and any pre-existing minutes/seconds are silently lost to clamping.

## Location

`src/actions.hpp:186-206` — the `Adj` table sets `max_val` for hours to 24 independently of minutes/seconds, then the combined value is clamped.

## Suggested fix

When `h == 24`, force `m` and `sec` to 0 before writing `ts.dur` — or cap `h` at 23 and allow 23:59:59 as the real maximum, matching the wrap table. Either approach keeps the UI honest.
EOF
)"
echo "  ✓ Issue 1 created"

# ── 2. Bug: Negative elapsed_ms in a hand-edited config is not rejected ───────
gh issue create --repo "$REPO" \
  --title "Bug: Negative elapsed_ms from corrupted config is not validated" \
  --label "bug,good first issue" \
  --body "$(cat <<'EOF'
## Problem

`config_io.hpp:134` reads `elapsed_ms` as `long long` directly from the config file. A hand-edited or corrupted config with a negative value (e.g., `sw_elapsed_ms=-5000`) is passed straight through to `Stopwatch::restore()` / `Timer::restore()`, producing a negative duration.

The stopwatch and timer code do not guard against negative input — `restore()` trusts its caller.

## Location

- `src/config_io.hpp:120` (`actual_ms` for stopwatch)
- `src/config_io.hpp:134` (`elapsed_ms` for timers)

## Suggested fix

Clamp `elapsed_ms` and `actual_ms` to `>= 0` immediately after computing them, before passing to `restore()`. One-line fix per location:

```cpp
actual_ms = std::max(actual_ms, 0LL);
```
EOF
)"
echo "  ✓ Issue 2 created"

# ── 3. Enhancement: Replace global state in label-edit subclass ───────────────
gh issue create --repo "$REPO" \
  --title "Replace global state in label-edit subclass with per-window properties" \
  --label "enhancement" \
  --body "$(cat <<'EOF'
## Problem

`input_label_edit.hpp:9-10` stores the original `WNDPROC` and the cancel flag as anonymous-namespace globals:

```cpp
WNDPROC g_orig_edit_proc = nullptr;
bool    g_edit_cancelled  = false;
```

This works today because only one edit control exists at a time, but it is fragile and non-idiomatic. If a second edit is ever created (e.g., a future rename-all feature), these globals silently collide.

## Suggested fix

Use `SetPropW` / `GetPropW` to attach both values to the HWND of the edit control, or use `SetWindowSubclass` / `RemoveWindowSubclass` (comctl32) which manages the original proc automatically. Either removes the globals entirely.
EOF
)"
echo "  ✓ Issue 3 created"

# ── 4. Enhancement: Add static_assert for wchar_t size on non-Windows ─────────
gh issue create --repo "$REPO" \
  --title "Add static_assert for wchar_t size assumption in encoding.hpp" \
  --label "enhancement,good first issue" \
  --body "$(cat <<'EOF'
## Problem

The non-Windows path in `encoding.hpp` (`#else` branch, lines 32-80) treats each `wchar_t` as a full UTF-32 codepoint by casting it to `uint32_t`. This is correct on Linux/macOS (where `wchar_t` is 4 bytes) but would silently produce garbled output on any platform where `wchar_t` is 2 bytes.

## Suggested fix

Add a single static assertion at the top of the `#else` block:

```cpp
static_assert(sizeof(wchar_t) == 4, "Non-Windows encoding assumes wchar_t is UTF-32 (4 bytes)");
```

One line, zero runtime cost, prevents a class of silent-corruption bugs.
EOF
)"
echo "  ✓ Issue 4 created"

# ── 5. Enhancement: Validate all runtime state fields on config load ──────────
gh issue create --repo "$REPO" \
  --title "Validate runtime state fields defensively on config load" \
  --label "enhancement" \
  --body "$(cat <<'EOF'
## Problem

`config_read()` validates some fields (e.g., `num_timers` is clamped, `win_w` has a minimum, `pomodoro_phase` is modulo 8) but several runtime-state fields are trusted verbatim:

| Field | Risk |
|---|---|
| `sw_elapsed_ms` | Negative → negative stopwatch duration |
| `sw_start_epoch_ms` | 0 or negative → huge delta on restore |
| `timer_elapsed_ms[i]` | Negative → negative remaining time |
| `timer_start_epoch_ms[i]` | Same as stopwatch epoch |
| `timer_secs[i]` | Already clamped ✓ |

A hand-edited or corrupted config file can produce surprising runtime behavior.

## Suggested fix

In `config_read()`, clamp all `_ms` fields to `>= 0` and all `_epoch_ms` fields to `>= 0`:

```cpp
c.sw_elapsed_ms = std::max(c.sw_elapsed_ms, 0LL);
c.sw_start_epoch_ms = std::max(c.sw_start_epoch_ms, 0LL);
// same for timer arrays
```

Alternatively, validate in `load_config()` after computing deltas.
EOF
)"
echo "  ✓ Issue 5 created"

# ── 6. Refactor: Split config.hpp into struct and serialization ───────────────
gh issue create --repo "$REPO" \
  --title "Split config.hpp into struct definition and serialization" \
  --label "refactor" \
  --body "$(cat <<'EOF'
## Motivation

`config.hpp` (160 lines) combines three concerns:

1. The `Config` struct and its constants (lines 11-42)
2. `config_write()` — INI serialization (lines 44-77)
3. `config_read()` — INI deserialization (lines 79-159)

The struct is included by many headers that only need the type and constants (e.g., `input_label_edit.hpp` only needs `MAX_LABEL_LEN`). Pulling in the parser brings `<charconv>`, `<istream>`, `<ostream>`, `<format>` along for the ride.

## Suggested split

- **`config.hpp`** — `Config` struct, `ThemeMode` enum, constants only. Lightweight, widely includable.
- **`config_serial.hpp`** — `config_read()` and `config_write()`, includes `config.hpp` + I/O headers.

`config_io.hpp` (save/load) already includes `config.hpp`; it would switch to `config_serial.hpp` — no other files change.

## Benefit

Cleaner dependency graph, faster incremental compiles, each file has a single responsibility.
EOF
)"
echo "  ✓ Issue 6 created"

# ── 7. Testing: Timer duration adjustment edge cases ──────────────────────────
gh issue create --repo "$REPO" \
  --title "Add tests for timer duration adjustment edge cases" \
  --label "testing,good first issue" \
  --body "$(cat <<'EOF'
## Gap

`test_actions.cpp` tests basic H/M/S increment/decrement and wrap-around, but does not cover:

- **24h boundary interaction**: incrementing minutes when hours = 24 (clamped to 86 400)
- **Zero-duration start**: calling `A_TMR_START` when duration is 0:00:00 (should be a no-op per `actions.hpp:152`)
- **Wrap + clamp compound**: scrolling hours down from 0 (wraps to 24) when minutes > 0
- **All three fields at max**: setting 24:59:59 via individual field adjustments (result should clamp)

## Suggested tests

```cpp
TEST_CASE("Timer adjustment clamped at 24h boundary") {
    App app; app.timers.resize(1);
    app.timers[0].dur = std::chrono::seconds{86400}; // 24:00:00
    app.timers[0].t.set(app.timers[0].dur);
    auto now = std::chrono::steady_clock::now();
    dispatch_action(app, tmr_act(0, A_TMR_MUP), now, ".");
    CHECK(app.timers[0].dur.count() == 86400); // still clamped
}

TEST_CASE("Zero-duration timer cannot start") {
    App app; app.timers.resize(1);
    app.timers[0].dur = std::chrono::seconds{0};
    app.timers[0].t.set(app.timers[0].dur);
    auto now = std::chrono::steady_clock::now();
    dispatch_action(app, tmr_act(0, A_TMR_START), now, ".");
    CHECK_FALSE(app.timers[0].t.is_running());
}
```
EOF
)"
echo "  ✓ Issue 7 created"

# ── 8. Testing: Config parsing with malformed / extreme values ────────────────
gh issue create --repo "$REPO" \
  --title "Add tests for config parsing with extreme and malformed values" \
  --label "testing" \
  --body "$(cat <<'EOF'
## Gap

`test_config.cpp` covers round-trip serialization, partial configs, and basic clamping, but does not exercise:

- **Integer overflow**: values like \`timer0=99999999999999999\` (exceeds \`int\` range after \`long long\` parse)
- **Negative runtime values**: \`sw_elapsed_ms=-1000\`, \`timer0_elapsed_ms=-500\`
- **Epoch edge cases**: \`sw_start_epoch_ms=0\` with \`sw_running=1\`
- **Duplicate keys**: same key appearing twice (last-wins? first-wins?)
- **Empty values**: \`timer0_label=\` (empty string label)
- **Whitespace**: \`timer0 = 60\` (spaces around \`=\`)

These matter because the config file is user-editable plaintext and the app should handle anything gracefully.

## Suggested scope

~6-8 new test cases in `test_config.cpp` validating that `config_read` either clamps or ignores each malformed input without crashing or producing invalid state.
EOF
)"
echo "  ✓ Issue 8 created"

# ── 9. CI: Add code coverage reporting ────────────────────────────────────────
gh issue create --repo "$REPO" \
  --title "CI: Add code coverage reporting" \
  --label "ci,enhancement" \
  --body "$(cat <<'EOF'
## Motivation

The CI runs 148 tests (336 assertions) but there is no visibility into which code paths are actually exercised. Coverage data would:

- Reveal untested paths (e.g., error handling in \`config_io.hpp\`, theme switching, tray notification)
- Prevent coverage regressions as new features are added
- Guide where to invest test effort

## Suggested implementation

Add a coverage job (or extend the existing Linux debug job) using \`llvm-cov\`:

1. Compile with \`-fprofile-instr-generate -fcoverage-mapping\`
2. Run \`chronos_tests\`
3. Merge raw profiles: \`llvm-profdata merge\`
4. Generate report: \`llvm-cov report\` (text summary in CI logs)
5. Optionally export \`llvm-cov export --format=lcov\` for a coverage badge or Codecov integration

Since the project uses Clang throughout, \`llvm-cov\` is a natural fit with zero new dependencies.

## Non-goal

Do not enforce a hard coverage threshold — this is a personal project. The value is visibility, not gatekeeping.
EOF
)"
echo "  ✓ Issue 9 created"

# ── 10. Feature: Keyboard shortcuts for per-timer reset ───────────────────────
gh issue create --repo "$REPO" \
  --title "Keyboard shortcuts for per-timer reset" \
  --label "feature" \
  --body "$(cat <<'EOF'
## Context

\`1\`, \`2\`, \`3\` start/stop individual timers, but \`R\` always resets the stopwatch (if visible) or only the first timer. There is no keyboard path to reset timer 2 or 3 individually.

## Suggestion

Map \`Shift+1\`, \`Shift+2\`, \`Shift+3\` to reset the corresponding timer. This mirrors the existing start/stop pattern and is discoverable via the help overlay.

## Implementation

In \`input_keyboard.hpp\`, add a \`WM_KEYDOWN\` case for \`'1'..'3'\` when \`GetKeyState(VK_SHIFT) < 0\`:

```cpp
case '1': case '2': case '3': {
    int idx = (int)(wp - '1');
    if (GetKeyState(VK_SHIFT) < 0) {
        if (s.app.show_tmr && idx < (int)s.app.timers.size())
            handle(hwnd, tmr_act(idx, A_TMR_RST), s);
    } else {
        if (s.app.show_tmr && idx < (int)s.app.timers.size())
            handle(hwnd, tmr_act(idx, A_TMR_START), s);
    }
    return 0;
}
```

Update the help overlay in \`painting.hpp\` to document the new shortcut.
EOF
)"
echo "  ✓ Issue 10 created"

# ── 11. Refactor: Extract window title update from polling loop ───────────────
gh issue create --repo "$REPO" \
  --title "Extract window-title update logic from handle_wm_timer" \
  --label "refactor,good first issue" \
  --body "$(cat <<'EOF'
## Motivation

\`polling.hpp:64-84\` builds the window title string inline inside \`handle_wm_timer()\`. This 20-line block:

1. Scans all timers for a running one
2. Falls back to the stopwatch
3. Falls back to the system clock
4. Appends " — Chronos"
5. Updates both the title bar and the tray tooltip

It is self-contained and has no dependency on the rest of \`handle_wm_timer\` other than the \`WndState\` reference.

## Suggested refactor

Extract into a free function:

```cpp
inline void update_title(HWND hwnd, WndState& s) { /* ... */ }
```

Called from \`handle_wm_timer\` as a single line. Reduces \`handle_wm_timer\` to its core responsibility (checking timer expiry and polling rate), and makes the title logic independently testable in the future.
EOF
)"
echo "  ✓ Issue 11 created"

echo ""
echo "Done — 11 issues created."
