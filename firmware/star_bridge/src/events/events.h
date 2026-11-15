#pragma once
// events: the bus between gestures, face and network. No Arduino calls (host-testable); time comes in as `now` (ms).
// Local gestures go out as EVENT at once; local, remote and peer events reach the handler from eventsUpdate().
#include <stdint.h>
#include "../net/protocol.h"
#include "../peers/peers.h"

enum EventKind : uint8_t {
  EV_LOCAL = 1,       // our own gesture, already broadcast
  EV_REMOTE = 2,      // EVENT packet from a peer
  EV_PEER_JOIN = 3,   // code and arg are 0
  EV_PEER_LEAVE = 4,  // code and arg are 0
};

struct Event {
  uint32_t src;  // node that made it: our ID for EV_LOCAL, else the peer's ID
  uint8_t kind;  // EventKind
  uint8_t code;  // EVENT code, same values as GestureCode (docs/protocol.md)
  int16_t arg;
};

typedef void (*EventHandler)(const Event &e);
// Temperature in degrees C x100. False if no reading this time. Pass nullptr when no sensor is fitted.
typedef bool (*TempRead)(int16_t &value_x100);

// Clears the queue and peer readings. send is the same callback peers uses. Call once after transportInit.
void eventsBegin(uint32_t selfId, PeerSend send, EventHandler handler, TempRead readTemp, uint32_t now);

void eventsLocal(uint8_t code, int16_t arg);           // From the gesture callback: broadcasts EVENT, queues EV_LOCAL.
void eventsPeerChange(const Peer &p, bool joined);     // From the peers change callback.
void eventsOnPacket(const Packet &p, uint32_t now);    // Every accepted packet. EVENT and TELEMETRY from self are ignored.

// Hands queued events to the handler (oldest first) and sends TELEMETRY every TELEMETRY_INTERVAL_MS. Call every loop().
void eventsUpdate(uint32_t now);

uint32_t eventsDropped();  // events lost because the queue was full

// Latest TELEMETRY from a peer, kept until it leaves. False if none heard.
bool peerReading(uint32_t id, TelemetryPayload &out, uint32_t &at);
