// UWB anchor: sits at a known spot and answers the tag's ranging requests.
// lib/uwb_ranging runs the two-way ranging protocol; this file just sets it
// up with the right address.
#include <Arduino.h>
#include <uwb_config.h>
#include <uwb_ranging.h>

#ifndef ANCHOR_ID
#error "Build with an anchor env, e.g. pio run -e anchor1"
#endif

// Short address the tag polls this anchor by.
// TODO(milestone 4): every anchor currently gets the same address. Make it
// ANCHOR_ADDR_BASE + ANCHOR_ID so anchor1 = 0x81, anchor2 = 0x82 etc.
const uint8_t anchorAddress = ANCHOR_ADDR_BASE + 1;

uint32_t pollsAnswered = 0;
uint32_t lastPrintMs = 0;

void setup() {
    Serial.begin(115200);
    delay(1000);
    Serial.printf("anchor %d starting\n", ANCHOR_ID);

    if (!uwbInit()) {
        Serial.println("UWB init failed: check the wiring and power, then reset");
        while (true) {
            delay(1000);
        }
    }
    Serial.printf("listening as %02X\n", anchorAddress);
}

void loop() {
    if (uwbAnswerPoll(anchorAddress)) {
        pollsAnswered++;
    }

    // Once a second, show whether the tag is reaching this anchor.
    uint32_t now = millis();
    if (now - lastPrintMs >= 1000) {
        Serial.printf("answered %lu polls\n", (unsigned long)pollsAnswered);
        pollsAnswered = 0;
        lastPrintMs = now;
    }
}
