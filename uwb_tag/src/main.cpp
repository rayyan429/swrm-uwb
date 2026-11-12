// UWB tag: ranges to every anchor, then works out its own (x, y).
#include <Arduino.h>
#include <uwb_config.h>
#include <uwb_ranging.h>
#include <trilateration.h>

// Short address for this tag. Give each bot's tag a different one, below
// ANCHOR_ADDR_BASE so it can't clash with an anchor.
#define TAG_ADDRESS 0x01

// A range older than this is treated as missing (anchor blocked or off).
#define STALE_MS 500
// How often to compute and print a position.
#define REPORT_MS 100
// Pause after each ranging exchange so the anchor can turn its receiver back on.
#define RANGE_GAP_MS 10

float distances[NUM_ANCHORS];      // latest range to each anchor, metres
uint32_t lastSeenMs[NUM_ANCHORS];  // millis() when each range arrived (0 = never)
uint32_t lastReportMs = 0;

// Called for every successful range. addr is 0x81 .. for anchors 1 ...
void newRange(uint8_t addr, float range) {
    // Milestone 2: compare these against a tape measure at 1, 2 and 5 m.
    Serial.printf("anchor %02X: %.2f m\n", addr, range);

    // TODO(milestone 4): convert addr to an index 0 .. NUM_ANCHORS-1 using
    // ANCHOR_ADDR_BASE (anchor1 = 0x81 -> 0). Ignore anything outside that
    // range, then store range and millis() in distances[] and lastSeenMs[].
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

    if (!uwbInit()) {
        Serial.println("UWB init failed: check the wiring and power, then reset");
        while (true) {
            delay(1000);
        }
    }
}

void loop() {
    // Range with each anchor in turn; only the addressed anchor replies.
    for (int i = 0; i < NUM_ANCHORS; i++) {
        uint8_t addr = ANCHOR_ADDR_BASE + 1 + i;
        float range;
        if (uwbRangeTo(TAG_ADDRESS, addr, range)) {
            newRange(addr, range);
        }
        delay(RANGE_GAP_MS);
    }

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
