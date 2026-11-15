# Star-Bridge wire protocol (draft, PROTO_VERSION = 1)

Transport: ESP-NOW broadcast. Max frame 250 bytes.

## Envelope

| Field | Size | Notes |
|-------|------|-------|
| version | u8 | PROTO_VERSION, receivers drop other versions |
| type | u8 | see below |
| src_id | u32 | chip ID of the original sender |
| seq | u16 | per-sender counter, used to drop duplicates |
| ttl | u8 | 1 in v0.1 (no relay). Decrement on relay later |
| payload_len | u8 | 0..240 |
| payload | n | type-specific |

Header is 10 bytes, so payload is at most 240 bytes. Multi-byte fields are little-endian (native on ESP). Use packed structs.

## Message types

| Type | Value | Sent | Payload |
|------|-------|------|---------|
| HELLO | 1 | once on boot, again on request | 19 bytes, see below |
| BEAT | 2 | every few seconds | 5 bytes |
| EVENT | 3 | on a gesture | 3 bytes |
| TELEMETRY | 4 | periodic | 3 bytes |
| CMD | 5 | on demand | 1 + n bytes, n = 0..239 |

## Payloads

C structs in `firmware/star_bridge/src/net/protocol.h`, packed, with `static_assert` on every size.

| Type | Field | Size | Notes |
|------|-------|------|-------|
| HELLO | name | 16 x char | zero-padded; not NUL-terminated when all 16 chars are used |
| HELLO | caps | u16 | capability bitmask |
| HELLO | fw_version | u8 | |
| BEAT | uptime_s | u32 | seconds since boot |
| BEAT | status | u8 | battery or status byte |
| EVENT | code | u8 | event code |
| EVENT | arg | i16 | |
| TELEMETRY | sensor_id | u8 | |
| TELEMETRY | value_x100 | i16 | value scaled x100 |
| CMD | code | u8 | command code |
| CMD | args | n x u8 | n = payload_len - 1 |

## Receiver rules

Implemented in `net/codec`. A frame is dropped if it is shorter than the header, `version` is not PROTO_VERSION, `type` is unknown, `payload_len` is larger than the bytes received, or `payload_len` is not the exact size for the type (CMD: 1..240). Bytes after the payload are ignored. Accepted frames are remembered by (src_id, seq) in a 16-entry ring shared by all senders; a repeat in that window is dropped. Senders start `seq` at a random value on boot so a quick reboot is not taken for a repeat.

## Capability bits

`DISPLAY=1<<0, IMU=1<<1, TEMP=1<<2, LIGHT=1<<3`

## Open questions

- Peer timeout: N missed beats, and the beat interval. Starting guess: 3 s beat, drop after 3 misses.
- Whether EVENT needs acknowledgement. Starting guess: no, events are fire-and-forget.
- Encryption: ESP-NOW supports it, off for v0.1.
