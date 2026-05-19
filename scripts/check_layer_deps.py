#!/usr/bin/env python3
from pathlib import Path
import argparse
import re
import sys

ROOT = Path(__file__).resolve().parents[1]
SRC = ROOT / "src"
INCLUDE_RE = re.compile(r'^\s*#\s*include\s*[<\"]([^\">]+)[\">]')

FORBIDDEN_BY_DIR = {
    "core": ["windows.h", "dwmapi.h", "X11/", "src/win32/", "src/linux/"],
    "ui": ["windows.h", "dwmapi.h", "X11/", "src/win32/", "src/linux/"],
    "win32": ["src/linux/"],
    "linux": ["src/win32/"],
}

BASELINE_ALLOW = {
    "src/core/encoding.cpp:5:windows.h",
    "src/ui/icon.hpp:2:windows.h",
    "src/ui/paint_ctx.hpp:2:windows.h",
    "src/ui/theme.hpp:2:windows.h",
}

TARGET_EXTS = {".hpp", ".h", ".cpp", ".cc", ".cxx"}


def layer_of(path: Path) -> str | None:
    parts = path.relative_to(SRC).parts
    return parts[0] if parts else None


def main() -> int:
    parser = argparse.ArgumentParser()
    parser.add_argument("--strict", action="store_true", help="Fail for all violations including baseline allowlist")
    args = parser.parse_args()

    violations: list[str] = []
    new_violations: list[str] = []

    for path in SRC.rglob("*"):
        if path.suffix not in TARGET_EXTS:
            continue
        layer = layer_of(path)
        if layer not in FORBIDDEN_BY_DIR:
            continue
        forbidden_patterns = FORBIDDEN_BY_DIR[layer]
        for idx, line in enumerate(path.read_text(encoding="utf-8", errors="ignore").splitlines(), start=1):
            m = INCLUDE_RE.match(line)
            if not m:
                continue
            inc = m.group(1)
            for pattern in forbidden_patterns:
                if pattern in inc:
                    rel = path.relative_to(ROOT)
                    key = f"{rel}:{idx}:{inc}"
                    msg = f"{rel}:{idx}: forbidden include '{inc}' (matched '{pattern}')"
                    violations.append(msg)
                    if key not in BASELINE_ALLOW:
                        new_violations.append(msg)
                    break

    if new_violations:
        print("New layer dependency violations found:")
        for v in new_violations:
            print(f"- {v}")
        return 1

    if violations and not args.strict:
        print("Layer dependency audit passed with baseline exceptions:")
        for v in violations:
            print(f"- {v}")
        return 0

    if violations and args.strict:
        print("Layer dependency violations found:")
        for v in violations:
            print(f"- {v}")
        return 1

    print("Layer dependency audit passed.")
    return 0


if __name__ == "__main__":
    sys.exit(main())
