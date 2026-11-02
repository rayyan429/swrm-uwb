// UWB anchor: sits at a known spot and answers the tag's ranging requests.
// The DW1000Ranging library runs the two-way ranging protocol; this file
// just sets it up with the right address.
#include <Arduino.h>
#include <SPI.h>
#include <DW1000Ranging.h>
#include <uwb_config.h>

#ifndef ANCHOR_ID
#error "Build with an anchor env, e.g. pio run -e anchor1"
#endif

// 64-bit address (EUI). With randomShortAddress = false, the library uses
// the first byte as this anchor's short address, which is what the tag sees.
// TODO(milestone 4): every anchor currently gets the same address. Make the
// first byte (ANCHOR_ADDR_BASE + ANCHOR_ID) so anchor1 = "81:...",
// anchor2 = "82:..." etc. Hint: snprintf into this buffer in setup().
char anchorAddress[] = "81:00:5B:D5:A9:9A:E2:9C";

void newRange() {
    DW1000Device *dev = DW1000Ranging.getDistantDevice();
    Serial.printf("range from %04X: %.2f m\n", dev->getShortAddress(), dev->getRange());
}

void newDevice(DW1000Device *dev) {
    Serial.printf("tag joined: %04X\n", dev->getShortAddress());
}

void inactiveDevice(DW1000Device *dev) {
    Serial.printf("tag lost: %04X\n", dev->getShortAddress());
}

void setup() {
    Serial.begin(115200);
    delay(1000);
    Serial.printf("anchor %d starting\n", ANCHOR_ID);

    SPI.begin(PIN_SPI_SCK, PIN_SPI_MISO, PIN_SPI_MOSI);
    // Prints "Device ID: DECA0130" on success; that is milestone 1.
    DW1000Ranging.initCommunication(PIN_DW_RST, PIN_DW_CS, PIN_DW_IRQ);
    DW1000.setAntennaDelay(ANTENNA_DELAY);

    DW1000Ranging.attachNewRange(newRange);
    DW1000Ranging.attachNewDevice(newDevice);
    DW1000Ranging.attachInactiveDevice(inactiveDevice);

    DW1000Ranging.startAsAnchor(anchorAddress, DW1000.MODE_LONGDATA_RANGE_LOWPOWER, false);
}

void loop() {
    DW1000Ranging.loop();
}
