// Star-Bridge wire format. Must match docs/protocol.md.
// Any change here bumps PROTO_VERSION and updates docs/protocol.md.
#pragma once

#include <stdint.h>

#if __has_include("../config/config.h")
#include "../config/config.h"
#endif
#ifndef PROTO_VERSION
// shortcut: stub until #1 lands config.h, then this fallback is dead code and can go.
#define PROTO_VERSION 1
#endif

static_assert(__BYTE_ORDER__ == __ORDER_LITTLE_ENDIAN__, "wire format is little-endian");

constexpr uint8_t ESPNOW_MAX_FRAME = 250;

// Message types (Header::type).
enum MsgType : uint8_t {
  MSG_HELLO = 1,
  MSG_BEAT = 2,
  MSG_EVENT = 3,
  MSG_TELEMETRY = 4,
  MSG_CMD = 5,
};

// Capability bits (HelloPayload::caps).
enum CapBits : uint16_t {
  CAP_DISPLAY = 1 << 0,
  CAP_IMU = 1 << 1,
  CAP_TEMP = 1 << 2,
  CAP_LIGHT = 1 << 3,
};

struct __attribute__((packed)) Header {
  uint8_t version;      // PROTO_VERSION; receivers drop other versions
  uint8_t type;         // MsgType
  uint32_t src_id;      // chip ID of the original sender
  uint16_t seq;         // per-sender counter, for duplicate drop
  uint8_t ttl;          // 1 in v0.1 (no relay)
  uint8_t payload_len;  // 0..PAYLOAD_MAX
};

constexpr uint8_t PAYLOAD_MAX = ESPNOW_MAX_FRAME - sizeof(Header);
constexpr uint8_t NODE_NAME_LEN = 16;

struct __attribute__((packed)) HelloPayload {
  char name[NODE_NAME_LEN];  // zero-padded, not NUL-terminated when 16 chars long
  uint16_t caps;             // CapBits
  uint8_t fw_version;
};

struct __attribute__((packed)) BeatPayload {
  uint32_t uptime_s;
  uint8_t status;  // battery or status byte
};

struct __attribute__((packed)) EventPayload {
  uint8_t code;
  int16_t arg;
};

struct __attribute__((packed)) TelemetryPayload {
  uint8_t sensor_id;
  int16_t value_x100;  // value scaled by 100
};

// Variable length: payload_len = 1 + number of argument bytes.
struct __attribute__((packed)) CmdPayload {
  uint8_t code;
  uint8_t args[PAYLOAD_MAX - 1];
};

struct __attribute__((packed)) Packet {
  Header hdr;
  uint8_t payload[PAYLOAD_MAX];
};

static_assert(sizeof(Header) == 10, "header must be 10 bytes");
static_assert(PAYLOAD_MAX == 240, "max payload must be 240 bytes");
static_assert(sizeof(Packet) == ESPNOW_MAX_FRAME, "packet must fill one ESP-NOW frame");
static_assert(sizeof(HelloPayload) == 19, "HELLO payload size changed: update docs/protocol.md");
static_assert(sizeof(BeatPayload) == 5, "BEAT payload size changed: update docs/protocol.md");
static_assert(sizeof(EventPayload) == 3, "EVENT payload size changed: update docs/protocol.md");
static_assert(sizeof(TelemetryPayload) == 3, "TELEMETRY payload size changed: update docs/protocol.md");
static_assert(sizeof(CmdPayload) == PAYLOAD_MAX, "CMD payload must be at most PAYLOAD_MAX");
