# net

Packets and the radio.

- `protocol.h`: envelope struct, message types, capability bits. Matches `docs/protocol.md`.
- `codec.h`: `codecInit`, `codecBuild`, `codecParse`. Validates version, type and payload length, drops duplicates by (src_id, seq) in a 16-slot ring (96 B, no heap). Host test: `test/codec_test.cpp`.
- `transport`: ESP-NOW wrapper. Send broadcast, receive callback that queues packets for the main loop (never do work inside the callback).

Depends on: config, identity.
