# File Layout

Planned structure for the SWRM UWB positioning system: ESP32 + Ai-Thinker BU03 (DW3000 chip), 4 anchors, 1 tag, PlatformIO.

The firmware was first written for the DW1000, and `lib/DW1000` is still in place until the DW3000 port (milestone 0 in [ROADMAP.md](ROADMAP.md)).

- **C++ (ESP32, PlatformIO):** ranging firmware and trilateration. The tag solves its own position.
- **Python (PC):** reads the tag's serial output for live plotting and accuracy logging.

## Repo layout

```mermaid
flowchart TD
  root["swrm-uwb/"]
  root --> lib["lib/<br/>shared by both projects via lib_extra_dirs"]
  root --> anchor["uwb_anchor/"]
  root --> tag["uwb_tag/"]
  root --> tools["tools/"]
  root --> docs["docs/"]

  lib --> dw["DW1000/<br/>ranging library<br/>TODO: replace with DW3000 driver"]:::existing
  lib --> cfg["uwb_config/uwb_config.h<br/>pins, anchor IDs,<br/>anchor (x,y) positions, antenna delay"]:::new
  lib --> tri["Trilateration/"]
  tri --> trih["trilateration.h<br/>function signatures + docs"]:::new
  tri --> tric["trilateration.cpp<br/>TODO: linearize, least-squares solve"]:::blank

  anchor --> ap["platformio.ini<br/>envs anchor1 to anchor4 (-DANCHOR_ID=N)"]:::edit
  anchor --> am["src/main.cpp<br/>TODO: address, antenna delay"]:::blank

  tag --> tp["platformio.ini<br/>+ env:native to run tests on PC"]:::edit
  tag --> tm["src/main.cpp<br/>TODO: store range per anchor,<br/>call solver, print CSV"]:::blank
  tag --> tt["test/test_trilateration/<br/>TODO: more test cases"]:::blank

  tools --> req["requirements.txt<br/>pyserial, matplotlib"]:::new
  tools --> lp["live_plot.py<br/>TODO: parse CSV, draw"]:::blank

  docs --> rm["ROADMAP.md<br/>milestones + checklist"]:::new
  docs --> fl["FILE_LAYOUT.md<br/>this file"]:::new

  classDef existing fill:#e5e7eb,stroke:#6b7280,color:#111
  classDef new fill:#dbeafe,stroke:#2563eb,color:#111
  classDef edit fill:#fef3c7,stroke:#d97706,color:#111
  classDef blank fill:#dcfce7,stroke:#16a34a,color:#111
```

| Color | Meaning |
|---|---|
| Gray | Already in the repo |
| Blue | New, written as starter code |
| Yellow | Existing file, edited |
| Green | Starter code with **TODO blanks to fill in** |

## How data flows

```mermaid
flowchart LR
  A1["Anchor 1<br/>ESP32 + BU03"]
  A2["Anchor 2"]
  A3["Anchor 3"]
  A4["Anchor 4"]

  subgraph T["Tag: ESP32 + BU03"]
    R["Two-way ranging<br/>distance d1..d4"] --> S["trilaterate()<br/>least squares, all anchors"]
    S --> O["Serial CSV<br/>x,y,d1,d2,d3,d4"]
  end

  A1 -- "UWB two-way ranging" --> R
  A2 --> R
  A3 --> R
  A4 --> R
  O -- "USB serial, 115200" --> P["tools/live_plot.py<br/>PC, Python"]
```

The BU03 talks to the ESP32 over SPI (CSN, MOSI, MISO, CLK, plus IRQ, RSTN and WAKEUP). SCK 18, MISO 19 and MOSI 23 are the ESP32's default SPI pins. CS, RST and IRQ depend on your wiring. All of them live in `uwb_config.h`, so a custom PCB can reuse them.

## Milestones

0. **DW3000 port:** pick the ESP32 board and wiring, swap the DW1000 library for a DW3000 driver.
1. **Smoke test:** read the DW3000 device ID (`DECA0302` or `DECA0312`) over SPI.
2. **Two-board ranging:** check against a tape measure at 1, 2 and 5 m.
3. **Antenna delay calibration:** error of about 10 cm or less at a known distance.
4. **4 anchors:** tag collects a distance from each.
5. **Trilateration:** unit tests pass on PC, then the solver runs on the tag.
6. **Live plot + accuracy test:** marked spots, with and without blocking.
7. **Later:** Kalman smoothing, then the custom PCB on the same pins.
