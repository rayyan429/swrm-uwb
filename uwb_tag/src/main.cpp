// UWB tag: ranges to every anchor, then works out its own (x, y).
#include <Arduino.h>
#include <SPI.h>
#include <DW1000Ranging.h>
#include <uwb_config.h>
#include <trilateration.h>

// Unique 64-bit address for this tag. Give each bot's tag a different one.
char tagAddress[] = "7D:00:22:EA:82:60:3B:9C";

// A range older than this is treated as missing (anchor blocked or off).
#define STALE_MS 500
// How often to compute and print a position.
#define REPORT_MS 100

float distances[NUM_ANCHORS];      // latest range to each anchor, metres
uint32_t lastSeenMs[NUM_ANCHORS];  // millis() when each range arrived (0 = never)
uint32_t lastReportMs = 0;

void newRange() {
    DW1000Device *dev = DW1000Ranging.getDistantDevice();
    uint8_t addr = dev->getShortAddress() & 0xFF;  // 0x81 .. 0x84 for anchors 1..4
    float range = dev->getRange();

    // Milestone 2: compare these against a tape measure at 1, 2 and 5 m.
    Serial.printf("anchor %02X: %.2f m\n", addr, range);

    // TODO(milestone 4): convert addr to an index 0 .. NUM_ANCHORS-1 using
    // ANCHOR_ADDR_BASE (anchor1 = 0x81 -> 0). Ignore anything outside that
    // range, then store range and millis() in distances[] and lastSeenMs[].
}

void newDevice(DW1000Device *dev) {
    Serial.printf("anchor found: %04X\n", dev->getShortAddress());
}

void inactiveDevice(DW1000Device *dev) {
    Serial.printf("anchor lost: %04X\n", dev->getShortAddress());
}

// True if every anchor has a range newer than STALE_MS.
bool allRangesFresh(uint32_t now) {
    // TODO(milestone 5): check lastSeenMs[] for every anchor.
    (void)now;
    return false;
}

void setup() {
    Serial.begin(115200);
    delay(1000);
    Serial.println("tag starting");

    SPI.begin(PIN_SPI_SCK, PIN_SPI_MISO, PIN_SPI_MOSI);
    // Prints "Device ID: DECA0130" on success; that is milestone 1.
    DW1000Ranging.initCommunication(PIN_DW_RST, PIN_DW_CS, PIN_DW_IRQ);
    DW1000.setAntennaDelay(ANTENNA_DELAY);

    DW1000Ranging.attachNewRange(newRange);
    DW1000Ranging.attachNewDevice(newDevice);
    DW1000Ranging.attachInactiveDevice(inactiveDevice);

    DW1000Ranging.startAsTag(tagAddress, DW1000.MODE_LONGDATA_RANGE_LOWPOWER, false);
}

void loop() {
    DW1000Ranging.loop();

    uint32_t now = millis();
    if (now - lastReportMs < REPORT_MS) {
        return;
    }
    lastReportMs = now;

    if (allRangesFresh(now)) {
        // TODO(milestone 5): call trilaterate2D(ANCHOR_POS, distances, NUM_ANCHORS, p).
        // On success print one CSV line for tools/live_plot.py:
        //   x,y,d1,d2,d3,d4     e.g.  1.92,0.88,2.11,3.40,2.95,1.67
        // Once that works, remove the per-range print in newRange() so the
        // serial output is just CSV.
    }
}
