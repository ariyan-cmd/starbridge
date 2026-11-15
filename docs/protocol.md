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

| Type | Value | Sent | Payload (proposed) |
|------|-------|------|--------------------|
| HELLO | 1 | once on boot, again on request | name (up to 16 chars), capability bitmask u16, firmware version u8 |
| BEAT | 2 | every few seconds | uptime s u32, battery or status byte |
| EVENT | 3 | on a gesture | event code u8, argument i16 |
| TELEMETRY | 4 | periodic | sensor id u8, value i16 (scaled x100) |
| CMD | 5 | on demand | command code u8, argument bytes |

## Capability bits (proposed)

`DISPLAY=1<<0, IMU=1<<1, TEMP=1<<2, LIGHT=1<<3`

## Open questions

- Peer timeout: N missed beats, and the beat interval. Starting guess: 3 s beat, drop after 3 misses.
- Whether EVENT needs acknowledgement. Starting guess: no, events are fire-and-forget.
- Encryption: ESP-NOW supports it, off for v0.1.
