# Roadmap

Work through these in order. Each milestone has a pass check, so if something breaks you know which layer it's in. See [FILE_LAYOUT.md](FILE_LAYOUT.md) for where everything lives.

**Setup:** 3 anchors + 1 tag. Each is an ESP32 with an Ai-Thinker BU03 UWB module ([LCSC C42419011](https://www.lcsc.com/product-detail/C42419011.html)): a DW3000-series chip on direct SPI (no onboard MCU, channels 5 and 9).

## Status (2026-11-11)
| | |
|---|---|
| **Done** | Trilateration solver + unit tests, live plot + `--demo` mode, hardware in the lab, BU03s soldered, DW3000 firmware port (builds; not yet run on hardware) |
| **Now** | Confirm the pin wiring, then the smoke test (milestone 1) |
| **In parallel** | Tag logic (milestones 4–5), switch the repo to 3 anchors |
| **Next on hardware** | Two-board ranging (milestone 2) |

**Unplug and replug the board after uploading.** UWB chips sometimes don't reset cleanly otherwise.

## 0. Hardware and DW3000 port
Hardware (in the lab: 7 BU03, 6 ESP32, 4 × 3.7 V Li batteries):
- [x] Choose and order the ESP32 boards and BU03 modules.
- [x] Solder the BU03s to the ESP32s.
- [ ] Confirm the wiring matches `lib/uwb_config/uwb_config.h` (recommended: CLK 18, MISO 19, MOSI 23, CSN 4, RSTN 27, IRQ 34, WAKEUP to GND). If a board differs, change the pins there.

Power: the BU03 takes 2.5–3.6 V. Feed it from the ESP32 board's **3V3 pin**. Never connect a 3.7 V Li cell (4.2 V when full) directly to the BU03 or to the ESP32's 3V3 pin. RSTN must never be driven high.

Firmware:
- [x] Replace the DW1000 library with a DW3000 driver and port both `main.cpp` files. The driver (Qorvo API, Makerfabs Arduino port) is fetched by PlatformIO via `lib_deps`; the ranging code is in `lib/uwb_ranging`. The tag polls each anchor by address in turn (single-sided two-way ranging) and only that anchor replies.

**Pass:** both projects build with `pio run`. ✅

## 1. Smoke test (next)
- [ ] Plug one board into USB. `cd uwb_anchor` then `pio run -e anchor1 -t upload` then `pio device monitor`. The firmware prints the device ID at start-up.
- [ ] Repeat on all 4 boards.

**Pass:** every board prints the DW3000 device ID (`DECA0302` for a DW3110, `DECA0312` for a DW3120). If you see `00000000` or `FFFFFFFF`, the SPI link is wrong: check the pins in `uwb_config.h`, the 3V3/GND wiring and the solder joints.

## 2. Two-board ranging
- [ ] One anchor and the tag powered. The tag prints `anchor 81: x.xx m`.
- [ ] Record readings at 1, 2 and 5 m (tape measure, line of sight).

**Pass:** readings are stable. They will read long by a roughly constant amount until milestone 3.

## 3. Antenna delay calibration
- [ ] Hold the tag at a known distance (e.g. 2.00 m).
- [ ] Adjust `ANTENNA_DELAY` in `uwb_config.h`. Raising it shortens the reading (about 4.7 mm per unit). Re-flash both boards each time.

**Pass:** the error is about 10 cm or less at 1, 2 and 5 m.

## 4. Three anchors
- [ ] Switch the repo to 3 anchors: `NUM_ANCHORS 3` and 3 rows of `ANCHOR_POS` in `uwb_config.h`, drop `[env:anchor4]` from `uwb_anchor/platformio.ini`, 3 `ANCHORS` in `tools/live_plot.py` (and a 3-anchor demo field).
- [ ] Fill in the anchor address TODO in `uwb_anchor/src/main.cpp`, then flash `anchor1` to `anchor3`.
- [ ] Place the anchors in a wide triangle around the field, all at the same height (never in a straight line). Measure their (x, y) and fill in `ANCHOR_POS` and the `ANCHORS` list in `live_plot.py`.
- [ ] Fill in the `newRange()` TODO in the tag so it stores a range per anchor.

**Pass:** the tag sees all three anchors (`81` to `83`).

With only 3 anchors there is no spare: if one is blocked, the tag gets no position until it comes back. Accuracy is best inside the triangle.

## 5. Trilateration
- [x] Fill in `lib/Trilateration/trilateration.cpp`.
- [x] `cd uwb_tag` then `pio test -e native`. 4 tests pass (exact, ±0.1 m noise, too few anchors, colinear anchors).
- [ ] Add a 3-anchor test, since that's the real setup.
- [ ] Fill in `allRangesFresh()` and the CSV TODO in the tag's `loop()`.

**Pass:** the unit tests pass and the tag prints `x,y,d1,d2,d3` lines.

## 6. Live plot + accuracy test
- [x] `pip install -r tools/requirements.txt` and fill in the `live_plot.py` TODOs.
- [x] `python tools/live_plot.py --demo` runs with a fake tag (no board needed).
- [ ] Run against the real tag: `python tools/live_plot.py --port COMx`.
- [ ] Put the tag on marked spots and log reported vs. true position.
- [ ] Repeat with a person standing between the tag and one anchor.

**Pass:** about 10–15 cm error with line of sight. Write down how blocking affects it.

## 7. Later
- Kalman filter to smooth the position.
- Custom PCB on the same pins as `uwb_config.h`.

## Setting up native tests on Windows
`pio test -e native` compiles on your PC, so it needs gcc:
`winget install BrechtSanders.WinLibs.POSIX.UCRT`, then restart the terminal and check `gcc --version`.

If `pio` isn't recognised, use the PlatformIO terminal (PlatformIO sidebar → Quick Access → PlatformIO Core CLI) or run `%USERPROFILE%\.platformio\penv\Scripts\pio.exe`.
