# Arduino Uno R4 Minima — General Purpose

A general-purpose sandbox for sketches and code targeting the **Arduino Uno R4 Minima**.

## Directory structure

This repo is intentionally **flat**: all `.ino` sketch files and their supporting source
files (`.h`, `.cpp`, etc.) live directly in the root directory, with no `src/` or nested
subfolders. This is required by the Arduino IDE, which expects a sketch's files to sit
alongside its `.ino` file in the same folder.

> Note: the Arduino IDE also expects each sketch's folder name to match its main `.ino`
> filename. If this repo grows to hold multiple distinct sketches, each will need its own
> folder (named to match), even though that folder itself stays flat internally.

## Board

- **Board:** Arduino Uno R4 Minima
- **Core:** [Arduino UNO R4 Boards](https://github.com/arduino/ArduinoCore-renesas)

## Getting started

1. Install the Arduino IDE (2.x recommended).
2. Install the "Arduino UNO R4 Boards" core via Boards Manager.
3. Open the `.ino` file in this repo with the Arduino IDE.
4. Select **Tools > Board > Arduino Uno R4 Minima** and the correct port.
5. Upload.

## License

MIT — see [LICENSE](LICENSE).
