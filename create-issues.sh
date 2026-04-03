#!/usr/bin/env bash
# Run: gh auth login && bash create-issues.sh
# Creates curated GitHub issues for andreasnonslid/chronos
set -euo pipefail

REPO="andreasnonslid/chronos"

echo "Creating issues for $REPO..."

# ── 1. CI: Run tests on pull requests ────────────────────────────────────────
gh issue create --repo "$REPO" \
  --title "CI: Run tests on pull requests" \
  --label "enhancement,ci" \
  --body "$(cat <<'EOF'
## Problem

`ci.yml` only triggers on `push`. Pull requests receive no CI feedback before merge.

```yaml
on:
  push:
    branches: ["**"]
    tags-ignore: ["**"]
```

## Fix

Add a `pull_request` trigger so PRs are tested automatically:

```yaml
on:
  push:
    branches: ["**"]
    tags-ignore: ["**"]
  pull_request:
```

This ensures all four jobs (Windows build, Linux release, Linux debug, sanitizer) run on PRs before merge.

**File:** `.github/workflows/ci.yml`
EOF
)"
echo "  [1/7] Created: CI pull_request trigger"

# ── 2. Test: A_THEME action coverage ────────────────────────────────────────
gh issue create --repo "$REPO" \
  --title "Add test coverage for A_THEME action dispatch" \
  --label "enhancement,testing" \
  --body "$(cat <<'EOF'
## Gap

`A_THEME` in `actions.hpp:139-143` cycles `ThemeMode` (Auto → Dark → Light → Auto) and sets `apply_theme = true` on the result. This is the only action in `dispatch_action()` without test coverage in `test_actions.cpp`.

## Suggested test

```cpp
TEST_CASE("A_THEME cycles theme mode and signals apply_theme", "[actions]") {
    App app;
    REQUIRE(app.theme_mode == ThemeMode::Auto);

    auto r1 = dispatch_action(app, A_THEME, t0(), {});
    REQUIRE(app.theme_mode == ThemeMode::Dark);
    REQUIRE(r1.apply_theme);
    REQUIRE(r1.save_config);

    auto r2 = dispatch_action(app, A_THEME, t0(), {});
    REQUIRE(app.theme_mode == ThemeMode::Light);

    auto r3 = dispatch_action(app, A_THEME, t0(), {});
    REQUIRE(app.theme_mode == ThemeMode::Auto);
}
```

**File:** `tests/test_actions.cpp`
EOF
)"
echo "  [2/7] Created: A_THEME test coverage"

# ── 3. Validate changelog extraction in release workflow ─────────────────────
gh issue create --repo "$REPO" \
  --title "Validate changelog extraction in release workflow" \
  --label "enhancement,ci" \
  --body "$(cat <<'EOF'
## Problem

In `release.yml:35-38`, the `awk` changelog extraction has two fallback levels (versioned section → Unreleased), but if both miss, `BODY` is empty and the release is created with no description. This fails silently.

## Fix

Add a guard after the extraction:

```bash
if [ -z "$BODY" ]; then
  echo "::warning::No changelog section found for $VERSION or [Unreleased]"
  BODY="No changelog entry for this release."
fi
```

This ensures releases always have a body and the CI logs surface missing changelog entries.

**File:** `.github/workflows/release.yml`
EOF
)"
echo "  [3/7] Created: Release changelog validation"

# ── 4. Feature: Keyboard shortcut for Pomodoro ──────────────────────────────
gh issue create --repo "$REPO" \
  --title "Keyboard shortcut to toggle Pomodoro mode" \
  --label "feature" \
  --body "$(cat <<'EOF'
## Motivation

Every major action has a keyboard shortcut (Space, L, E, C, R, T, D, H, 1-3, +/-) except toggling Pomodoro mode, which requires right-clicking an idle timer and selecting from a context menu.

## Proposal

Add **P** key to toggle Pomodoro on the first idle timer (or focused timer, matching how Space/R target the first timer when stopwatch is hidden).

Behavior:
- If the first timer is idle and not in Pomodoro mode → enable Pomodoro (same as context menu selection)
- If the first timer is in Pomodoro mode → disable Pomodoro, clear label, keep current duration

Add the shortcut to the help overlay in `painting.hpp`.

**Files:** `input_keyboard.hpp`, `painting.hpp`
EOF
)"
echo "  [4/7] Created: Pomodoro keyboard shortcut"

# ── 5. Enhancement: Config Pomodoro defaults ─────────────────────────────────
gh issue create --repo "$REPO" \
  --title "Use Pomodoro constants from pomodoro.hpp in Config defaults" \
  --label "enhancement" \
  --body "$(cat <<'EOF'
## Problem

The same Pomodoro default durations appear in three independent locations:

1. **pomodoro.hpp:7-9** — named constants: `POMODORO_WORK_SECS = 25 * 60`, etc.
2. **config.hpp:41-43** — struct defaults: `pomodoro_work_secs = 25 * 60`, etc. (hardcoded, not referencing the constants)
3. **config.hpp:51** — write guard: `if (c.pomodoro_work_secs != 25 * 60 || ...)` (hardcoded again)

If any one changes, the others become stale. The config write guard in particular would silently start always writing Pomodoro fields even when they match the actual default.

## Fix

In `config.hpp`, include `pomodoro.hpp` and use the named constants:

```cpp
int pomodoro_work_secs  = POMODORO_WORK_SECS;
int pomodoro_short_secs = POMODORO_SHORT_BREAK_SECS;
int pomodoro_long_secs  = POMODORO_LONG_BREAK_SECS;
```

For the write guard, compare against a default-constructed `Config`:

```cpp
Config defaults;
if (c.pomodoro_work_secs  != defaults.pomodoro_work_secs ||
    c.pomodoro_short_secs != defaults.pomodoro_short_secs ||
    c.pomodoro_long_secs  != defaults.pomodoro_long_secs)
```

**Files:** `src/config.hpp`
EOF
)"
echo "  [5/7] Created: Config Pomodoro constants"

# ── 6. Refactor: Encapsulate label-edit global state ─────────────────────────
gh issue create --repo "$REPO" \
  --title "Encapsulate label-edit global state into WndState" \
  --label "refactor" \
  --body "$(cat <<'EOF'
## Problem

`input_label_edit.hpp:8-10` uses two anonymous-namespace globals:

```cpp
namespace {
WNDPROC g_orig_edit_proc = nullptr;
bool g_edit_cancelled = false;
}
```

Issue #8 ("Eliminate global mutable state") addressed globals elsewhere but missed these. They work because the project is single-TU, but they bypass the explicit state management used everywhere else (WndState/App).

## Suggested approach

Move both fields into `WndState`:

```cpp
struct WndState {
    // ...
    WNDPROC orig_edit_proc = nullptr;
    bool edit_cancelled = false;
};
```

The subclass proc (`EditSubclassProc`) can retrieve `WndState*` from the parent window's `GWLP_USERDATA`, which is already set in `WM_CREATE`:

```cpp
LRESULT CALLBACK EditSubclassProc(HWND hwnd, UINT msg, WPARAM wp, LPARAM lp) {
    auto* s = (WndState*)GetWindowLongPtr(GetParent(hwnd), GWLP_USERDATA);
    // use s->orig_edit_proc and s->edit_cancelled
}
```

This eliminates the last remaining global mutable state in the codebase.

**File:** `src/input_label_edit.hpp`, `src/wndstate.hpp`
EOF
)"
echo "  [6/7] Created: Label-edit global state refactor"

# ── 7. CI: Set artifact retention ────────────────────────────────────────────
gh issue create --repo "$REPO" \
  --title "Set retention period for CI log artifacts" \
  --label "enhancement,ci" \
  --body "$(cat <<'EOF'
## Problem

The four log artifact uploads in `build.yml` (windows-logs, linux-release-logs, linux-debug-logs, sanitizer-logs) use GitHub's default 90-day retention. These are debug-only logs that are only useful for investigating recent failures.

## Fix

Add `retention-days: 7` to all log artifact uploads:

```yaml
- name: Upload logs
  if: always()
  uses: actions/upload-artifact@v4
  with:
    name: windows-logs
    retention-days: 7
    path: |
      configure-output.txt
      build-output.txt
      test-output.txt
    if-no-files-found: ignore
```

Keep the `chronos` binary artifact at default retention since it's used by the release workflow.

**File:** `.github/workflows/build.yml` (4 locations)
EOF
)"
echo "  [7/7] Created: Artifact retention"

echo ""
echo "Done! All 7 issues created."
