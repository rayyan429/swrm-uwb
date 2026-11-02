# Roadmap

Work through these in order. Each milestone has a pass check, so if something breaks you know which layer it's in. See [FILE_LAYOUT.md](FILE_LAYOUT.md) for where everything lives.

**Unplug and replug the board after uploading.** The DW1000 sometimes doesn't reset cleanly otherwise.

## 1. Smoke test
- [ ] `cd uwb_anchor` then `pio run -e anchor1 -t upload` then `pio device monitor`
- [ ] Do the same for `uwb_tag` on the second board.

**Pass:** both print `Device ID: DECA0130`. If you see `00000000` or `FFFFFFFF`, the SPI link is wrong (check the pins in `lib/uwb_config/uwb_config.h`).

## 2. Two-board ranging
- [ ] One anchor and the tag powered. The tag prints `anchor 81: x.xx m`.
- [ ] Record readings at 1, 2 and 5 m (tape measure, line of sight).

**Pass:** readings are stable. They will read long by a roughly constant amount until milestone 3.

## 3. Antenna delay calibration
- [ ] Hold the tag at a known distance (e.g. 2.00 m).
- [ ] Adjust `ANTENNA_DELAY` in `uwb_config.h`. Raising it shortens the reading (about 4.7 mm per unit). Re-flash both boards each time.

**Pass:** the error is about 10 cm or less at 1, 2 and 5 m.

## 4. Four anchors
- [ ] Fill in the anchor address TODO in `uwb_anchor/src/main.cpp`, then flash `anchor1` to `anchor4`.
- [ ] Place the anchors at the field corners, all at the same height. Measure their (x, y) and fill in `ANCHOR_POS`.
- [ ] Fill in the `newRange()` TODO in the tag so it stores a range per anchor.

**Pass:** the tag sees all four anchors (`81` to `84`).

## 5. Trilateration
- [ ] Fill in `lib/Trilateration/trilateration.cpp`.
- [ ] `cd uwb_tag` then `pio test -e native` (needs gcc, see below). Add the extra tests listed in the TODO.
- [ ] Fill in `allRangesFresh()` and the CSV TODO in the tag's `loop()`.

**Pass:** the unit tests pass and the tag prints `x,y,d1,d2,d3,d4` lines.

## 6. Live plot + accuracy test
- [ ] `pip install -r tools/requirements.txt` and fill in the `live_plot.py` TODOs.
- [ ] Put the tag on marked spots and log reported vs. true position.
- [ ] Repeat with a person standing between the tag and one anchor.

**Pass:** about 10–15 cm error with line of sight. Write down how blocking affects it.

## 7. Later
- Kalman filter to smooth the position.
- Custom PCB on the same pins as `uwb_config.h`.

## Setting up native tests on Windows
`pio test -e native` compiles on your PC, so it needs gcc:
`winget install BrechtSanders.WinLibs.POSIX.UCRT`, then restart the terminal and check `gcc --version`.
