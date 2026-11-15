# Task report

The one stop for why a decision was taken and what it cost. Newest entries go at the top of each section. Update it in the same commit as the change it records.

Entry rules:
- Decisions get an ID (D-001, ...) and are never deleted. If reversed, add a new decision that supersedes the old one.
- Every number has a date and a setup. Unmeasured items say `pending`.
- Cost means what we give up or pay: RAM, flash, time, features, flexibility.

## Decisions

| ID | Date | Decision | Why | Alternatives | Cost |
|----|------|----------|-----|--------------|------|
| D-007 | 2026-11-15 | Public GitHub repo `starbridge` | Share and back up the work | Private repo | Everything committed is public. No secrets, Wi-Fi passwords or keys in the tree (`local_config.h` is git-ignored) |
| D-006 | 2026-11-15 | Face is eyes only, parametric (rounded rects plus lid cuts), no mouth, no bitmaps | Eyes carry most of the readable emotion, cheap to draw, easy to interpolate between states | Full face, bitmap expressions, outline-only eyes | No lip shapes. Expressions limited to what lids, gaze and size can say |
| D-005 | 2026-11-15 | Parts confirmed: ESP8266MOD NodeMCU, MPU6050 (0x68) | Hardware on the desk | ESP32 | ESP8266 has about 80 KB RAM and a different ESP-NOW API from ESP32. MPU6050 has no magnetometer, so no compass heading |
| D-004 | 2026-11-15 | Display set to 128x64 (was the 128x32 example) | Looks clearer on the real panel | Stay on 128x32 | Framebuffer is 1024 bytes instead of 512. A full frame sends twice the I2C data, so expect a lower max fps (not measured yet, see Numbers) |
| D-003 | 2026-11-15 | Packet header is 10 bytes: version u8, type u8, src_id u32, seq u16, ttl u8, payload_len u8 | Fits the planned envelope with a ttl for later relaying | 16-bit src_id, no seq | 10 bytes per packet, payload max 240 of 250. A 32-bit src_id is used so chip IDs fit without a lookup |
| D-002 | 2026-11-15 | One Arduino sketch with modules under `src/<module>/` | Arduino compiles `src/` recursively and needs no separate library install | Separate libraries, PlatformIO | Includes must be written `"src/<module>/x.h"` from the .ino and relative paths inside `src/`. No per-module build isolation |
| D-001 | 2026-11-15 | Transport is ESP-NOW broadcast | No router or IP stack, low latency | Wi-Fi with a router, painlessMesh | 250-byte frame limit and no multi-hop in v0.1. The ttl field is reserved so relaying can be added without a wire change |

## Deviations from plan

| Date | Plan said | What happened | Impact |
|------|-----------|---------------|--------|
| 2026-11-15 | ESP-NOW works on ESP8266 and ESP32, so either chip is fine | Chip is ESP8266MOD | Use the ESP8266 ESP-NOW API in `net/transport`. No ESP32-only calls |
| 2026-11-15 | Display example was 128x32 | Switched to 128x64 | Layout constants in `config` use 64. Bring-up test updated |
| 2026-11-15 | Plan lists six features only | Added a bring-up test folder and a protocol doc before module code | No cost to v0.1 scope. Gives a verified display before the face module starts |

## Numbers and debug log

| Date | What | Setup | Value |
|------|------|-------|-------|
| 2026-11-15 | Display bring-up result | `firmware/bringup/oled_test`, 128x64, 400 kHz, SSD1306 at 0x3C | Visual pass, reported clearer than 128x32 |
| 2026-11-15 | Frames per second at 400 kHz / 800 kHz / 1 MHz | Same test, Speed section | pending (read from the OLED or Serial Monitor and record here) |
| 2026-11-15 | Max payload | Header 10 bytes, ESP-NOW frame 250 bytes | 240 bytes |
| 2026-11-15 | Display framebuffer | 128 x 64 / 8 | 1024 bytes |
| 2026-11-15 | MPU6050 I2C address | AD0 low assumed | 0x68 (confirm the AD0 pin level on the module) |

## Open items

- Record the fps numbers from the bring-up test.
- Confirm the MPU6050 address by running the I2C scan with it attached.
- Are any DHT or LDR sensors fitted? Needed for the temperature telemetry in plan item 5.
- Peer timeout: beat interval and missed-beat limit are guesses (3 s, 3 misses) until tested.
