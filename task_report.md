# Task report

The one stop for why a decision was taken and what it cost. Newest entries go at the top of each section. Update it in the same commit as the change it records.

Entry rules:
- Decisions get an ID (D-001, ...) and are never deleted. If reversed, add a new decision that supersedes the old one.
- Every number has a date and a setup. Unmeasured items say `pending`.
- Cost means what we give up or pay: RAM, flash, time, features, flexibility.

## Decisions

| ID | Date | Decision | Why | Alternatives | Cost |
|----|------|----------|-----|--------------|------|
| D-013 | 2026-11-15 | Issue #4: face renderer draws per column, not per pixel: for each x it finds the lit row span (circle cut by the lid lines) and calls `drawFastVLine`; the ring is outer span minus the inset span; the star pupil is clipped to the inset span. Bridge arch is 13 px wide, 4 px high, 2 px thick, baseline y = 44 | Avoids ~2600 per-pixel float tests on an 80 MHz ESP8266 with no FPU. Arch is shrunk from concept D because C's eye gap is only 14 px | Per-pixel inside test as in the HANDOFF text (simpler, slower), integer-only math | Floats are still used per column (sqrt, ceil) and in `powf` once at init. Not measured yet, so render time is pending. Not done in #4 (deferred to #11): row-tear glitch for `lost`, comet pixel for `data`, random saccades and blink schedule. Orbit dots drawn at radius 15.8 px on the ring's inner edge, one dot pair per peer per eye, phase offset 1.57 rad per peer (my reading of the spec) |
| D-012 | 2026-11-15 | Issue #3: `hal/display` is a flat C-style API (`displayInit/Clear/Flush/SetContrast/Dim`) plus `displayGfx()` returning `Adafruit_GFX&`; the SSD1306 object is private to `display.cpp` | Face can use GFX drawing and never sees the SSD1306 type, so a panel swap touches only `hal/` | Expose the `Adafruit_SSD1306` object, own drawing wrappers | `displayGfx()` leaks the GFX type (acceptable, GFX is the drawing standard). Per-pixel `drawPixel` goes through GFX bounds and rotation checks, so it is slower than writing the buffer directly. Face will measure it, see #4 |
| D-011 | 2026-11-15 | Issue #1 config: `local_config.h` is pulled in by `config.h` with `__has_include`, and every value in `config.h` is `#ifndef`-guarded so a node can override it | One header to include, per-node values stay out of git | Always-required `local_config.h` (breaks fresh clones), `-D` build flags (not possible from Arduino IDE) | Needs a GCC with `__has_include` (ESP8266 core 3.x has it, not checked here). Gesture thresholds (2.0 / 1.8 / 0.3 g) are placeholders, not measured, see Numbers |
| D-010 | 2026-11-15 | `origin/main` held the 7 pre-rewrite commits with Claude trailers, so main is replaced by the 12-commit bundle history (force push, lease pinned to c58fe76) | Keep D-008 true on GitHub | Merge both histories (keeps trailers public forever) | Rewrites public history. Old commits stay reachable by hash on GitHub for a while. Anyone who cloned before must re-clone |
| D-009 | 2026-11-15 | v0.1 split into 16 issues, 8 per person, along the device/network line (Ariyan: display, face, MPU6050, gestures. Saad: protocol, ESP-NOW, identity, peers, events, console, integration) | The wire format and module headers are a clean contract, so both sides can work in parallel | Split by module count only, or by strengths | The split is by interface, not by known strengths. Integration risk is concentrated in #16 and in two handshakes (event codes #8/#12, blink hook #11/#16). Issues were created on 2026-11-15 from Claude Code with `gh` (Saad = `saad-mh`, 16 issues, #1 to #16) and the one-shot script was then deleted |
| D-008 | 2026-11-15 | No `Co-Authored-By` or `Claude-Session` trailers on commits or PRs. Existing 7 local commits rewritten to remove them | Owner's choice for how history reads | Keep default attribution | Commit hashes changed once, before the first push (the bundle was regenerated). No machine-readable record of which commits Claude wrote. A `commit-msg` hook in `tools/hooks/` enforces it, but it needs `git config core.hooksPath tools/hooks` in each clone |
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
| 2026-11-15 | Build size of the sketch with config, hal/display and face preview | `arduino-cli compile`, board `esp8266:esp8266:nodemcuv2`, core 3.1.2, Adafruit SSD1306 2.5.17, GFX 1.13.0, arduino-cli 1.6.0 | RAM 30316 / 80192 bytes (37%) static, flash 259732 / 1048576 bytes (24%), IRAM 60943 / 65536 bytes (92%, includes the 32768 B reserved cache). Compiles with no errors. Runtime fps and heap: pending |
| 2026-11-15 | Display bring-up result | `firmware/bringup/oled_test`, 128x64, 400 kHz, SSD1306 at 0x3C | Visual pass, reported clearer than 128x32 |
| 2026-11-15 | Frames per second, full 128x64 redraw with a moving bar | Bring-up test as first written, clock labelled 400k / 800k / 1M | 38.08 fps, identical at all three labels (reported as 400k/600k/1M; the test actually used 800k) |
| 2026-11-15 | Debug: identical fps at every clock label | Same | Cause (likely): the Adafruit SSD1306 library resets the I2C clock to its constructor value before every transfer, so `Wire.setClock()` calls in the test had no effect and all three runs were at 400 kHz. Fix: clock now passed to the display constructor via `I2C_CLOCK_HZ`, one measurement per upload. Re-measure at 800k and 1M is pending |
| 2026-11-15 | Sanity check on 38.08 fps | 1024 data bytes x 9 bits / 400 kHz | About 23 ms of pure data time (a ceiling near 43 fps) against 26.3 ms measured. Consistent with an effective 400 kHz clock (my arithmetic, not a measurement) |
| 2026-11-15 | Frame budget at 38.08 fps | v0.1 face animation target of 30 fps | 38.08 fps clears 30 fps at 400 kHz. Cost: a full redraw uses about 26 ms per frame, leaving little of a 33 ms frame for gestures and networking in the same loop. Budget is tight, so avoid full-screen redraws every loop where possible |
| 2026-11-15 | Max payload | Header 10 bytes, ESP-NOW frame 250 bytes | 240 bytes |
| 2026-11-15 | Display framebuffer | 128 x 64 / 8 | 1024 bytes |
| 2026-11-15 | MPU6050 I2C address | AD0 low assumed | 0x68 (confirm the AD0 pin level on the module) |

## Open items

- Face renderer (#4) compiled but not run. Ariyan to upload and paste Serial: per-preset fps, render us, flush us, free heap. Also note what looks wrong on screen (ring thickness, star shape, lids, bridge arch).
- hal/display (#3) compiled but not run: pending Ariyan's upload.
- Gesture thresholds in `config.h` (shake 2.0 g, tap 1.8 g, freefall 0.3 g) are placeholders: pending measurement.
- Re-run the speed test at I2C_CLOCK_HZ = 800000UL and 1000000UL (one upload each) and record fps plus whether the screen garbles.
- Confirm the MPU6050 address by running the I2C scan with it attached.
- Are any DHT or LDR sensors fitted? Needed for the temperature telemetry in plan item 5.
- Peer timeout: beat interval and missed-beat limit are guesses (3 s, 3 misses) until tested.
