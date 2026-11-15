#include "codec.h"

#include <string.h>

static uint16_t nextSeq;

// Ring of recently accepted (src_id, seq). Split arrays avoid struct padding: 16 x 6 = 96 bytes.
static uint32_t seenSrc[DEDUPE_SLOTS];
static uint16_t seenSeq[DEDUPE_SLOTS];
static uint8_t seenCount, seenNext;

void codecInit(uint16_t firstSeq) {
  nextSeq = firstSeq;
  seenCount = seenNext = 0;
}

size_t codecBuild(Packet &out, uint8_t type, uint32_t src_id, const void *payload, uint8_t len) {
  if (len > PAYLOAD_MAX) return 0;
  out.hdr.version = PROTO_VERSION;
  out.hdr.type = type;
  out.hdr.src_id = src_id;
  out.hdr.seq = nextSeq++;
  out.hdr.ttl = 1;
  out.hdr.payload_len = len;
  if (len) memcpy(out.payload, payload, len);
  return sizeof(Header) + len;
}

// Exact payload size per type; CMD is 1..PAYLOAD_MAX. Unknown type is false.
static bool sizeOk(uint8_t type, uint8_t len) {
  switch (type) {
    case MSG_HELLO: return len == sizeof(HelloPayload);
    case MSG_BEAT: return len == sizeof(BeatPayload);
    case MSG_EVENT: return len == sizeof(EventPayload);
    case MSG_TELEMETRY: return len == sizeof(TelemetryPayload);
    case MSG_CMD: return len >= 1;
  }
  return false;
}

static bool seen(uint32_t src, uint16_t seq) {
  for (uint8_t i = 0; i < seenCount; i++)
    if (seenSrc[i] == src && seenSeq[i] == seq) return true;
  return false;
}

ParseResult codecParse(const uint8_t *frame, size_t len, Packet &out) {
  if (len < sizeof(Header)) return PARSE_SHORT;
  Header h;
  memcpy(&h, frame, sizeof(Header));
  if (h.version != PROTO_VERSION) return PARSE_VERSION;
  if (h.type < MSG_HELLO || h.type > MSG_CMD) return PARSE_TYPE;
  // payload_len <= PAYLOAD_MAX follows from len <= ESPNOW_MAX_FRAME, but check both: len is the caller's.
  if (h.payload_len > PAYLOAD_MAX || h.payload_len > len - sizeof(Header) || !sizeOk(h.type, h.payload_len))
    return PARSE_LENGTH;
  if (seen(h.src_id, h.seq)) return PARSE_DUPLICATE;

  seenSrc[seenNext] = h.src_id;
  seenSeq[seenNext] = h.seq;
  seenNext = (seenNext + 1) % DEDUPE_SLOTS;
  if (seenCount < DEDUPE_SLOTS) seenCount++;

  out.hdr = h;
  memcpy(out.payload, frame + sizeof(Header), h.payload_len);
  return PARSE_OK;
}
