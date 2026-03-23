# Refactor Review — March 2026

This review focuses on long-term improvements for simplicity, maintainability, and testability.

## Existing open issues reviewed

The repository already has open issues for:

- `#8` — eliminating global mutable state in favor of `HWND` user data.
- `#10` — adding DPI awareness for high-DPI displays.
- `#11` — adding unit tests for stopwatch, timer, and config modules.

The recommendations below are intended to **add scope to those issues where useful** and then extend the backlog with **additional issues that are not covered yet**.

## Recommended additions to existing issues

### Additions to issue `#8` — eliminate global mutable state

The current issue is directionally correct, but it would be more valuable if it explicitly split state into a few buckets instead of moving every global into one larger `HWND` payload.

**Suggested additions**

- Separate state into:
  - runtime app state (`App`, timers, stopwatch data)
  - UI resources (fonts, cached layout data, button map)
  - services/infrastructure (logging, config path, lap export dependency)
- Prefer an owning `WindowState`/`AppContext` struct stored in `GWLP_USERDATA` that holds these buckets explicitly.
- Keep helper functions accepting `AppContext&` or narrower references instead of reaching into hidden globals.
- Use this issue as the prerequisite seam for controller-level tests and future multi-window support.

**Definition-of-done additions**

- No mutable process-wide globals remain for app/window state.
- `WndProc` can recover all state from `hwnd`.
- Pure helpers no longer depend on hidden global data.

---

### Additions to issue `#10` — add DPI awareness for high-DPI displays

The current issue correctly identifies scaling and `WM_DPICHANGED`, but it can be strengthened by clarifying what must become DPI-derived instead of fixed.

**Suggested additions**

- Introduce a DPI helper that scales:
  - layout constants
  - font point sizes
  - button corner radii / padding
  - minimum window dimensions
- Recreate fonts when DPI changes rather than scaling only window dimensions.
- Decide whether persisted window width should be stored in logical units instead of raw physical pixels.
- Ensure hit-testing uses the same DPI-scaled rectangles as painting.

**Definition-of-done additions**

- `Layout` values are not raw compile-time pixel constants in the paint path.
- Fonts and section geometry update correctly after `WM_DPICHANGED`.
- Moving the window between monitors preserves readable text and correctly sized controls.

---

### Additions to issue `#11` — add unit tests for stopwatch, timer, and config modules

The current issue is a good start, but it should be more explicit about edge cases and about the next testing layer after module tests exist.

**Suggested additions**

- Add concrete test cases for current boundaries:
  - timer duration clamping (`10` to `86400` seconds)
  - config handling of missing/invalid keys
  - stopwatch resume behavior after stop/start cycles
  - timer pause/resume preserving elapsed time
- Add round-trip tests that verify only the intended config fields persist.
- Add a follow-up section or checklist for controller-level tests once action handling is extracted from `main.cpp`.
- Treat this issue as the foundation for CI assertions, not the full test strategy.

**Definition-of-done additions**

- CI runs the new unit tests.
- Config edge cases and time-state edge cases are covered.
- A short backlog note points from module tests to future controller/integration tests.

## New GitHub issue proposals

### 1) Refactor: Split `main.cpp` into app-state, rendering, and Win32 shell modules

**Why this matters**

`src/main.cpp` currently owns domain-adjacent state, config persistence, layout helpers, painting, action handling, `WndProc`, and startup logic in one file. That keeps the app easy to ship in the short term, but it makes future changes riskier because nearly every feature edit crosses unrelated concerns.

**Evidence in the code**

- The same file defines the global `App` state and related globals.
- The same file also contains config persistence helpers.
- Painting and interaction logic live beside `WndProc` and process startup.

**Suggested scope**

- Create separate translation units or modules for:
  - `app_state` / controller logic
  - rendering / drawing helpers
  - window procedure and Win32 shell bootstrap
- Keep Win32-only code at the edges and move pure state transitions into testable helpers.
- Make `main.cpp` only responsible for process startup and top-level wiring.

**Definition of done**

- `main.cpp` becomes a thin entry point.
- Rendering code no longer needs direct access to unrelated startup/config code.
- Action handling can be exercised without entering the full message loop.

---

### 2) Refactor: Replace arithmetic action IDs with a typed command model

**Why this matters**

User actions are encoded through integer offsets (`A_TMR_BASE`, `TMR_STRIDE`, `tmr_act`, and offset math in `handle`). That approach is compact, but it hides intent and makes future changes fragile because adding or reordering actions depends on keeping several implicit numeric relationships aligned.

**Evidence in the code**

- Timer actions are decoded by integer division and modulo.
- Branching behavior depends on a shared stride contract.
- `wants_blink` and `handle` both know details about the numeric scheme.

**Suggested scope**

- Introduce a typed action representation, for example:
  - `enum class ActionKind`
  - `struct Action { ActionKind kind; std::optional<int> timer_index; }`
- Move command dispatch into small section-specific handlers:
  - top-bar actions
  - stopwatch actions
  - timer actions
- Keep the Win32 button hit-testing layer responsible only for mapping button rectangles to typed actions.

**Definition of done**

- No timer behavior depends on `% TMR_STRIDE` or `/ TMR_STRIDE` arithmetic.
- Adding a timer action requires changing one explicit action table rather than multiple numeric constants.
- Blink behavior can switch on semantic action kinds instead of ID math.

---

### 3) Refactor: Centralize layout and sizing rules in a single layout engine

**Why this matters**

Window height and section layout are recomputed in multiple places using very similar logic. That duplication increases maintenance cost because UI changes require updating several copies of the same sizing rules and keeping them consistent.

**Evidence in the code**

- Height is recalculated in `resize_window`.
- Similar height logic appears again in `WM_SIZING`.
- Related minimum-size logic appears again in `WM_GETMINMAXINFO`.
- Button coordinates are manually recomputed inline during painting.

**Suggested scope**

- Introduce a `LayoutModel` or equivalent helper that computes:
  - total client height
  - minimum track size
  - section rectangles
  - button rectangles
- Make both painting and resize-related message handlers call the same layout functions.
- Consider returning a tree/struct of rectangles from one place so hit testing and drawing use the same geometry source.

**Definition of done**

- There is one authoritative implementation for section sizing.
- `WM_SIZING`, `WM_GETMINMAXINFO`, and painting reuse the same layout computations.
- Future UI changes only require updating one layout module.

---

### 4) Refactor: Extract stopwatch lap export into a dedicated persistence service

**Why this matters**

Stopwatch lap export currently mixes business behavior, timestamp-based filename creation, file I/O, formatting, and user-facing error handling inside the `handle` switch. That makes lap behavior harder to test and couples stopwatch changes to Win32 UI concerns.

**Evidence in the code**

- `A_SW_START` creates the lap filename inside the UI action handler.
- `A_SW_LAP` appends to the export file, formats lap lines, and shows a message box on failure.
- `A_SW_GET` opens the exported file directly via `ShellExecuteW`.

**Suggested scope**

- Introduce a small lap export service responsible for:
  - generating output filenames
  - formatting lap rows
  - writing/appending to disk
  - reporting failures via a structured result
- Keep the UI layer responsible only for deciding how to surface failures.
- Make formatting helpers independent from Win32 so they can be covered by tests.

**Definition of done**

- Stopwatch action handlers no longer perform raw file I/O directly.
- Lap export behavior is testable without a window handle.
- Error reporting is explicit instead of being embedded in the happy-path action code.

---

### 5) Refactor: Introduce a dedicated config repository with validation and schema boundaries

**Why this matters**

Configuration concerns are split between `main.cpp` and `config.ixx`. The persistence format is simple, which is good, but the current flow still couples file discovery, window geometry capture, validation rules, and serialization entry points across modules. That makes future settings additions more error-prone than they need to be.

**Evidence in the code**

- `config.ixx` parses key/value data and clamps some fields.
- `main.cpp` decides where config lives, how window geometry is captured, and how `App` maps to `Config`.
- Configuration loading and saving are directly intertwined with window state.

**Suggested scope**

- Introduce a thin config repository/service with explicit responsibilities:
  - locate config file
  - translate between runtime state and persistent config
  - validate/clamp loaded values in one place
  - optionally prepare for format versioning or migration
- Move `App` ↔ `Config` mapping into dedicated conversion helpers.
- Add room for result/error reporting so malformed config can be surfaced cleanly.

**Definition of done**

- Loading/saving app settings no longer depends on ad hoc field copying inside `main.cpp`.
- Validation rules live in one boundary.
- New settings can be added without editing several unrelated functions.

---

### 6) Refactor: Introduce section view-models so paint code becomes data-driven

**Why this matters**

`paint_all` currently computes UI state, layout decisions, text formatting, colors, and button geometry inline while drawing. That is workable for a small app, but it scales poorly because rendering and presentation rules are hard to review in isolation.

**Evidence in the code**

- `paint_all` derives current stopwatch display text, timer warning colors, progress widths, button enablement, and section-specific layout inline.
- Timer rendering combines domain queries (`remaining`, `expired`, `touched`) with drawing primitives in a single loop.

**Suggested scope**

- Introduce lightweight per-section presentation structs, for example:
  - `TopBarViewModel`
  - `StopwatchViewModel`
  - `TimerViewModel`
- Build these models from the current app state before painting.
- Make the painter consume precomputed presentation data rather than consulting domain state directly.

**Definition of done**

- Painting logic mostly draws from prepared view-model data.
- Presentation rules such as warning colors and display strings are testable without GDI.
- Section-specific UI changes are isolated from low-level drawing code.

---

### 7) Refactor: Wrap long-lived Win32 resources in RAII types

**Why this matters**

The file already uses a small RAII helper for generic GDI objects, which is a good start. However, longer-lived resources such as fonts, paint-time memory DC/bitmap ownership, and timer lifetime are still managed manually. Extending RAII consistently would simplify cleanup paths and reduce the chance of future leaks when the app grows.

**Evidence in the code**

- Fonts are created in `WM_CREATE` and manually deleted in `WM_DESTROY`.
- The double-buffered paint path manually manages compatible DC/bitmap selection and cleanup.
- The window timer is started in multiple places and explicitly killed on destroy.

**Suggested scope**

- Add narrow RAII wrappers for:
  - owned fonts
  - paint backbuffer resources
  - timer registration lifetime if practical
- Prefer wrappers that make the correct cleanup automatic and local.
- Keep wrappers minimal and Win32-specific so they do not add architectural overhead.

**Definition of done**

- Resource cleanup no longer depends on remembering matching delete/kill calls in distant code paths.
- Temporary paint resources are exception-safe and early-return-safe.
- Resource ownership is explicit from type names.

---

### 8) Refactor: Replace hard-coded local build paths with CMake presets and documented workflows

**Why this matters**

`Justfile` currently hard-codes local MSYS2 executable paths. That is convenient for one machine, but it increases setup friction and makes contributor workflows harder to standardize. Build workflow simplification is a maintainability win because it reduces hidden local assumptions.

**Evidence in the code**

- `Justfile` points directly at `C:/msys64/.../cmake.exe` and `clang++.exe`.
- CI already describes the intended toolchain setup separately in GitHub Actions.

**Suggested scope**

- Add `CMakePresets.json` for local development and CI parity.
- Make `Justfile` call presets or plain `cmake` from `PATH` instead of hard-coded absolute paths.
- Update README to describe one canonical build path for contributors.

**Definition of done**

- A new contributor can build without editing repository files for local absolute paths.
- Local and CI builds use the same named presets or near-identical configuration.
- Build instructions become shorter and easier to maintain.

---

### 9) Refactor: Add controller-level tests for action handling after seams are introduced

**Why this matters**

Issue `#11` covers pure modules, but the highest-risk behavior today lives in `main.cpp` action handling: toggles, timer manipulation, config save triggers, and stopwatch export flow. Once state and actions are decoupled from Win32, this behavior deserves its own test layer.

**Suggested scope**

- Introduce a controller/API that accepts typed actions and returns deterministic state changes / side effects.
- Add tests for:
  - top-bar toggles resizing/saving decisions
  - timer add/remove rules and bounds
  - timer duration adjustments
  - stopwatch reset/export state transitions
  - which actions should trigger persistence
- Keep Win32 message translation outside these tests.

**Definition of done**

- Core action behavior can be tested without `HWND`, `MessageBoxW`, or `ShellExecuteW`.
- Regressions in `handle`-style logic can be caught by automated tests.
- This layer clearly complements, rather than replaces, pure module tests.

---

### 10) Refactor: Extract diagnostics and error reporting policy from event handlers

**Why this matters**

Debug logging, file-open failure handling, and user notifications are currently embedded directly in startup/config/export flows. That makes it harder to test failure modes and spreads error policy across unrelated branches.

**Suggested scope**

- Introduce a tiny diagnostics/error-reporting abstraction for:
  - debug log output
  - config load/save failures
  - lap export failures
- Return structured results from persistence helpers so the UI layer decides whether to log silently, show a dialog, or ignore.
- Keep the abstraction intentionally small; this is not a full logging framework.

**Definition of done**

- Domain/persistence helpers do not call UI notifications directly.
- Failure behavior is visible in function signatures instead of being hidden in side effects.
- Error paths can be exercised in tests without requiring Win32 UI.

---

### 11) Refactor: Extract time-formatting and display policy into pure utilities

**Why this matters**

Formatting rules for stopwatch, timer, and clock display are currently implemented near the UI layer. Pulling them into pure utilities would simplify the painter, reduce duplication risk, and make display decisions easier to test when formatting rules inevitably evolve.

**Suggested scope**

- Move display helpers into a small formatting module for:
  - stopwatch short/long formatting policy
  - timer `MM:SS` formatting
  - clock display formatting
  - lap row formatting shared with export
- Keep formatting functions free of Win32 types.
- Add focused tests that lock down rendering-facing string behavior.

**Definition of done**

- Paint code asks for formatted strings instead of building them inline.
- Export formatting and on-screen formatting share common rules where appropriate.
- Formatting regressions can be caught without UI tests.

## Prioritization suggestion

Recommended implementation order:

1. Expand issue `#8` with explicit state buckets and ownership goals.
2. Split `main.cpp` by concern.
3. Centralize layout rules.
4. Replace arithmetic action IDs with a typed command model.
5. Extract stopwatch lap export.
6. Introduce a dedicated config repository.
7. Add section view-models.
8. Expand issue `#11` with edge-case and controller-test follow-up scope.
9. Add controller-level tests for action handling.
10. Expand issue `#10` with font recreation, logical sizing, and DPI-scaled hit testing.
11. Expand RAII coverage for Win32 resources.
12. Simplify local build workflows with presets.
13. Extract diagnostics/error reporting and formatting utilities as cleanup follow-ups.

That ordering creates cleaner seams first, which should make testing and future UI work much easier.
