// Packet build, parse, validate and duplicate drop. No heap, no Arduino calls (host-testable).
#pragma once

#include <stddef.h>
#include "protocol.h"

// firstSeq: start of this node's seq counter. Pass a random value on boot so peers that
// still remember (src_id, seq) from our previous boot do not drop our first packets.
void codecInit(uint16_t firstSeq);

// Fills out.hdr (version, type, src_id, next seq, ttl = 1, len) and copies len payload bytes.
// Returns the frame length to send, or 0 if len > PAYLOAD_MAX.
size_t codecBuild(Packet &out, uint8_t type, uint32_t src_id, const void *payload, uint8_t len);

enum ParseResult : uint8_t {
  PARSE_OK,
  PARSE_SHORT,      // frame shorter than the header
  PARSE_VERSION,    // version != PROTO_VERSION
  PARSE_LENGTH,     // payload_len larger than the frame, or wrong size for the type
  PARSE_TYPE,       // unknown MsgType
  PARSE_DUPLICATE,  // (src_id, seq) seen recently
};

// Validates frame and copies it into out. Only PARSE_OK packets are recorded for duplicate drop.
ParseResult codecParse(const uint8_t *frame, size_t len, Packet &out);

constexpr uint8_t DEDUPE_SLOTS = 16;  // recent (src_id, seq) pairs remembered, shared by all peers
