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

## Sketches

- `basic_serial` — prints an incrementing "count X" over serial every 2 seconds.
- `nrf24_receiver` — listens for packets on an nRF24L01 module and prints each
  payload received.
- `nrf24_transmitter` — sends a "count X" packet over an nRF24L01 module every
  3 seconds.

### nRF24L01 wiring

| nRF24L01 pin | Arduino Uno R4 Minima pin |
| ------------ | ------------------------- |
| VCC          | 3V3 (**not** 5V)          |
| GND          | GND                       |
| CE           | D9                        |
| CSN          | D10                       |
| SCK          | D13                       |
| MOSI         | D11                       |
| MISO         | D12                       |
| IRQ          | not connected             |

The module runs on 3.3V logic/power only — connecting VCC to 5V can damage it.
A decoupling capacitor (e.g. 10uF) across VCC/GND right at the module is
recommended, since the module is sensitive to power supply noise.

Requires the **RF24** library by TMRh20, installable via the Arduino IDE
Library Manager.

## Getting started

1. Install the Arduino IDE (2.x recommended).
2. Install the "Arduino UNO R4 Boards" core via Boards Manager.
3. Open the `.ino` file in this repo with the Arduino IDE.
4. Select **Tools > Board > Arduino Uno R4 Minima** and the correct port.
5. Upload.

## License

MIT — see [LICENSE](LICENSE).
