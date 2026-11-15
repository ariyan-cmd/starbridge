#pragma once
// peers: HELLO, BEAT, peer table and timeout. No Arduino calls (host-testable); time comes in as `now` (ms).
#include <stdint.h>
#include "../net/protocol.h"

struct Peer {
  uint32_t id;                    // src_id
  char name[NODE_NAME_LEN + 1];   // NUL-terminated copy of HelloPayload::name
  uint16_t caps;                  // CapBits
  uint8_t fw_version;
  uint32_t lastSeen;              // ms, any accepted packet from this peer
};

// Builds and broadcasts one packet. True if the radio took it.
typedef bool (*PeerSend)(uint8_t type, const void *payload, uint8_t len);
// joined = true when a peer is added, false after it timed out (p is a copy, no longer in the table).
typedef void (*PeerChange)(const Peer &p, bool joined);

// Peer dropped when nothing heard for this long: MISSED_BEAT_LIMIT beats plus half a beat of jitter margin.
uint32_t peerTimeoutMs();

// Stores self for HELLO and broadcasts it. Call once after transportInit.
void peersBegin(const HelloPayload &self, PeerSend send, PeerChange onChange, uint32_t now);

// Replaces self (after a rename) and broadcasts the new HELLO at once.
void peersSetSelf(const HelloPayload &self, uint32_t now);

// Feed every accepted packet. HELLO adds or refreshes a peer and is answered with our HELLO;
// a BEAT from an unknown peer gets our HELLO so it re-introduces itself. When the table is full
// (PEER_TABLE_LEN) a new peer is ignored until a slot frees.
void peersOnPacket(const Packet &p, uint32_t now);

// Sends BEAT every BEAT_INTERVAL_MS and drops timed-out peers. Call every loop().
void peersUpdate(uint32_t now);

uint8_t peerCount();
const Peer &peerAt(uint8_t i);  // 0..peerCount()-1; order changes when a peer is dropped
