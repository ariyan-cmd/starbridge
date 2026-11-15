# net

Packets and the radio.

- `protocol.h`: envelope struct, message types, capability bits. Matches `docs/protocol.md`.
- `codec`: build and parse packets, validate version and payload length, drop duplicates by (src_id, seq).
- `transport`: ESP-NOW wrapper. Send broadcast, receive callback that queues packets for the main loop (never do work inside the callback).

Depends on: config, identity.
