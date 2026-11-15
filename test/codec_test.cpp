// Host-side test for net/codec. Lives outside the sketch so Arduino does not compile it.
// Run: g++ -std=c++11 -Wall -Wextra -I firmware/star_bridge/src/net test/codec_test.cpp firmware/star_bridge/src/net/codec.cpp -o /tmp/codec_test && /tmp/codec_test
#include <stdio.h>
#include <string.h>

#include "codec.h"

static int fails;
#define CHECK(c) do { if (!(c)) { printf("FAIL %s:%d: %s\n", __FILE__, __LINE__, #c); fails++; } } while (0)

static size_t buildEvent(Packet &p, uint32_t src) {
  EventPayload e = {7, -300};
  return codecBuild(p, MSG_EVENT, src, &e, sizeof(e));
}

int main() {
  codecInit(65534);
  Packet p, in;
  const uint8_t *raw = reinterpret_cast<const uint8_t *>(&p);

  // build fills the header and payload; seq counts up and wraps
  size_t n = buildEvent(p, 0xA1B2C3D4);
  CHECK(n == sizeof(Header) + sizeof(EventPayload));
  CHECK(p.hdr.version == PROTO_VERSION && p.hdr.type == MSG_EVENT && p.hdr.src_id == 0xA1B2C3D4);
  CHECK(p.hdr.seq == 65534 && p.hdr.ttl == 1 && p.hdr.payload_len == sizeof(EventPayload));
  CHECK(raw[0] == PROTO_VERSION && raw[2] == 0xD4 && raw[5] == 0xA1);  // little-endian on the wire
  Packet q;
  buildEvent(q, 1);
  CHECK(q.hdr.seq == 65535);
  buildEvent(q, 1);
  CHECK(q.hdr.seq == 0);
  uint8_t big[PAYLOAD_MAX + 1] = {0};
  CHECK(codecBuild(q, MSG_CMD, 1, big, PAYLOAD_MAX) == ESPNOW_MAX_FRAME);
  CHECK(codecBuild(q, MSG_CMD, 1, big, PAYLOAD_MAX + 1) == 0);

  // good packet round-trips
  CHECK(codecParse(raw, n, in) == PARSE_OK);
  EventPayload e;
  memcpy(&e, in.payload, sizeof(e));
  CHECK(in.hdr.src_id == 0xA1B2C3D4 && e.code == 7 && e.arg == -300);

  // duplicate (same src, seq) dropped; same seq from another src accepted
  CHECK(codecParse(raw, n, in) == PARSE_DUPLICATE);
  p.hdr.src_id = 0x11111111;
  CHECK(codecParse(raw, n, in) == PARSE_OK);

  // truncated: shorter than header, and payload cut off
  n = buildEvent(p, 42);
  CHECK(codecParse(raw, sizeof(Header) - 1, in) == PARSE_SHORT);
  CHECK(codecParse(raw, n - 1, in) == PARSE_LENGTH);
  CHECK(codecParse(raw, n, in) == PARSE_OK);  // rejected frames were not recorded as seen

  // wrong version, unknown type, wrong size for type
  n = buildEvent(p, 43);
  p.hdr.version = PROTO_VERSION + 1;
  CHECK(codecParse(raw, n, in) == PARSE_VERSION);
  p.hdr.version = PROTO_VERSION;
  p.hdr.type = 0;
  CHECK(codecParse(raw, n, in) == PARSE_TYPE);
  p.hdr.type = 6;
  CHECK(codecParse(raw, n, in) == PARSE_TYPE);
  p.hdr.type = MSG_BEAT;  // EVENT-sized payload under a BEAT header
  CHECK(codecParse(raw, n, in) == PARSE_LENGTH);
  n = codecBuild(p, MSG_CMD, 43, nullptr, 0);
  CHECK(codecParse(raw, n, in) == PARSE_LENGTH);  // CMD needs at least the code byte

  // table forgets the oldest pair after DEDUPE_SLOTS newer ones
  codecInit(0);
  n = buildEvent(p, 99);
  CHECK(codecParse(raw, n, in) == PARSE_OK);
  Packet o;
  const uint8_t *oraw = reinterpret_cast<const uint8_t *>(&o);
  for (int i = 0; i < DEDUPE_SLOTS; i++) {
    size_t m = buildEvent(o, 100 + i);
    CHECK(codecParse(oraw, m, in) == PARSE_OK);
  }
  CHECK(codecParse(raw, n, in) == PARSE_OK);

  printf(fails ? "%d check(s) failed\n" : "all codec checks passed\n", fails);
  return fails != 0;
}
