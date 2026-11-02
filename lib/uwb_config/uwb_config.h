// Shared settings for every board in the system (anchors and tag).
// Change things here, not in the individual main.cpp files.
#pragma once

// ---- Pins: ESP32 wired to an Ai-Thinker BU03 (DW3000) ----
// The BU03 is a bare SMD module, so these depend on your carrier board / wiring.
// SCK/MISO/MOSI are the ESP32's default VSPI pins.
// TODO(milestone 0): set CS, RST and IRQ to match your wiring. These are
// placeholders left over from the DWM1000 board.
// BU03 notes: RSTN must never be driven high (drive low or leave floating),
// and tie WAKEUP to GND if you don't use it.
// A custom PCB that keeps these pins can run the same firmware unchanged.
#define PIN_SPI_SCK   18
#define PIN_SPI_MISO  19
#define PIN_SPI_MOSI  23
#define PIN_DW_CS      4
#define PIN_DW_RST    27
#define PIN_DW_IRQ    34

// ---- Network ----
#define NUM_ANCHORS 4

// Anchor N uses short address (ANCHOR_ADDR_BASE + N): anchor1 = 0x81 ... anchor4 = 0x84.
// The tag uses this to work out which anchor a range came from.
#define ANCHOR_ADDR_BASE 0x80

// ---- Calibration ----
// Antenna delay in DW3000 time units (~15.65 ps each, so 1 unit ~ 4.7 mm of range).
// 16385 is Qorvo's uncalibrated example default; expect it to read off by tens of cm.
// TODO(milestone 3): replace with your measured value.
#define ANTENNA_DELAY 16385

// ---- Field geometry ----
// Anchor (x, y) positions in metres, measured from your chosen origin corner.
// Keep all anchors at the same height, ideally the tag's height.
// TODO(milestone 4): tape-measure your field and fill these in.
// Keep tools/live_plot.py ANCHORS in sync with this table.
static const float ANCHOR_POS[NUM_ANCHORS][2] = {
    {0.0f, 0.0f},  // anchor 1 (origin)
    {0.0f, 0.0f},  // anchor 2
    {0.0f, 0.0f},  // anchor 3
    {0.0f, 0.0f},  // anchor 4
};
