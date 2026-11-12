// Two-way ranging on the DW3000 (Ai-Thinker BU03), shared by the anchors and
// the tag. Single-sided two-way ranging, based on Qorvo's ex_06a/ex_06b
// examples with Makerfabs's ESP32 timings:
//
//   tag  --- poll, addressed to one anchor --->  anchor
//   tag  <--- response with 2 timestamps ------  anchor
//
// The tag measures its own round trip, subtracts how long the anchor waited
// before replying, and halves the rest: that is the time of flight.
// Only the addressed anchor replies, so the tag ranges with one at a time.
#pragma once

#include <stdint.h>

// Resets and configures the DW3000 using the pins in uwb_config.h, and prints
// its device ID ("Device ID: DECA0302"); that print is milestone 1.
// Returns false if the chip doesn't answer or won't configure.
bool uwbInit();

// Tag: ranges with the anchor at anchorAddr. On success writes the distance
// in metres to distance and returns true; returns false if the anchor didn't
// answer.
bool uwbRangeTo(uint8_t myAddr, uint8_t anchorAddr, float &distance);

// Anchor: listens for up to ~100 ms and answers a poll addressed to myAddr.
// Call it from loop(). Returns the address of the tag it answered, or 0 if
// nothing arrived for this anchor.
uint8_t uwbAnswerPoll(uint8_t myAddr);
