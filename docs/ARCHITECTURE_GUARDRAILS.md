# Architecture Guardrails

## Motivation
This document codifies module dependency rules so the implementation stays aligned with the architecture plan.

Refs: `docs/reviews/2026-05-19-project-review.md`

## Allowed dependency directions
- `src/core` -> may depend only on C++ stdlib and other `src/core` headers.
- `src/ui` -> may depend on `src/core`, but must not include platform headers.
- `src/painting` -> may depend on `src/core` + `src/ui`.
- `src/input` -> may depend on `src/core` + `src/ui` + `src/win32` integration seams only.
- `src/win32` -> may depend on all shared layers (`core`, `ui`, `painting`, `input`).
- `src/linux` -> may depend on all shared layers (`core`, `ui`, `painting`).

## Forbidden includes
- Any file under `src/core` including Windows headers (`windows.h`, `dwmapi.h`) or X11 headers (`X11/Xlib.h`, etc.).
- Any file under `src/ui` including platform headers.
- Cross-platform backend cross-talk:
  - `src/win32/*` must not include `src/linux/*`
  - `src/linux/*` must not include `src/win32/*`

## Refactor playbook
1. If platform-only logic appears in `core` or `ui`, move behavior behind a data contract.
2. Keep state transitions in `core` action/data code, and leave drawing/event loop adapters in backends.
3. Add tests for moved logic before and after extraction.
4. Run `scripts/check_layer_deps.py` after structural changes.
