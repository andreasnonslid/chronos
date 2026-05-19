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
    for path in root.rglob("*"):
        if path.suffix in TARGET_EXTS and path.is_file():
            yield path


def main() -> int:
    parser = argparse.ArgumentParser(description="Check forbidden include edges by source layer.")
    parser.add_argument("--strict", action="store_true", help="Fail on all violations, including baseline allowlist entries.")
    args = parser.parse_args()

    all_violations: list[str] = []
    new_violations: list[str] = []

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

            msg = f"{rel}:{lineno}: forbidden include '{include}' (matched '{matched}')"
            all_violations.append(msg)

            if (rel, include) not in BASELINE_ALLOW:
                new_violations.append(msg)

    if new_violations:
        print("New layer dependency violations found:")
        for v in new_violations:
            print(f"- {v}")
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
        return 0

    print("Layer dependency audit passed.")
    return 0


if __name__ == "__main__":
    sys.exit(main())
