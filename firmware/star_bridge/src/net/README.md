# net

Packets and the radio.

- `protocol.h`: envelope struct, message types, capability bits. Matches `docs/protocol.md`.
- `codec.h`: `codecInit`, `codecBuild`, `codecParse`. Validates version, type and payload length, drops duplicates by (src_id, seq) in a 16-slot ring (96 B, no heap). Host test: `test/codec_test.cpp`.
- `transport.h`: ESP-NOW (ESP8266 SDK) broadcast. `transportInit(handler)`, `transportSend(frame, len)`, `transportPoll()` from `loop()`, `transportStats()`. The receive callback only copies into `frame_queue.h` (8 x 251 B, drops the new frame when full); `transportPoll` parses with the codec and calls the handler. No RSSI on ESP8266 (D-021). Host test: `test/frame_queue_test.cpp`.

Depends on: config, identity.
