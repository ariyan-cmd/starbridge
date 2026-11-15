// ESP-NOW broadcast transport for the ESP8266 core. Receive only queues; transportPoll() does the work.
#pragma once

#include <stdint.h>
#include "protocol.h"

// Called from transportPoll() (loop context) for every frame codecParse accepts.
// No RSSI: the ESP8266 receive callback does not report it (see task_report.md).
typedef void (*PacketHandler)(const Packet &p);

// Station mode on ESPNOW_CHANNEL, role COMBO, broadcast peer added. False if the SDK refuses.
bool transportInit(PacketHandler onPacket);

// Broadcasts len bytes (1..ESPNOW_MAX_FRAME). False if the radio refused it at once;
// the send callback's verdict arrives later in transportStats().
bool transportSend(const void *frame, uint8_t len);

// Drains the receive queue through codecParse and the handler. Call every loop().
void transportPoll();

struct TransportStats {
  uint32_t rx;         // frames queued by the receive callback
  uint32_t rxDropped;  // frames lost because the queue was full (or bad length)
  uint32_t parseErr;   // frames codecParse rejected (duplicates included)
  uint32_t sendOk;     // send callback status 0. Broadcast has no ACK: this means "went on air"
  uint32_t sendFail;   // send callback non-zero, or transportSend refused
  uint8_t queued;      // frames waiting now
};
TransportStats transportStats();
