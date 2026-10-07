# CLAUDE.md

This file provides guidance to Claude Code (claude.ai/code) when working with code in this repository.

## Project

Sketches for the **Arduino UNO R4 Minima** (Renesas RA4M1). The one that matters most is
`sketches/remote_hub`: the **Remote Hub**, a user IO device (2 buttons, 2 LEDs, a
potentiometer) that talks to the ControlHubAA26 control hub (NUCLEO-F439ZI) over an
nRF24L01 radio using SerLink. The control hub firmware lives at
`C:\Users\rwabc\Software\Projects\ControlHubAA26\Embedded\V1\ControlHubAA26_V1\`, and its
CLAUDE.md is the reference for SerLink and for the hub's sockets. **Its
`sockets_summary.txt` is the protocol contract**: the remote hub's sockets (BTN01, POT01,
HBT01 out; LED01 in) and their frame data are defined there, and a change to any of them
has to be made at both ends.

The remote hub only reports and displays — the hub decides everything (modes, what a
button press means, the speed). Button 1 is start/stop, button 2 toggles the direction,
the pot sets the speed of runs started from the remote, LED A shows the hub's mode and
LED B the selected direction. A heartbeat (HBT01, every 500 ms) lets the hub stop a run
the remote started if the radio link drops; the hub's LED01 refresh (every second) lets
the remote show the link as lost.

`README.md` has the nRF24L01 wiring. Each sketch's `.ino` header comment documents its
sockets, frame examples and extra wiring (e.g. the LEDs are off-board, since D13 is the radio's SCK).

## Layout, and the replication rule

```
lib/                  master copy of every shared module (flat: .c .cpp .h .hpp)
sketches/<name>/      one folder per sketch, <name>.ino + COPIES of the lib/ files
  synclib.py          copies every source file from lib/ into this sketch folder
Arduino_uno_r4_gp.ino the root sketch, a copy of one sketch's .ino (copy_sketch.py)
```

The Arduino IDE compiles only the files in the sketch's own folder, so every sketch that
uses the shared modules holds **its own copy** of them. Those sketches are `remote_hub`,
`serlink_nrf24_brg` and `nrf24_receiver`. `basic_serial` and `nrf24_transmitter` are
single `.ino` files.

**Rule: a change to a shared source file must end up in `lib/` and in every sketch
folder that has a copy.** Make the edit in `lib/`, then run `synclib.py` in each of the
three sketch folders:

```bash
cd sketches/remote_hub && python synclib.py
cd sketches/serlink_nrf24_brg && python synclib.py
cd sketches/nrf24_receiver && python synclib.py
```

Notes on that:

- `synclib.py` copies **all** of `lib/` and deletes nothing. A file added to `lib/`
  therefore appears in every sketch and is compiled into all of them, so it must build
  for the R4. A file removed or renamed in `lib/` has to be deleted from each sketch
  by hand.
- If an edit was made in a sketch folder, copy it back to `lib/` first. Otherwise the
  next `synclib.py` silently overwrites it.
- A sketch's `.ino` is not shared and is never synced.
- After syncing, compile all three sketches (below), not only the one you changed.

`copy_sketch.py [name]` copies `sketches/<name>/<name>.ino` over the root
`Arduino_uno_r4_gp.ino` so the root folder opens in the IDE. It copies only the `.ino`.

## Build

arduino-cli is installed at `C:\Program Files\Arduino CLI\arduino-cli.exe`, with the
`arduino:renesas_uno` core and the **RF24** library (TMRh20). From the repo root, in
Git Bash:

```bash
"/c/Program Files/Arduino CLI/arduino-cli" compile --fqbn arduino:renesas_uno:minima sketches/remote_hub
```

A successful build prints the program storage / dynamic memory summary. There are no
tests and no lint step. Uploading is done from the IDE, or with
`arduino-cli upload -p COMx --fqbn arduino:renesas_uno:minima sketches/<name>`.

## Architecture

### No RTOS: polled state machines

Everything runs from `loop()`. Each component is a `StateMachine` (or has a `run()`)
that must be called on **every** loop iteration and must never block. Nothing works
unless its `run()` is in `loop()`: `Reader`, `Writer`, `Socket`, `Radio`,
`SerLinkRadioAdapter`, `Led`, `Button`, `Adc`, and so on. Timing comes from `timer0`
(`swTimer.h` software timers), not `delay()`.

### SerLink (Arduino port)

The same frame format as the control hub (`PROTO` + type + roll code + length + data;
`'T'` needs an ack, `'U'` fire-and-forget, `'A'` ack, `'B'` relay ack, `'S'` system
request), but implemented differently from the hub's FreeRTOS version:

- **`Reader` / `Writer`** each take a `LinkInterface` adapter: `SerLinkUartAdapter`
  (uart, `uart.h`) or `SerLinkRadioAdapter` (wraps `Radio`). Instance ids are in
  `Reader_config.hpp` / `Writer_config.hpp`: 1 for the uart, 2 for the radio.
- **`Socket`** is constructed directly, as a file-scope object bound to one
  writer/reader pair with its own rx/tx `Frame` buffers. There is no Transport and no
  acquire. A `nullptr` rx frame makes it send-only, a `nullptr` tx frame receive-only.
  The application **polls** it: `getRxData()` returns true when a frame has
  arrived, and `sendData(data, len, ack)` sends. An optional *instant handler* runs
  before the ack goes out, and its output is returned inside the ack (reads/queries),
  as on the hub. A reader registers at most **4** instant handlers, one fewer than
  `READER_CONFIG__MAX_NUM_INSTANT_HANDLERS` suggests.
- `sendData()` only fills the socket's single tx frame; `Socket::run()` hands it to the
  writer when the writer is idle. A second `sendData()` before that **overwrites** the
  first. Sockets on one writer take turns, and a `'T'` frame keeps the writer busy until
  its ack, or the 1000 ms timeout (`Writer.cpp`, no resend).
- `Writer::getStatus()` clears any non-busy status, and every `Socket::run()` calls it, so
  an ack timeout is gone before the application can see it in the usual loop order. To
  detect a lost ack, use the socket's `getAndClearSendStatus()` straight after
  `writer.run()`, or time the ack yourself.
- Call `reader.clearRxFlag()` every loop after the sockets have been polled, so a frame
  that no socket claims (unknown protocol) is dropped and doesn't block the reader.
- `Frame::toString()` writes one byte past the `'\n'`, so frame buffers are
  `UART_BUFF_LEN + 1` (`FRAME_BUFF_LEN` in the sketches).
- `SerlinkRelay` exists here too, but no sketch uses it at the moment.

### Radio

`Radio` (see its header) is the polled port of the control hub's
`Components/Radio`. It splits a serialised frame across 32-byte nRF24L01 packets with a
START flag in byte 0 and reassembles up to `'\n'`. All SPI happens in `run()`. The
channel, data rate, payload size and address (`"00001"`) **must match the control hub
exactly**: a mismatch gives silence, not corruption. Both ends use one address, so it is
a two-node, half-duplex link, and two ends transmitting at once both fail.

### Hardware modules (HardMod)

`Led`, `Button`, `Pot` (+ `Adc` / `HwModule`) use a small event model
(`HardMod_Event`, `HardMod_EventQueue`, `idChar`). Each device has a one-character
id, and its events serialise to short strings that are used directly as SerLink socket
data. For example, `"A1"` means LED `'A'`, action `LEDEVENT__ON` (`Led.hpp`), and
`"AF0002020"` flashes it for ever, 2 ticks on, 2 off (250 ms ticks). Button events are
`<id>P` / `<id>L` / `<id>R<ddd>` (`Button.hpp`; `S` is defined but never generated).
A pot event is the id plus 3 digits of percent, e.g. `"P050"` — the action character is
not serialised (`pot.hpp`). `HwModule.cpp` is the R4 version (it uses `analogRead`);
the original targeted the ATmega328's registers.

- `Button`'s constructor sets `pinMode(INPUT)` with no pull-up; for an active-low
  button, call `pinMode(pin, INPUT_PULLUP)` in `setup()`. It debounces over ~100 ms and
  holds one event at a time (`getEvent()` reads and clears it).
- `Pot` samples every 250 ms and reports any change of 1% or more, so add hysteresis in
  the sketch (remote_hub sends only on a 2% change). Call `pot.getEvent()` in the same
  loop pass as `pot.run()`, which clears the last result.
- `LedUtils::setLedEvent()` applies a decoded `LedEvent`, flashes included.
  `LedEvent::copy()` does not copy `finalFlashOff`.

### UART

`uart.h` selects the port with `UART_PORT`: USB CDC (the programming cable, the
default) or SCI2 on D0/D1 at `UART_BAUD_RATE`. `UART_BUFF_LEN` (22) limits a uart frame,
so a uart frame carries less data than a radio one.

## Conventions

- No dynamic allocation: every buffer, frame and socket is a file-scope object in the
  `.ino`.
- Shared code lives in `lib/` (see the replication rule); sketch-specific code stays in
  the `.ino`.
- Header comments carry the design rationale and the threading/ownership contract.
  Keep that style when adding modules.
- `env_config.h` selects the target (`ENV_CONFIG__SYSTEM_ARDUINO_UNO_R4`), and
  `env.hpp` holds the debug-level labels used by `DebugUser`.
