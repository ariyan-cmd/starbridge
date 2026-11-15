# peers

Who else is out there.

- `peersBegin(self, send, onChange, now)` broadcasts our HELLO. `peersOnPacket(p, now)` takes every accepted packet; `peersUpdate(now)` sends BEAT every `BEAT_INTERVAL_MS` and drops timed-out peers.
- HELLO from anyone is answered with our HELLO (at most one per second, so two nodes do not ping-pong). A BEAT from an unknown node also gets our HELLO, which makes it answer with its own.
- Peer table: `PEER_TABLE_LEN` (8) entries of id, name, caps, fw version, last seen. A new peer is ignored while the table is full. No RSSI on ESP8266 (D-021).
- Timeout: dropped after `peerTimeoutMs()` = `MISSED_BEAT_LIMIT` beats plus half a beat with nothing heard. `onChange(peer, joined)` fires on join and on drop.
- No Arduino calls: the sketch passes `millis()` and a send function. Host test: `test/peers_test.cpp`.

Depends on: config, net (protocol only).
