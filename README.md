# SendReceiveCAN — cockpit CAN bridge

`can_bridge` connects the cockpit hardware of a flight simulator to the
simulator software. It reads buttons, knobs and switches from two CAN buses
and publishes them to the simulator's shared memory, and it drives the
cockpit indicators (compass, slip ball, ADF frequency, transponder display)
from simulator values.

This is a rewrite of the original single-file Visual Studio program. The
protocol and the shared-memory contract are unchanged (verified table by
table against the old code); the structure, the build and the I/O layer
are new.

## Highlights

- **Builds without Visual Studio**: CMake + Ninja with MinGW-w64 GCC or
  Clang on Windows, clang-cl, or a cross build from Linux. MSVC still works.
- **No SDK dependency**: talks to the CAN board driver directly through a
  minimal interface description (`src/can/advantech/advantech_abi.h`).
- **Decomposed design**: protocol decoding, cockpit state, publishing,
  transport and the simulator adapter are separate, unit-tested components.
- **No lost input**: frames are read in batches and every key press and
  encoder step stays visible to the simulator for at least one cycle.
- **Self-healing**: ports reopen automatically with back-off, bus-off is
  recovered, the switch state is re-polled and indicators are refreshed
  after every reconnect, and port health is reported through the
  simulator's fault flags.
- **Google C++ style** (C++20), enforced with `.clang-format` and
  `.clang-tidy`.

## Architecture

```
            +-------------- CanChannel thread -------------+
panel bus --+-> AdvantechCanPort -> PanelDecoder -----------+--> InputScheduler
 (can1)     |                       ack frames <-+          |         |
            +----------------------------------------------+          | every 20 ms
            +-------------- CanChannel thread -------------+          v
overhead  --+-> AdvantechCanPort -> OverheadDecoder --------+--> Simulator (LegacySimulator
 (can2)     |                       ack frames, poll <-+    |     -> shared memory)
            +----------------------------------------------+          |
                                                                      | every 100 ms
panel bus <------------------------- OutputEncoder <------------------+
```

| Directory        | Contents                                                        |
|------------------|-----------------------------------------------------------------|
| `src/can`        | `CanFrame`, `CanPort` interface, Advantech driver backend        |
| `src/protocol`   | Frame decoders, key tables, indicator encoder, protocol constants |
| `src/cockpit`    | Input model and `InputScheduler` (per-cycle publishing)          |
| `src/sim`        | `Simulator` interface, legacy shared-memory adapter              |
| `src/config`     | INI reader and validated `BridgeConfig`                          |
| `src/app`        | `CanChannel` (I/O thread), `Bridge` (wiring, main loop), `main`  |
| `tests`          | GoogleTest suite, fakes, stub simulator headers                  |

Everything except the driver backend, the Win32 shared memory and `main` is
portable C++ and is tested on Linux as well as Windows.

### Why an input scheduler

The simulator samples shared memory periodically. The old program read one
frame per bus every 20 ms, which both throttled the bus (encoders queued up
in the driver) and decided how long a state stayed visible. The new reader
drains the driver in batches, so a press and its release can arrive within
one cycle. `InputScheduler` therefore applies events per publish cycle and
defers an event to the next cycle when it would overwrite a field changed in
the current one. Every state is visible for at least one cycle, per-field
order is preserved, and unrelated inputs are never delayed.

## Building

The executable needs the simulator's headers (`so2010struct.h`,
`STRUCT_AVN_410_RTSH_4.h`, `Struct_SUT_410.h`). They belong to the simulator
project and are not part of this repository; pass their directory in
`CAN_BRIDGE_SIM_INCLUDE_DIR`. Without it, only the libraries and tests are
built.

### Windows, MinGW-w64 with GCC or Clang (MSYS2) — no Microsoft tools

```
pacman -S mingw-w64-ucrt-x86_64-gcc mingw-w64-ucrt-x86_64-cmake mingw-w64-ucrt-x86_64-ninja
cmake --preset windows-mingw -DCAN_BRIDGE_SIM_INCLUDE_DIR=C:/path/to/sim/headers
cmake --build --preset windows-mingw
```

For Clang, use the MSYS2 CLANG64 environment
(`mingw-w64-clang-x86_64-clang`, `-cmake`, `-ninja`) and the
`windows-clang` preset. MinGW builds link statically: `can_bridge.exe` needs
no extra DLLs.

### Windows, LLVM clang-cl

Uses the Microsoft C++ library and Windows SDK, so it needs the free Visual
Studio Build Tools (not the IDE), plus LLVM, CMake and Ninja
(`winget install LLVM.LLVM Kitware.CMake Ninja-build.Ninja`). In a developer
command prompt:

```
cmake --preset windows-clang-cl -DCAN_BRIDGE_SIM_INCLUDE_DIR=C:/path/to/sim/headers
cmake --build --preset windows-clang-cl
```

### Cross build on Linux

```
sudo apt install g++-mingw-w64-x86-64-posix cmake ninja-build
cmake --preset mingw-cross -DCAN_BRIDGE_SIM_INCLUDE_DIR=/path/to/sim/headers
cmake --build --preset mingw-cross
```

Add `-DCMAKE_CROSSCOMPILING_EMULATOR=wine` to run the Windows test binary
under Wine with `ctest`.

### Tests

```
cmake --preset dev && cmake --build --preset dev && ctest --preset dev
```

`dev-sanitizers` (AddressSanitizer + UBSan) and `dev-tsan`
(ThreadSanitizer) run the same suite with runtime checks. On Windows the
test build also compiles the driver backend and a copy of the executable
against the stub headers, so every build checks all of the code; that copy
refuses to run.

## Running

```
can_bridge.exe [--config <file>] [--version] [--help]
```

Settings are read from `can_bridge.ini` next to the executable (see
`config/can_bridge.ini`, which documents every option); without the file the
defaults of the old program are used. Unknown keys produce a warning, invalid
values stop the program with a message.

The bridge exits on Ctrl+C, when the console is closed, or when the simulator
sets `SHSUT::Close`. Only one instance can run at a time.

## Protocol summary

Frames are dispatched by `data[0]` (device code). Key frames carry the key
code in `data[1]`, `0` meaning "all keys of the device released". Knob frames
are 3 bytes for a rotation step (`data[1]`: 0 = clockwise, 1 = counter-
clockwise) and 4 bytes for the knob's push button (`data[3]`).

| Code      | Device                                   | Ack (id = 100 + code) |
|-----------|------------------------------------------|-----------------------|
| 20–28     | MFI knobs, ISRP-26 buttons on shared frames | –                  |
| 29, 30    | ISRP-26 BARO, HDG knobs                  | –                     |
| 42, 43    | B8-50KR keys, left / right               | `{code, data[1]}`     |
| 60        | SO-2010 keys (+ entered code)            | –                     |
| 61–63     | MFI keys: center, right, left            | `{code, data[1]}`     |
| 64–67     | MFPU keys: right main/extra, left main/extra | `{code, data[1]}` |
| 71–77     | Overhead switch rows (bus 2)             | `{code, data[1], data[2]}` |
| 81, 82    | B8-50KR selectors, right / left          | –                     |

Outputs (CAN id = code): 51 ADF-40 frequency, 60 SO-2010 display, 80 compass,
88 slip ball. Ids 121–127 with payload `111` request the state of overhead
rows 1–7.

Glossary: MFI — МФИ, MFPU — МФПУ, B8-50KR — Б8-50КР, ISRP-26 — ИСРП-26,
SO-2010 — СО-2010, ADF-40 — АРК-40, slip ball — «шарик».

## Behaviour changes compared to the old program

Fixed:

- Reads and writes no longer share one `OVERLAPPED` structure across threads
  (could return results of the wrong request or hang).
- Frames are read in batches instead of one per 20 ms per bus.
- Every key frame gets its own acknowledgement. Previously only the last
  frame of each 100 ms window was acknowledged, sometimes with bytes of a
  different frame.
- The configuration file is read from the executable's directory (the old
  relative path made Windows look in `C:\Windows`).
- Empty frames and remote requests no longer replay stale data.
- The B8-50KR selector decoder ignores contact bounce and does not count a
  phantom step on the first frame.
- Indicators are sent when their encoded value changes: the slip ball used to
  move only after a change of 1.0, half of its range.
- Indicator values are sent on start-up and after a reconnect; overhead
  switches are re-polled after a reconnect.
- Port failures are recovered instead of being fatal (start-up) or permanent
  (runtime); fault flags are cleared again when the port recovers.
- The 20 ms console print and the unsynchronised shared state between threads
  are gone.

Kept as is, but worth checking on the rig (all configurable or one-line
changes):

- The HDG knob drives the left MFI outer ring (`[cockpit] hdg_knob`).
- The compass heading is split with base 255 (`compass_byte_base`).
- B8-50KR digits 5 and 8 use swapped codes (10/11) on the left and right
  panels (`src/protocol/key_tables.cc`).
- The center MFI brightness knob reports the opposite direction.
- B8-50KR key fields receive `1` when pressed; the old program wrote the key
  code (equal for `bool` fields).
