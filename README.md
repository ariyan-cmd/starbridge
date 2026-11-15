# Star-Bridge

A network system that lets small robots find each other, share events, and show it on their faces.
First node type: DestroBot (NodeMCU, 0.96" 128x64 blue OLED, accelerometer).

Status: **v0.1 in progress.** See `docs/v0.1-plan.md`. Decisions, deviations and measured numbers live in `task_report.md`. Working rules for Claude are in `CLAUDE.md`.

## Layout

```
Star_bridge/
  README.md
  .gitignore
  docs/                      design notes (plan, wire protocol, hardware)
  firmware/
    star_bridge/             main sketch, open this folder in Arduino IDE
      star_bridge.ino        thin entry point, wires modules together
      src/                   one folder per module (Arduino compiles src/ recursively)
        config/              pins, timings, build-time settings
        hal/                 display and IMU drivers behind small interfaces
        identity/            node ID, name, capability manifest
        gestures/            accelerometer -> gesture events
        face/                eye renderer, face states, status screen
        net/                 packet envelope + ESP-NOW transport
        peers/               peer table, HELLO/BEAT, timeouts
        events/              event bus: local gestures out, remote events in
        splash/              boot animation, boot init runs behind it
        console/             serial commands
    bringup/                 throwaway hardware tests, not part of the main build
      oled_test/             display bring-up (already verified on 128x64)
  test/                      host-side g++ tests, kept out of the sketch so Arduino skips them
```

## Rules we follow

- The sketch folder name must equal the `.ino` name (Arduino requirement), so the main sketch is `firmware/star_bridge/star_bridge.ino`.
- Includes from the `.ino` use `#include "src/<module>/<module>.h"`. Inside `src/`, use relative paths such as `"../config/config.h"`.
- Modules talk through their headers only. No module reads another module's globals.
- Hardware access lives in `hal/`. Everything above it is testable without a board.
- Every wire-format change bumps `PROTO_VERSION` and updates `docs/protocol.md`.
