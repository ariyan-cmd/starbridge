// Host-side test for events. Lives outside the sketch so Arduino does not compile it.
// Run: g++ -std=c++11 -Wall -Wextra -I firmware/star_bridge/src/events test/events_test.cpp firmware/star_bridge/src/events/events.cpp firmware/star_bridge/src/peers/peers.cpp firmware/star_bridge/src/net/codec.cpp -o /tmp/events_test && /tmp/events_test
#include <stdio.h>
#include <string.h>

#include "events.h"
#include "../net/codec.h"
#include "../config/config.h"

static int fails;
#define CHECK(c) do { if (!(c)) { printf("FAIL %s:%d: %s\n", __FILE__, __LINE__, #c); fails++; } } while (0)

static const uint32_t A = 0xA1, B = 0xB2;
static uint32_t sender;  // node whose events module is "running"
static uint8_t frame[ESPNOW_MAX_FRAME];
static size_t frameLen;
static int sends;

// Plays the sketch's sendMsg: real codec, frame kept as if it went on air.
static bool fakeSend(uint8_t type, const void *payload, uint8_t len) {
  Packet out;
  frameLen = codecBuild(out, type, sender, payload, len);
  memcpy(frame, &out, frameLen);
  sends++;
  return frameLen != 0;
}

static Event got[EVENT_QUEUE_LEN + 4];
static int nGot;
static void handler(const Event &e) { got[nGot++] = e; }
static void echo(const Event &e) { handler(e); eventsLocal(e.code, e.arg); }

static int16_t temp = 2150;
static bool readTemp(int16_t &v) { v = temp; return true; }

static bool air(Packet &p) { return codecParse(frame, frameLen, p) == PARSE_OK; }

static bool peerSend(uint8_t, const void *, uint8_t) { return true; }  // peers' HELLO/BEAT, not under test
static void addPeer(uint32_t id) {
  Packet h = {};
  h.hdr.type = MSG_HELLO;
  h.hdr.src_id = id;
  peersOnPacket(h, 0);
}

static Packet telemetry(uint32_t src, uint8_t sensor, int16_t v) {
  Packet q = {};
  q.hdr.type = MSG_TELEMETRY;
  q.hdr.src_id = src;
  TelemetryPayload tp = {sensor, v};
  memcpy(q.payload, &tp, sizeof(tp));
  return q;
}

int main() {
  codecInit(100);
  Packet p;
  HelloPayload me = {};
  peersBegin(me, peerSend, nullptr, 0);  // one peer table plays both nodes' view: A and B know each other
  addPeer(A);
  addPeer(B);

  // Node A: a local gesture is broadcast at once and reaches A's handler on the next update.
  sender = A;
  eventsBegin(A, fakeSend, handler, nullptr, 0);
  eventsLocal(1, 0);  // SHAKE
  CHECK(sends == 1);
  CHECK(nGot == 0);
  eventsUpdate(10);
  CHECK(nGot == 1 && got[0].kind == EV_LOCAL && got[0].src == A && got[0].code == 1);
  CHECK(air(p) && p.hdr.type == MSG_EVENT && p.hdr.src_id == A);

  // Node B: the same frame arrives as EV_REMOTE from A.
  sender = B;
  nGot = 0;
  eventsBegin(B, fakeSend, handler, nullptr, 0);
  eventsOnPacket(p, 20);
  eventsUpdate(20);
  CHECK(nGot == 1 && got[0].kind == EV_REMOTE && got[0].src == A && got[0].code == 1 && got[0].arg == 0);

  // EVENT from a node that is not a peer is ignored.
  Packet stranger = p;
  stranger.hdr.src_id = 0xC3;
  nGot = 0;
  eventsOnPacket(stranger, 25);
  eventsUpdate(25);
  CHECK(nGot == 0);

  // B's own broadcast heard back is ignored.
  nGot = 0;
  eventsLocal(2, 412);  // TAP, peak 4.12 g
  eventsUpdate(30);
  CHECK(nGot == 1 && got[0].kind == EV_LOCAL);
  CHECK(air(p) && p.hdr.src_id == B);
  nGot = 0;
  eventsOnPacket(p, 31);
  eventsUpdate(31);
  CHECK(nGot == 0);

  // Negative arg survives the wire.
  sender = A;
  eventsBegin(A, fakeSend, handler, nullptr, 0);
  eventsLocal(3, -2);
  CHECK(air(p));
  sender = B;
  eventsBegin(B, fakeSend, handler, nullptr, 0);
  nGot = 0;
  eventsOnPacket(p, 40);
  eventsUpdate(40);
  CHECK(nGot == 1 && got[0].code == 3 && got[0].arg == -2);

  // Peer join/leave become events in order.
  Peer peer = {};
  peer.id = A;
  nGot = 0;
  eventsPeerChange(peer, true);
  eventsPeerChange(peer, false);
  eventsUpdate(50);
  CHECK(nGot == 2 && got[0].kind == EV_PEER_JOIN && got[1].kind == EV_PEER_LEAVE && got[1].src == A);

  // Full queue drops the newest and counts it; order kept across the ring wrap.
  nGot = 0;
  for (int i = 0; i < EVENT_QUEUE_LEN + 2; i++) eventsLocal(1, i);
  CHECK(eventsDropped() == 2);
  eventsUpdate(60);
  CHECK(nGot == EVENT_QUEUE_LEN);
  for (int i = 0; i < nGot; i++) CHECK(got[i].arg == i);

  // Telemetry: A sends only when due, B stores it, B forgets it when A leaves.
  sender = A;
  sends = 0;
  eventsBegin(A, fakeSend, handler, readTemp, 1000);
  eventsUpdate(1000 + TELEMETRY_INTERVAL_MS - 1);
  CHECK(sends == 0);
  eventsUpdate(1000 + TELEMETRY_INTERVAL_MS);
  CHECK(sends == 1);
  CHECK(air(p) && p.hdr.type == MSG_TELEMETRY);
  sender = B;
  sends = 0;
  eventsBegin(B, fakeSend, handler, nullptr, 0);
  eventsUpdate(1000 + 2 * TELEMETRY_INTERVAL_MS);
  CHECK(sends == 0);  // no sensor, nothing sent
  TelemetryPayload t;
  uint32_t at;
  CHECK(!peerReading(A, t, at));
  eventsOnPacket(p, 777);
  CHECK(peerReading(A, t, at) && t.sensor_id == SENSOR_TEMP && t.value_x100 == 2150 && at == 777);
  eventsPeerChange(peer, false);
  CHECK(!peerReading(A, t, at));

  // Only peers and only SENSOR_TEMP are stored: a non-peer would never get a LEAVE to free its slot.
  eventsOnPacket(telemetry(0xC3, SENSOR_TEMP, 1), 800);
  CHECK(!peerReading(0xC3, t, at));
  eventsOnPacket(telemetry(A, 9, 5), 801);
  CHECK(!peerReading(A, t, at));

  // A handler that raises an event per event does not spin: the new one waits for the next update.
  nGot = 0;
  eventsBegin(B, fakeSend, echo, nullptr, 0);
  eventsLocal(1, 0);
  eventsUpdate(900);
  CHECK(nGot == 1);
  eventsUpdate(901);
  CHECK(nGot == 2);

  printf("%s\n", fails ? "FAILED" : "all events checks pass");
  return fails != 0;
}
