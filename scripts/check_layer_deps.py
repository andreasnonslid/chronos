#!/usr/bin/env python3
"""Enforce architectural layer include boundaries.

Default mode: fail only on *new* violations while allowing a documented baseline.
Strict mode: fail on any violation, including baseline entries.
"""

from __future__ import annotations

import argparse
import re
import sys
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
SRC = ROOT / "src"

INCLUDE_RE = re.compile(r'^\s*#\s*include\s*[<\"]([^\">]+)[\">]')
TARGET_EXTS = {".hpp", ".h", ".cpp", ".cc", ".cxx"}

# Keyed by first directory under src/.
FORBIDDEN_BY_LAYER: dict[str, tuple[str, ...]] = {
    "core": ("windows.h", "dwmapi.h", "X11/", "src/win32/", "src/linux/"),
    "ui": ("windows.h", "dwmapi.h", "X11/", "src/win32/", "src/linux/"),
    "win32": ("src/linux/",),
    "linux": ("src/win32/",),
}

# Existing violations tracked during migration.
# Format: (<repo-relative path>, <included header>)
BASELINE_ALLOW: set[tuple[str, str]] = {
    ("src/core/encoding.cpp", "windows.h"),
    ("src/ui/icon.hpp", "windows.h"),
    ("src/ui/paint_ctx.hpp", "windows.h"),
    ("src/ui/theme.hpp", "windows.h"),
}


def detect_layer(path: Path) -> str | None:
    parts = path.relative_to(SRC).parts
    return parts[0] if parts else None


def iter_sources(root: Path):
    for path in sorted(root.rglob("*")):
        if path.suffix in TARGET_EXTS and path.is_file():
            yield path


def main() -> int:
    parser = argparse.ArgumentParser(description="Check forbidden include edges by source layer.")
    parser.add_argument("--strict", action="store_true", help="Fail on all violations, including baseline allowlist entries.")
    parser.add_argument(
        "--fail-on-stale-baseline",
        action="store_true",
        help="Fail if baseline allowlist entries are no longer needed (helps keep baseline minimal).",
    )
    args = parser.parse_args()

    all_violations: list[str] = []
    new_violations: list[str] = []
    seen_pairs: set[tuple[str, str]] = set()

    for path in iter_sources(SRC):
        layer = detect_layer(path)
        if layer not in FORBIDDEN_BY_LAYER:
            continue

        forbidden = FORBIDDEN_BY_LAYER[layer]
        rel = path.relative_to(ROOT).as_posix()
        text = path.read_text(encoding="utf-8", errors="ignore")

        for lineno, line in enumerate(text.splitlines(), start=1):
            m = INCLUDE_RE.match(line)
            if not m:
                continue

            include = m.group(1)
            matched = next((pat for pat in forbidden if pat in include), None)
            if not matched:
                continue

            key = (rel, include)
            seen_pairs.add(key)
            msg = f"{rel}:{lineno}: forbidden include '{include}' (matched '{matched}')"
            all_violations.append(msg)

            if key not in BASELINE_ALLOW:
                new_violations.append(msg)

    stale_baseline = sorted(BASELINE_ALLOW - seen_pairs)

    if new_violations:
        print("New layer dependency violations found:")
        for v in new_violations:
            print(f"- {v}")
        return 1

    if stale_baseline and args.fail_on_stale_baseline:
        print("Stale baseline entries found (remove these from BASELINE_ALLOW):")
        for path, inc in stale_baseline:
            print(f"- {path}:{inc}")
        return 1

    if all_violations and args.strict:
        print("Layer dependency violations found:")
        for v in all_violations:
            print(f"- {v}")
        return 1

    if all_violations:
        print("Layer dependency audit passed with baseline exceptions:")
        for v in all_violations:
            print(f"- {v}")
    else:
        print("Layer dependency audit passed.")

    if stale_baseline:
        print("Stale baseline entries detected (non-fatal):")
        for path, inc in stale_baseline:
            print(f"- {path}:{inc}")

    return 0


if __name__ == "__main__":
    sys.exit(main())
