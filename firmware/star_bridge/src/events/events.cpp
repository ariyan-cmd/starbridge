#include "events.h"

#include <string.h>
#include "../config/config.h"

static_assert(EVENT_QUEUE_LEN >= 1 && EVENT_QUEUE_LEN <= 255, "EVENT_QUEUE_LEN must be 1..255");

struct Reading {
  uint32_t id;
  uint32_t at;
  TelemetryPayload t;
};

static uint32_t self;
static PeerSend send;
static EventHandler handler;
static TempRead readTemp;
static Event queue[EVENT_QUEUE_LEN];
static uint8_t head, count;  // head = oldest slot
static uint32_t dropped, lastTelemetry;
static Reading readings[PEER_TABLE_LEN];  // one per peer, same bound as the peer table
static uint8_t nReadings;

void eventsBegin(uint32_t selfId, PeerSend sendFn, EventHandler h, TempRead r, uint32_t now) {
  self = selfId;
  send = sendFn;
  handler = h;
  readTemp = r;
  head = count = nReadings = 0;
  dropped = 0;
  lastTelemetry = now;
}

static void push(uint32_t src, uint8_t kind, uint8_t code, int16_t arg) {
  if (count == EVENT_QUEUE_LEN) { dropped++; return; }  // drop newest, like the RX queue
  queue[(head + count++) % EVENT_QUEUE_LEN] = {src, kind, code, arg};
}

void eventsLocal(uint8_t code, int16_t arg) {
  EventPayload e = {code, arg};
  if (send) send(MSG_EVENT, &e, sizeof(e));
  push(self, EV_LOCAL, code, arg);
}

static Reading *findReading(uint32_t id) {
  for (uint8_t i = 0; i < nReadings; i++)
    if (readings[i].id == id) return &readings[i];
  return nullptr;
}

void eventsPeerChange(const Peer &p, bool joined) {
  if (!joined) {
    Reading *r = findReading(p.id);
    if (r) *r = readings[--nReadings];  // swap-remove
  }
  push(p.id, joined ? EV_PEER_JOIN : EV_PEER_LEAVE, 0, 0);
}

static bool isPeer(uint32_t id) {
  for (uint8_t i = 0; i < peerCount(); i++)
    if (peerAt(i).id == id) return true;
  return false;
}

void eventsOnPacket(const Packet &p, uint32_t now) {
  uint32_t src = p.hdr.src_id;
  // Peers only (feed peersOnPacket first): a node the full peer table ignores, or one that timed out,
  // would never get a LEAVE, so its reading would hold a slot forever.
  if (src == self || !isPeer(src)) return;
  if (p.hdr.type == MSG_EVENT) {
    EventPayload e;
    memcpy(&e, p.payload, sizeof(e));  // codec checked payload_len == sizeof(EventPayload)
    push(src, EV_REMOTE, e.code, e.arg);
  } else if (p.hdr.type == MSG_TELEMETRY && p.payload[0] == SENSOR_TEMP) {
    // shortcut: one reading per peer, temperature only; key by (id, sensor_id) when a second sensor type exists.
    Reading *r = findReading(src);
    if (!r && nReadings < PEER_TABLE_LEN) r = &readings[nReadings++];  // peers only, so full only if a LEAVE was missed
    if (!r) return;
    r->id = src;
    r->at = now;
    memcpy(&r->t, p.payload, sizeof(r->t));
  }
}

void eventsUpdate(uint32_t now) {
  for (uint8_t n = count; n && count; n--) {  // only what was queued before this call: a handler that raises events cannot spin
    Event e = queue[head];
    head = (head + 1) % EVENT_QUEUE_LEN;
    count--;
    if (handler) handler(e);  // after the pop, so an event the handler raises is not overwritten
  }
  if (readTemp && send && now - lastTelemetry >= TELEMETRY_INTERVAL_MS) {
    lastTelemetry = now;
    int16_t v;  // packed field cannot bind to a reference
    if (readTemp(v)) {
      TelemetryPayload t = {SENSOR_TEMP, v};
      send(MSG_TELEMETRY, &t, sizeof(t));
    }
  }
}

uint32_t eventsDropped() { return dropped; }

bool peerReading(uint32_t id, TelemetryPayload &out, uint32_t &at) {
  Reading *r = findReading(id);
  if (!r) return false;
  out = r->t;
  at = r->at;
  return true;
}
