#!/usr/bin/env python3
"""Copy a sketch from sketches/<name>/<name>.ino to the repo root.

The Arduino IDE requires a sketch's folder name to match its main .ino
filename, and (per this repo's convention) the root .ino must be named
after the repo folder: Arduino_uno_r4_gp.ino. Sketches are developed under
sketches/<name>/<name>.ino and this script promotes one of them to root
so it can be opened directly by the Arduino IDE.

Usage:
    python copy_sketch.py [sketch_name]

If sketch_name is omitted, the active sketch below (SKETCH_NAME) is used.
"""

import shutil
import sys
from pathlib import Path

ROOT_DIR = Path(__file__).resolve().parent
SKETCHES_DIR = ROOT_DIR / "sketches"
ROOT_SKETCH_NAME = f"{ROOT_DIR.name}.ino"

# Select the sketch to copy by leaving exactly one line uncommented below.
SKETCH_NAME = "basic_serial"
# SKETCH_NAME = "nrf24_receiver"
# SKETCH_NAME = "nrf24_transmitter"


def copy_sketch(sketch_name: str) -> Path:
    src = SKETCHES_DIR / sketch_name / f"{sketch_name}.ino"
    if not src.is_file():
        raise FileNotFoundError(f"Sketch not found: {src}")

    dest = ROOT_DIR / ROOT_SKETCH_NAME
    shutil.copyfile(src, dest)
    return dest


def main() -> int:
    sketch_name = sys.argv[1] if len(sys.argv) > 1 else SKETCH_NAME
    try:
        dest = copy_sketch(sketch_name)
    except FileNotFoundError as exc:
        print(f"Error: {exc}", file=sys.stderr)
        return 1

    print(f"Copied sketches/{sketch_name}/{sketch_name}.ino -> {dest.relative_to(ROOT_DIR)}")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
