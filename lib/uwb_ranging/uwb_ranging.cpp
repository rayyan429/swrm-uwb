#include "uwb_ranging.h"

#include <Arduino.h>
#include <SPI.h>
#include <dw3000.h>
#include <uwb_config.h>

// Radio settings. Must be identical on every board or they can't hear each
// other. Channel 5, 6.8 Mbps, no STS: the same as the Qorvo examples.
static dwt_config_t config = {
    5,                /* Channel number. */
    DWT_PLEN_128,     /* Preamble length. Used in TX only. */
    DWT_PAC8,         /* Preamble acquisition chunk size. Used in RX only. */
    9,                /* TX preamble code. Used in TX only. */
    9,                /* RX preamble code. Used in RX only. */
    1,                /* Non-standard 8 symbol SFD. */
    DWT_BR_6M8,       /* Data rate. */
    DWT_PHRMODE_STD,  /* PHY header mode. */
    DWT_PHRRATE_STD,  /* PHY header rate. */
    (129 + 8 - 8),    /* SFD timeout (preamble length + 1 + SFD length - PAC size). */
    DWT_STS_MODE_OFF, /* STS disabled. */
    DWT_STS_LEN_64,   /* STS length (unused with STS off). */
    DWT_PDOA_M0       /* PDOA off. */
};

// TX power and pulse shape, from the driver's defaults for channel 5.
extern dwt_txconfig_t txconfig_options;

// Timing in UWB microseconds, tuned for the ESP32 by Makerfabs. The anchor
// replies POLL_RX_TO_RESP_TX_DLY_UUS after the poll arrives. The tag turns its
// receiver on POLL_TX_TO_RESP_RX_DLY_UUS after sending and waits up to
// RESP_RX_TIMEOUT_UUS for the reply.
#define POLL_RX_TO_RESP_TX_DLY_UUS 900
#define POLL_TX_TO_RESP_RX_DLY_UUS 600
#define RESP_RX_TIMEOUT_UUS        400
// How long an anchor listens before uwbAnswerPoll() gives up and returns.
#define ANCHOR_RX_TIMEOUT_UUS      100000

// Frame layout (IEEE 802.15.4 data frame with 16-bit addresses):
//   0-1 frame control, 2 sequence number, 3-4 PAN ID,
//   5-6 destination address, 7-8 source address, 9 function code,
//   response only: 10-13 poll RX timestamp, 14-17 response TX timestamp.
// The DW3000 appends a 2-byte checksum, so each length includes 2 spare bytes.
#define IDX_SEQ        2
#define IDX_DST        5
#define IDX_SRC        7
#define IDX_FUNC       9
#define IDX_POLL_RX_TS 10
#define IDX_RESP_TX_TS 14
#define POLL_LEN       12
#define RESP_LEN       20

#define FUNC_POLL 0xE0
#define FUNC_RESP 0xE1

static uint8_t seqNum = 0;

static void writeHeader(uint8_t *frame, uint8_t dst, uint8_t src, uint8_t func) {
    frame[0] = 0x41;  // frame control: data frame, 16-bit addresses
    frame[1] = 0x88;
    frame[IDX_SEQ] = seqNum++;
    frame[3] = 0xCA;  // PAN ID 0xDECA
    frame[4] = 0xDE;
    frame[IDX_DST] = dst;
    frame[IDX_DST + 1] = 0;
    frame[IDX_SRC] = src;
    frame[IDX_SRC + 1] = 0;
    frame[IDX_FUNC] = func;
}

// True if the frame is ours (same frame control and PAN ID), is addressed to
// dst and carries the given function code.
static bool headerMatches(const uint8_t *frame, uint8_t dst, uint8_t func) {
    return frame[0] == 0x41 && frame[1] == 0x88 && frame[3] == 0xCA && frame[4] == 0xDE &&
           frame[IDX_DST] == dst && frame[IDX_DST + 1] == 0 && frame[IDX_FUNC] == func;
}

// Waits until the receiver gets a good frame, times out or errors. Copies a
// good frame into buf and returns its length (checksum included), or returns
// 0 if nothing usable arrived.
static uint32_t waitForFrame(uint8_t *buf, uint32_t bufLen) {
    uint32_t status;
    while (!((status = dwt_read32bitreg(SYS_STATUS_ID)) &
             (SYS_STATUS_RXFCG_BIT_MASK | SYS_STATUS_ALL_RX_TO | SYS_STATUS_ALL_RX_ERR))) {
    }
    if (!(status & SYS_STATUS_RXFCG_BIT_MASK)) {
        dwt_write32bitreg(SYS_STATUS_ID, SYS_STATUS_ALL_RX_TO | SYS_STATUS_ALL_RX_ERR);
        return 0;
    }
    dwt_write32bitreg(SYS_STATUS_ID, SYS_STATUS_RXFCG_BIT_MASK);

    uint32_t len = dwt_read32bitreg(RX_FINFO_ID) & RXFLEN_MASK;
    if (len > bufLen) {
        return 0;
    }
    dwt_readrxdata(buf, len, 0);
    return len;
}

bool uwbInit() {
    // The driver calls SPI.begin() with no pins; on the ESP32 a second begin()
    // is ignored, so starting it here first applies the pins in uwb_config.h.
    SPI.begin(PIN_SPI_SCK, PIN_SPI_MISO, PIN_SPI_MOSI);
    spiBegin(PIN_DW_IRQ, PIN_DW_RST);
    spiSelect(PIN_DW_CS);  // also resets the chip via RSTN (pulled low, never high)
    delay(2);              // DW3000 start-up (INIT_RC -> IDLE_RC)

    // Milestone 1: DECA0302 (DW3110) or DECA0312 (DW3120) means SPI works.
    // 00000000 or FFFFFFFF means check the wiring.
    Serial.printf("Device ID: %08lX\n", (unsigned long)dwt_readdevid());

    if (!dwt_checkidlerc()) {
        Serial.println("UWB: chip not idle (check 3V3, GND and RSTN)");
        return false;
    }
    if (dwt_initialise(DWT_DW_INIT) == DWT_ERROR) {
        Serial.println("UWB: initialise failed");
        return false;
    }
    if (dwt_configure(&config)) {
        Serial.println("UWB: configure failed (PLL or RX calibration)");
        return false;
    }
    dwt_configuretxrf(&txconfig_options);
    dwt_setrxantennadelay(ANTENNA_DELAY);
    dwt_settxantennadelay(ANTENNA_DELAY);
    dwt_setlnapamode(DWT_LNA_ENABLE | DWT_PA_ENABLE);
    return true;
}

bool uwbRangeTo(uint8_t myAddr, uint8_t anchorAddr, float &distance) {
    uint8_t poll[POLL_LEN] = {0};
    writeHeader(poll, anchorAddr, myAddr, FUNC_POLL);

    // Send the poll; the receiver turns on by itself to catch the reply.
    dwt_setrxaftertxdelay(POLL_TX_TO_RESP_RX_DLY_UUS);
    dwt_setrxtimeout(RESP_RX_TIMEOUT_UUS);
    dwt_write32bitreg(SYS_STATUS_ID, SYS_STATUS_TXFRS_BIT_MASK);
    dwt_writetxdata(sizeof(poll), poll, 0);
    dwt_writetxfctrl(sizeof(poll), 0, 1);
    dwt_starttx(DWT_START_TX_IMMEDIATE | DWT_RESPONSE_EXPECTED);

    uint8_t resp[RESP_LEN];
    if (waitForFrame(resp, sizeof(resp)) != RESP_LEN || !headerMatches(resp, myAddr, FUNC_RESP) ||
        resp[IDX_SRC] != anchorAddr) {
        return false;
    }

    // Low 32 bits of each 40-bit timestamp are enough: one exchange takes far
    // less than 2^32 ticks (~67 ms), so the subtractions below can't wrap twice.
    uint32_t pollTx = dwt_readtxtimestamplo32();
    uint32_t respRx = dwt_readrxtimestamplo32();
    uint32_t pollRx, respTx;
    resp_msg_get_ts(&resp[IDX_POLL_RX_TS], &pollRx);
    resp_msg_get_ts(&resp[IDX_RESP_TX_TS], &respTx);

    // Corrects for the anchor's crystal running slightly fast or slow.
    float clockOffsetRatio = ((float)dwt_readclockoffset()) / (uint32_t)(1 << 26);

    int32_t roundTrip = respRx - pollTx;  // tag: poll out -> reply in
    int32_t replyDelay = respTx - pollRx; // anchor: poll in -> reply out
    double tof = ((roundTrip - replyDelay * (1 - clockOffsetRatio)) / 2.0) * DWT_TIME_UNITS;
    distance = tof * SPEED_OF_LIGHT;
    return true;
}

uint8_t uwbAnswerPoll(uint8_t myAddr) {
    dwt_setrxtimeout(ANCHOR_RX_TIMEOUT_UUS);
    dwt_rxenable(DWT_START_RX_IMMEDIATE);

    uint8_t poll[POLL_LEN];
    if (waitForFrame(poll, sizeof(poll)) != POLL_LEN || !headerMatches(poll, myAddr, FUNC_POLL)) {
        return 0;
    }
    uint8_t tagAddr = poll[IDX_SRC];

    // Schedule the reply a fixed delay after the poll arrived, and put both
    // times in it so the tag can subtract our turnaround.
    uint64_t pollRx = get_rx_timestamp_u64();
    uint32_t respTxTime = (pollRx + (POLL_RX_TO_RESP_TX_DLY_UUS * UUS_TO_DWT_TIME)) >> 8;
    dwt_setdelayedtrxtime(respTxTime);
    uint64_t respTx = (((uint64_t)(respTxTime & 0xFFFFFFFEUL)) << 8) + ANTENNA_DELAY;

    uint8_t resp[RESP_LEN] = {0};
    writeHeader(resp, tagAddr, myAddr, FUNC_RESP);
    resp_msg_set_ts(&resp[IDX_POLL_RX_TS], pollRx);
    resp_msg_set_ts(&resp[IDX_RESP_TX_TS], respTx);
    dwt_writetxdata(sizeof(resp), resp, 0);
    dwt_writetxfctrl(sizeof(resp), 0, 1);

    // Fails if we were too slow and the scheduled time has already passed.
    if (dwt_starttx(DWT_START_TX_DELAYED) != DWT_SUCCESS) {
        return 0;
    }
    while (!(dwt_read32bitreg(SYS_STATUS_ID) & SYS_STATUS_TXFRS_BIT_MASK)) {
    }
    dwt_write32bitreg(SYS_STATUS_ID, SYS_STATUS_TXFRS_BIT_MASK);
    return tagAddr;
}
