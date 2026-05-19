# Architecture Guardrails

This document defines dependency boundaries between source layers and how to enforce them in CI.

## Layer dependency policy

Allowed dependency directions:

- `src/core` -> C++ standard library + `src/core` only.
- `src/ui` -> `src/core` only (no platform headers).
- `src/painting` -> `src/core` + `src/ui`.
- `src/input` -> `src/core` + `src/ui` + Win32 integration seams.
- `src/win32` -> shared layers (`core`, `ui`, `painting`, `input`).
- `src/linux` -> shared layers (`core`, `ui`, `painting`).

Forbidden includes:

- `src/core` and `src/ui` must not include platform headers (`windows.h`, `dwmapi.h`, `X11/...`).
- Backend cross-talk is forbidden:
  - `src/win32/*` must not include `src/linux/*`
  - `src/linux/*` must not include `src/win32/*`

## Enforcement

Run:

```bash
python3 scripts/check_layer_deps.py
```

Modes:

- **Default mode**: fails only on newly introduced violations, while allowing the documented migration baseline.
- **Strict mode** (`--strict`): fails on every violation, including baseline entries.

## Cleanup strategy

1. Use default mode in CI to prevent new debt.
2. Remove baseline violations incrementally.
3. Switch CI to strict mode when baseline reaches zero.

## Refactor guidance

- Move platform behavior out of `core`/`ui` behind explicit data contracts.
- Keep state transitions in core actions/model code.
- Keep painting/event loop concerns in backend adapters.
- Add regression tests before and after extraction.
