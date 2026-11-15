#!/usr/bin/env bash
# Creates the 16 v0.1 issues for Star-Bridge and splits them 8/8 between two people.
# Work split rationale: docs/work-split.md
#
# Usage (needs the GitHub CLI, logged in as someone who can write issues in the repo):
#   SAAD_LOGIN=<saad-github-username> bash tools/create_issues.sh
#   DRY_RUN=1 SAAD_LOGIN=x bash tools/create_issues.sh      # prints the plan, creates nothing
#
# Run it ONCE on a repo with no issues: bodies refer to each other by number (#1..#16).

set -euo pipefail

REPO="${REPO:-ariyan-cmd/starbridge}"
ARIYAN="${ARIYAN_LOGIN:-ariyan-cmd}"
SAAD="${SAAD_LOGIN:-}"
[ -n "$SAAD" ] || { echo "Set SAAD_LOGIN to Saad's GitHub username."; exit 1; }

FOOT='Working rules: small commits with brief messages. Record decisions, numbers and deviations in `task_report.md` in the same commit as the change. No Co-Authored-By or Claude-Session lines (see `CLAUDE.md`). Modules talk only through their headers.'

if [ -z "${DRY_RUN:-}" ]; then
  existing=$(gh issue list --repo "$REPO" --state all --limit 1 --json number --jq 'length')
  [ "$existing" = "0" ] || { echo "Repo already has issues. Issue numbers in the bodies would be wrong. Stop."; exit 1; }

  mklabel() { gh label create "$1" --color "$2" --description "$3" --force --repo "$REPO" >/dev/null; }
  mklabel v0.1 0e8a16 "Part of the v0.1 plan"
  mklabel device 1d76db "Device side: display, face, sensors, gestures"
  mklabel network c5def5 "Network side: protocol, radio, peers, events"
  mklabel integration d93f0b "Needs both sides working"
  mklabel tuning fbca04 "Measure, tune, record numbers"
  for m in config hal face gestures identity net peers events console; do
    mklabel "module:$m" ededed "src/$m"
  done
fi

n=0
mk() {  # title, labels, assignee ; body on stdin
  n=$((n + 1))
  local body; body="$(cat)"
  if [ -n "${DRY_RUN:-}" ]; then
    printf '#%-2s %-9s %-34s %s\n' "$n" "$3" "$2" "$1"
    return
  fi
  printf '%s\n\n---\n%s\n' "$body" "$FOOT" |
    gh issue create --repo "$REPO" --title "$1" --label "$2" --assignee "$3" --body-file -
}

# ---------------------------------------------------------------- #1
mk "config: pins, build settings and local override" "v0.1,device,module:config" "$ARIYAN" <<'EOF'
Goal: one place for build-time settings. No logic.

Tasks
- [ ] `src/config/pins.h`: I2C SDA=4 (D2), SCL=5 (D1)
- [ ] `src/config/config.h`: `SCREEN_WIDTH` 128, `SCREEN_HEIGHT` 64, `I2C_CLOCK_HZ`, `PROTO_VERSION` 1, beat interval (3000 ms), missed-beat limit (3), placeholder gesture thresholds
- [ ] `src/config/local_config.h.example` for per-node overrides (the real `local_config.h` is git-ignored)

Acceptance
- Sketch compiles with only config included.
- Every constant has a one-line comment saying who uses it.

Depends on: nothing. Small, do it first; #2 and #3 use it.
EOF

# ---------------------------------------------------------------- #2
mk "net: protocol.h (envelope, message types, capability bits)" "v0.1,network,module:net" "$SAAD" <<'EOF'
Goal: the wire format as C structs, matching `docs/protocol.md`.

Tasks
- [ ] Packed header struct: version u8, type u8, src_id u32, seq u16, ttl u8, payload_len u8 (10 bytes)
- [ ] Enums: HELLO, BEAT, EVENT, TELEMETRY, CMD
- [ ] Capability bits: DISPLAY, IMU, TEMP, LIGHT
- [ ] Payload structs for each type (start from the drafts in `docs/protocol.md`)
- [ ] `static_assert` on sizes so a change cannot slip through

Acceptance
- `sizeof(header) == 10`, max payload 240.
- Any change to the draft payloads is written back to `docs/protocol.md` and logged in `task_report.md`.

Depends on: #1 (PROTO_VERSION). Can stub it until #1 lands.
EOF

# ---------------------------------------------------------------- #3
mk "hal: display wrapper (init, flush, contrast, dim)" "v0.1,device,module:hal" "$ARIYAN" <<'EOF'
Goal: the only code that touches the SSD1306. Everything above it uses this interface.

Tasks
- [ ] `src/hal/display.h/.cpp`: `begin()`, `clear()`, `flush()`, `setContrast()`, `dim()`
- [ ] Adafruit SSD1306 at 128x64, address 0x3C, clock from `I2C_CLOCK_HZ` passed to the constructor (see the fps finding in `task_report.md`)
- [ ] Expose the drawing surface the face module needs (rounded rects, triangles, pixels)
- [ ] Pixel-shift helper for burn-in (shift the whole image by 1 px every few minutes)

Acceptance
- Init failure is reported, not a silent hang.
- Replaces the display code in `firmware/bringup/oled_test` without changing what it shows.

Depends on: #1.
EOF

# ---------------------------------------------------------------- #4
mk "face: parametric eye renderer (rounded rects, lid cuts, gaze)" "v0.1,device,module:face" "$ARIYAN" <<'EOF'
Goal: draw the eyes from a handful of parameters, no bitmaps.

Tasks
- [ ] Eye parameters: width, height, radius, top lid, lid slope, bottom lid, gaze x/y
- [ ] Render two eyes with lids cut by triangles or clipped rects
- [ ] Fully closed lid draws a thin line, not nothing
- [ ] Interpolation helper so states can ease between parameter sets
- [ ] Half-tone (checkerboard) fill option for the "weak signal" look

Acceptance
- Matches the eye preview from the design discussion at 128x64.
- Record frame time per redraw in `task_report.md` (budget: the loop must still fit gestures and networking inside 33 ms, see the 38.08 fps entry).

Depends on: #3.
EOF

# ---------------------------------------------------------------- #5
mk "net: codec (build, parse, validate, dedupe) with host-side test" "v0.1,network,module:net" "$SAAD" <<'EOF'
Goal: turn structs into packets and back, safely.

Tasks
- [ ] `build()` fills the header (seq counter, ttl=1) and copies the payload
- [ ] `parse()` rejects wrong version, short frames, `payload_len` larger than the frame, and unknown types
- [ ] Duplicate filter by (src_id, seq), small fixed-size table, no heap allocation
- [ ] A host-side test (compile with g++ on a PC) covering good packets, truncated packets, wrong version and duplicates

Acceptance
- Test passes on a PC and the module compiles for ESP8266.
- Number of RAM bytes used by the dedupe table recorded in `task_report.md`.

Depends on: #2.
EOF

# ---------------------------------------------------------------- #6
mk "net: ESP-NOW transport for ESP8266 (broadcast send, queued receive)" "v0.1,network,module:net" "$SAAD" <<'EOF'
Goal: send and receive raw frames over ESP-NOW on the ESP8266 core.

Tasks
- [ ] Init in station mode, add the broadcast peer, handle the ESP8266 ESP-NOW roles and callback signatures (they differ from ESP32)
- [ ] `send(frame, len)` broadcast, reports success or failure from the send callback
- [ ] Receive callback copies the frame into a fixed ring buffer and returns at once. No parsing or printing inside the callback.
- [ ] `poll()` called from `loop()` hands queued frames to the codec
- [ ] Capture RSSI per received frame if the core exposes it

Acceptance
- Two NodeMCUs exchange a test frame both ways.
- Queue depth and what happens on overflow recorded in `task_report.md`.

Depends on: #2.
EOF

# ---------------------------------------------------------------- #7
mk "hal: MPU6050 driver (accelerometer read at 0x68)" "v0.1,device,module:hal" "$ARIYAN" <<'EOF'
Goal: raw acceleration in g, behind a small interface.

Tasks
- [ ] `src/hal/imu.h/.cpp`: `begin()`, `read(ax, ay, az)`, optional gyro read
- [ ] Wake the chip from sleep, set range (start with +-4 g) and sample rate
- [ ] Confirm the address with an I2C scan. If AD0 is high it is 0x69. Update `docs/hardware.md`.
- [ ] Read WHO_AM_I and fail loudly if it does not match

Acceptance
- Serial print of x, y, z shows about 1 g on the vertical axis at rest and the signs flip when the board is turned over.
- Resting noise (min/max per axis over 10 s) recorded in `task_report.md`.

Depends on: #1.
EOF

# ---------------------------------------------------------------- #8
mk "gestures: shake, tap, tilt, pickup, face-down, freefall" "v0.1,device,module:gestures" "$ARIYAN" <<'EOF'
Goal: turn acceleration samples into discrete events.

Tasks
- [ ] Non-blocking `update()` called from `loop()`
- [ ] Detectors: SHAKE, TAP, TILT (with direction), PICKUP, FACE_DOWN, FREEFALL
- [ ] All thresholds and debounce times in `config.h`
- [ ] Emit events through a callback or the events queue (agree the type with #12)

Acceptance
- Each gesture is triggered reliably by hand and does not fire at rest.
- Final threshold values and the false-trigger rate you observed are recorded in `task_report.md`.

Depends on: #7. Output type to agree with #12.
EOF

# ---------------------------------------------------------------- #9
mk "identity: node ID, persisted name, capability manifest" "v0.1,network,module:identity" "$SAAD" <<'EOF'
Goal: who this node is and what it can do.

Tasks
- [ ] Node ID from the ESP8266 chip ID (32-bit, matches `src_id`)
- [ ] Name: default from config, changeable at runtime, saved to flash (EEPROM or LittleFS) and restored on boot
- [ ] I2C scan on boot: display at 0x3C and MPU6050 at 0x68 set capability bits
- [ ] Config line for sensors that cannot be detected (temp, light)
- [ ] `caps()` returns the bitmask used in HELLO

Acceptance
- Name survives a reboot.
- Scan result and boot time recorded in `task_report.md`.

Depends on: #1, #2.
EOF

# ---------------------------------------------------------------- #10
mk "peers: HELLO, BEAT, peer table and timeout" "v0.1,network,module:peers" "$SAAD" <<'EOF'
Goal: know who else is out there.

Tasks
- [ ] Broadcast HELLO on boot (name, capabilities, firmware version) and reply to HELLO from new nodes
- [ ] BEAT every `BEAT_INTERVAL_MS`
- [ ] Peer table: id, name, caps, last seen, RSSI. Fixed size, say 8 entries, define behaviour when full.
- [ ] Drop a peer after N missed beats and notify listeners (face reacts in #11)
- [ ] Join and leave callbacks

Acceptance
- Unplug one node and the other drops it within the expected time. Plug it back and it returns.
- Measured time to detect a lost peer and the beat interval you settled on recorded in `task_report.md`.

Depends on: #5, #6, #9.
EOF

# ---------------------------------------------------------------- #11
mk "face: state machine, blink scheduler and synchronized blink" "v0.1,device,module:face" "$ARIYAN" <<'EOF'
Goal: the robot's personality, driven by states.

Tasks
- [ ] States as parameter sets: idle, searching, handshake, linked, startled, dizzy, weak, lost, sleep
- [ ] Gaze follows tilt; saccade-style gaze jumps in idle; irregular blinks of about 150 ms
- [ ] Synchronized blink: when linked, blink on a shared trigger from the network (hook only, wired in #16)
- [ ] Sleep when face-down, dizzy on shake, startled on pickup or freefall
- [ ] `setState()` and `onGesture()` entry points for the events module

Acceptance
- Every state is reachable and looks as designed.
- Gesture to expression latency recorded in `task_report.md`.

Depends on: #4, #8.
EOF

# ---------------------------------------------------------------- #12
mk "events: bus for local gestures and remote events, plus telemetry" "v0.1,network,module:events" "$SAAD" <<'EOF'
Goal: the small bus connecting gestures, face and network.

Tasks
- [ ] Fixed-size event queue, no heap allocation
- [ ] Local gesture becomes an EVENT packet broadcast (agree the event codes with #8)
- [ ] Incoming EVENT and peer join/leave become callbacks the face can use
- [ ] TELEMETRY: publish temperature periodically if a sensor exists; store the latest peer readings
- [ ] Ignore events from self

Acceptance
- A host-side or on-board test shows an event going out on one node and arriving on another.
- Queue size and RAM used recorded in `task_report.md`.

Depends on: #5, #10. Event codes to agree with #8.
EOF

# ---------------------------------------------------------------- #13
mk "face: status screen (name, peer count, RSSI) with gesture toggle" "v0.1,device,module:face" "$ARIYAN" <<'EOF'
Goal: a second screen with the facts, reachable by gesture.

Tasks
- [ ] Layout for 128x64: node name, peer count, strongest RSSI, uptime
- [ ] Toggle between face and status with a gesture (pick one in #8, for example double tap)
- [ ] Return to the face after a timeout
- [ ] Peer list page if space allows

Acceptance
- Readable at arm's length on the blue OLED.
- Values update live as peers join and drop.

Depends on: #11, #10.
EOF

# ---------------------------------------------------------------- #14
mk "console: serial commands (peers, caps, set name, send, help)" "v0.1,network,module:console" "$SAAD" <<'EOF'
Goal: debug a distributed system from the Serial Monitor.

Tasks
- [ ] Non-blocking line reader called from `loop()`
- [ ] Commands: `help`, `peers`, `caps`, `set name <text>`, `send <event>`
- [ ] Optional: `stats` (packets sent and received, drops, free heap)
- [ ] Only uses other modules through their headers

Acceptance
- Every command works with two nodes on the bench.
- Free heap with all modules running recorded in `task_report.md`.

Depends on: #9, #10, #12.
EOF

# ---------------------------------------------------------------- #15
mk "tuning: fps at 800 kHz and 1 MHz, gesture and animation numbers" "v0.1,device,tuning" "$ARIYAN" <<'EOF'
Goal: replace guesses with measured numbers.

Tasks
- [ ] Re-run the bring-up speed test at `I2C_CLOCK_HZ` = 800000UL and 1000000UL, record fps and whether the screen garbles
- [ ] Pick the I2C clock for the main sketch and write down why
- [ ] Measure the main loop with the face, gestures and radio running together; find out where the 33 ms frame goes
- [ ] Tune blink timing and gaze speed on the real screen

Acceptance
- Every number is in `task_report.md` with date and setup.
- If a number changes a plan or a decision, add a deviation or a new decision entry.

Depends on: #4, #8. Re-run at the end once #16 is done.
EOF

# ---------------------------------------------------------------- #16
mk "integration: two-node link test and the shake demo" "v0.1,network,integration" "$SAAD" <<'EOF'
Goal: the v0.1 demo. Shake one robot and watch the other's face react.

Tasks
- [ ] Flash two nodes with the full sketch; confirm HELLO, peer table and beats
- [ ] Measure round-trip latency, packet loss and RSSI at 0.5 m, 2 m and 5 m
- [ ] Gesture on A produces an EVENT that B's face reacts to (needs #8, #11, #12)
- [ ] Wire the synchronized blink hook from #11 to a network trigger
- [ ] Test unplug and rejoin; test three nodes if a third board is available

Acceptance
- The demo works three times in a row from a cold boot.
- All numbers and any deviations from `docs/v0.1-plan.md` recorded in `task_report.md`. Update `docs/protocol.md` if the wire format changed.

Depends on: #6, #8, #10, #11, #12. Pair with the owner of #11 and #8 for the face side.
EOF

echo "Done: $n issues."
