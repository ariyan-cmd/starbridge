// Host-side test for peers. Lives outside the sketch so Arduino does not compile it.
// Run: g++ -std=c++11 -Wall -Wextra -I firmware/star_bridge/src/peers test/peers_test.cpp firmware/star_bridge/src/peers/peers.cpp -o /tmp/peers_test && /tmp/peers_test
#include <stdio.h>
#include <string.h>

#include "peers.h"
#include "../config/config.h"

static int fails;
#define CHECK(c) do { if (!(c)) { printf("FAIL %s:%d: %s\n", __FILE__, __LINE__, #c); fails++; } } while (0)

static int hellos, beats, joins, leaves;
static Peer lastChange;

static bool fakeSend(uint8_t type, const void *, uint8_t) {
  if (type == MSG_HELLO) hellos++;
  if (type == MSG_BEAT) beats++;
  return true;
}
static void fakeChange(const Peer &p, bool joined) { (joined ? joins : leaves)++; lastChange = p; }

static Packet hello(uint32_t src, const char *name) {
  Packet p = {};
  p.hdr.type = MSG_HELLO;
  p.hdr.src_id = src;
  HelloPayload h = {};
  strncpy(h.name, name, NODE_NAME_LEN);
  h.caps = CAP_DISPLAY;
  h.fw_version = 3;
  memcpy(p.payload, &h, sizeof(h));
  return p;
}
static Packet beat(uint32_t src) {
  Packet p = {};
  p.hdr.type = MSG_BEAT;
  p.hdr.src_id = src;
  return p;
}

int main() {
  HelloPayload self = {};
  strncpy(self.name, "me", NODE_NAME_LEN);
  peersBegin(self, fakeSend, fakeChange, 1000);
  CHECK(hellos == 1 && peerCount() == 0);

  // Reply to a HELLO right after our own boot HELLO is suppressed (no ping-pong).
  peersOnPacket(hello(0xA, "alpha"), 1010);
  CHECK(peerCount() == 1 && joins == 1 && hellos == 1);
  CHECK(strcmp(peerAt(0).name, "alpha") == 0 && peerAt(0).caps == CAP_DISPLAY && peerAt(0).fw_version == 3);

  // A later HELLO (peer booted after us) is answered; a repeat from a known peer is not a join.
  peersOnPacket(hello(0xB, "bravo-full-16chr"), 3000);
  CHECK(hellos == 2 && joins == 2 && strcmp(peerAt(1).name, "bravo-full-16chr") == 0);
  peersOnPacket(hello(0xB, "bravo2"), 4500);
  CHECK(hellos == 3 && joins == 2 && strcmp(peerAt(1).name, "bravo2") == 0);

  // BEAT from an unknown node triggers our HELLO, does not add it.
  peersOnPacket(beat(0xC), 6000);
  CHECK(hellos == 4 && peerCount() == 2);

  // BEAT every BEAT_INTERVAL_MS. `now` here is older than lastSeen of B (4500), which must not drop it.
  peersUpdate(1000 + BEAT_INTERVAL_MS - 1);
  CHECK(beats == 0);
  peersUpdate(1000 + BEAT_INTERVAL_MS);
  CHECK(beats == 1 && peerCount() == 2);

  // Timeout: B keeps beating, A goes silent and is dropped once peerTimeoutMs() passes.
  uint32_t t = 6000;
  for (; t <= 1010 + peerTimeoutMs(); t += 500) { peersOnPacket(beat(0xB), t); peersUpdate(t); }
  CHECK(leaves == 0 && peerCount() == 2);
  peersOnPacket(beat(0xB), t);
  peersUpdate(t);
  CHECK(leaves == 1 && lastChange.id == 0xA && peerCount() == 1 && peerAt(0).id == 0xB);

  // A comes back.
  peersOnPacket(hello(0xA, "alpha"), t + 100);
  CHECK(joins == 3 && peerCount() == 2);

  // Full table: extra peers are ignored, no join.
  for (uint32_t id = 0x100; peerCount() < PEER_TABLE_LEN; id++) peersOnPacket(hello(id, "x"), t + 200);
  int j = joins;
  peersOnPacket(hello(0xFFFF, "late"), t + 300);
  CHECK(joins == j && peerCount() == PEER_TABLE_LEN);
  int h = hellos;
  peersOnPacket(beat(0xFFFF), t + 2000);  // full: an unknown BEAT must not start a HELLO exchange
  CHECK(hellos == h);
  peersSetSelf(self, t + 2001);  // rename sends at once, even inside the 1 s gap
  CHECK(hellos == h + 1);

  // millis() wrap: a peer seen just before the wrap is not dropped just after it.
  peersBegin(self, fakeSend, fakeChange, 0xFFFFFF00u);
  peersOnPacket(hello(0xA, "alpha"), 0xFFFFFF00u);
  peersUpdate(0x100);
  CHECK(peerCount() == 1);

  printf("sizeof(Peer) = %zu, table = %zu B, timeout %u ms\n", sizeof(Peer), sizeof(Peer) * PEER_TABLE_LEN,
         (unsigned)peerTimeoutMs());
  printf(fails ? "%d FAILED\n" : "all passed\n", fails);
  return fails != 0;
}
