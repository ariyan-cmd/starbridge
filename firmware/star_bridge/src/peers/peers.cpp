#include "peers.h"

#include <string.h>
#include "../config/config.h"

static_assert(PEER_TABLE_LEN >= 1 && PEER_TABLE_LEN <= 255, "PEER_TABLE_LEN must be 1..255");

// Two nodes that answer every HELLO would ping-pong forever: skip a HELLO sent this soon after the last one.
static const uint32_t HELLO_MIN_GAP_MS = 1000;

static HelloPayload self;
static PeerSend send;
static PeerChange onChange;
static Peer table[PEER_TABLE_LEN];
static uint8_t count;
static uint32_t lastHello, lastBeat;

uint32_t peerTimeoutMs() { return (uint32_t)BEAT_INTERVAL_MS * MISSED_BEAT_LIMIT + BEAT_INTERVAL_MS / 2; }

static void sendHello(uint32_t now) {
  lastHello = now;
  send(MSG_HELLO, &self, sizeof(self));
}

void peersBegin(const HelloPayload &s, PeerSend sendFn, PeerChange changeFn, uint32_t now) {
  self = s;
  send = sendFn;
  onChange = changeFn;
  count = 0;
  lastBeat = now;
  sendHello(now);
}

void peersSetSelf(const HelloPayload &s, uint32_t now) {
  self = s;
  sendHello(now);
}

static Peer *find(uint32_t id) {
  for (uint8_t i = 0; i < count; i++)
    if (table[i].id == id) return &table[i];
  return nullptr;
}

void peersOnPacket(const Packet &p, uint32_t now) {
  Peer *peer = find(p.hdr.src_id);
  if (p.hdr.type == MSG_HELLO) {
    HelloPayload h;
    memcpy(&h, p.payload, sizeof(h));  // codec checked payload_len == sizeof(HelloPayload)
    bool joined = !peer && count < PEER_TABLE_LEN;
    if (joined) peer = &table[count++];
    if (peer) {
      peer->id = p.hdr.src_id;
      memcpy(peer->name, h.name, NODE_NAME_LEN);  // 16-byte bound: name is not NUL-terminated at full length
      peer->name[NODE_NAME_LEN] = '\0';
      peer->caps = h.caps;
      peer->fw_version = h.fw_version;
      peer->lastSeen = now;
      if (joined && onChange) onChange(*peer, true);
    }
    // Answer every HELLO, not only new ones: a peer that rebooted inside our timeout still needs our name.
    if (now - lastHello >= HELLO_MIN_GAP_MS) sendHello(now);
    return;
  }
  if (peer) peer->lastSeen = now;
  // We missed its HELLO. Not when full: we could not add it, and it would answer every beat.
  else if (p.hdr.type == MSG_BEAT && count < PEER_TABLE_LEN && now - lastHello >= HELLO_MIN_GAP_MS) sendHello(now);
}

void peersUpdate(uint32_t now) {
  if (now - lastBeat >= BEAT_INTERVAL_MS) {
    lastBeat = now;
    BeatPayload b = {now / 1000, 0};
    send(MSG_BEAT, &b, sizeof(b));
  }
  for (uint8_t i = 0; i < count;) {
    // Signed: lastSeen may be a few ms after now (packet stamped after loop() read millis()), and survives wrap.
    if ((int32_t)(now - table[i].lastSeen) <= (int32_t)peerTimeoutMs()) { i++; continue; }
    Peer gone = table[i];
    table[i] = table[--count];  // swap-remove: order is not kept
    if (onChange) onChange(gone, false);
  }
}

uint8_t peerCount() { return count; }
const Peer &peerAt(uint8_t i) { return table[i]; }
