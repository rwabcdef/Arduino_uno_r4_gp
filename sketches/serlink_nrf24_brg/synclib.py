#!/usr/bin/env python3
"""Copy all C/C++ source files from <repo>/lib/ into this sketch folder.

The Arduino IDE only compiles source files that sit in the sketch folder, so
shared modules kept in lib/ (a flat directory) are copied here before building.

Usage:
    python synclib.py
"""

import shutil
import sys
from pathlib import Path

SKETCH_DIR = Path(__file__).resolve().parent
ROOT_DIR = SKETCH_DIR.parent.parent
LIB_DIR = ROOT_DIR / "lib"

SOURCE_EXTENSIONS = {".c", ".cpp", ".cc", ".h", ".hpp"}


def sync_lib() -> list[Path]:
    if not LIB_DIR.is_dir():
        raise FileNotFoundError(f"lib dir not found: {LIB_DIR}")

    copied = []
    for src in sorted(LIB_DIR.iterdir()):
        if src.is_file() and src.suffix.lower() in SOURCE_EXTENSIONS:
            dest = SKETCH_DIR / src.name
            shutil.copyfile(src, dest)
            copied.append(dest)
    return copied


def main() -> int:
    try:
        copied = sync_lib()
    except FileNotFoundError as exc:
        print(f"Error: {exc}", file=sys.stderr)
        return 1

    for dest in copied:
        print(f"Copied lib/{dest.name} -> {dest.relative_to(ROOT_DIR)}")
    print(f"{len(copied)} file(s) copied")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
