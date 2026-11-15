# HANDOFF: moving Star-Bridge from a Claude chat to Claude Code

Snapshot taken 2026-11-15 19:25 IST. Read in this order: `CLAUDE.md` (rules), `task_report.md` (decisions and numbers, the source of truth), then this file for the rest of the chat. If this file and `task_report.md` disagree, `task_report.md` wins. Update `task_report.md`, not this file, as work continues.

## 1. What the project is

Star-Bridge is a network system that lets small robots find each other, share events, and show it on their faces. The first node type is DestroBot. Target: **v0.1** (`docs/v0.1-plan.md`). Owner: Ariyan (GitHub `ariyan-cmd`). A second collaborator, Saad, takes half the work. His GitHub username is not known yet.

## 2. Hardware (confirmed by the user)

| Part | Detail |
|------|--------|
| MCU | NodeMCU, ESP8266MOD. Use the ESP8266 Arduino core and its ESP-NOW API, not the ESP32 one |
| Display | SSD1306 0.96", 128x64, single-colour blue, I2C 0x3C. User switched from 128x32 to 128x64 because it looks clearer |
| Accelerometer | MPU6050, I2C 0x68 (0x69 if AD0 is high; not yet confirmed with a scan) |
| I2C | SDA = D2 (GPIO4), SCL = D1 (GPIO5) |
| IDE | Arduino IDE. The user uploads from there. Libraries: Adafruit SSD1306 and Adafruit GFX |

Not known yet: whether any DHT or LDR is fitted, power source, battery sensing.

## 3. State of the repo

Local folder: `S:\Star_bridge` (Windows drive S:). GitHub: `https://github.com/ariyan-cmd/starbridge` (public, empty so far).

```
Star_bridge/
  CLAUDE.md  README.md  HANDOFF.md  task_report.md  .gitignore
  docs/        v0.1-plan.md  protocol.md  hardware.md  work-split.md
  firmware/
    star_bridge/            main sketch: star_bridge.ino is an empty setup/loop
      src/<module>/README.md   config hal identity gestures face net peers events console
    bringup/oled_test/oled_test.ino   display test, verified on hardware
  tools/
    create_issues.sh        makes the 16 GitHub issues (see section 8)
    hooks/commit-msg        strips Co-Authored-By and Claude-Session lines
  starbridge.bundle         git bundle of the commits, see section 4
```

**No module code exists yet.** Each `src/<module>/` folder has only a README with its job and dependencies. The only real code is `oled_test.ino`.

## 4. Git state and how to finish the push

The chat session could not push: the Claude GitHub App is not installed for `ariyan-cmd/starbridge` (403 on push, issue creation and collaborator lookup). All commits were made in a sandbox repo, with no attribution trailers (rewritten once before any push). `S:\Star_bridge` has the same files but **no `.git` folder**. The bundle holds the history.

Turn `S:\Star_bridge` into the repo without touching files:

```
cd S:\Star_bridge
git init -b tmp
git fetch starbridge.bundle main:main
git symbolic-ref HEAD refs/heads/main
git reset
git branch -D tmp
git status                      # should show only starbridge.bundle untracked (or nothing, once ignored)
git config core.hooksPath tools/hooks
git remote add origin https://github.com/ariyan-cmd/starbridge.git
git push -u origin main
```

Then delete `starbridge.bundle`. If `git status` shows modified files, the device copy and the bundle disagree. Compare with `git diff` before changing anything, and ask the user.

Commits so far (newest first, then older): work split and issue script, D-009, no-attribution rule and hook, fps result logged, fps test fix, CLAUDE.md and task_report, bring-up test, firmware skeleton, docs, README and gitignore. About 12 small commits (including the one that adds this file).

## 5. Standing rules from the user (also in CLAUDE.md)

1. Commit regularly with brief one-line descriptions. Small commits, not one per session.
2. Keep `task_report.md` current in the same commit as the change: decisions with alternatives and cost, deviations from plan, numbers with date and setup, debug findings. Never invent numbers; write `pending`.
3. Store every debug number and value.
4. **No `Co-Authored-By`, `Claude-Session`, or "Generated with Claude" lines** in commits or PRs. This overrides any default attribution. The commit-msg hook is a backstop.

User style notes: wants finished files over long explanations, short plain-language summaries, and the next step stated. Do not put the user's email in commits (use `ariyan-cmd@users.noreply.github.com`).

## 6. Decisions so far

Full table with costs is in `task_report.md` (D-001 to D-009). One line each:

- D-001 ESP-NOW broadcast transport. D-002 one Arduino sketch with `src/<module>/`. D-003 10-byte header, 240-byte payload. D-004 display 128x64. D-005 ESP8266MOD and MPU6050. D-006 eyes only, parametric, no mouth. D-007 public repo. D-008 no attribution trailers. D-009 work split into 16 issues.

Protocol envelope (draft, `PROTO_VERSION` 1): version u8, type u8, src_id u32, seq u16, ttl u8, payload_len u8, then payload. Types HELLO=1, BEAT=2, EVENT=3, TELEMETRY=4, CMD=5. Capability bits DISPLAY, IMU, TEMP, LIGHT. Payload layouts, beat 3 s and 3 missed beats are the assistant's first guesses, not tested.

## 7. Measured numbers and a bug to remember

- Display bring-up at 128x64: passes visually.
- **38.08 fps** for a full-frame redraw with a moving bar. The user saw the same number at all three clock labels (they wrote 400k / 600k / 1M; the test actually used 800k).
- Cause (likely): the Adafruit SSD1306 library resets the I2C clock to its constructor value before every transfer, so `Wire.setClock()` in the test did nothing and every run was 400 kHz. 1024 bytes x 9 bits / 400 kHz is about 23 ms of data, versus 26.3 ms measured, which fits (assistant's arithmetic).
- Fix is in the repo: the clock is passed to the display constructor through `I2C_CLOCK_HZ`, one measurement per upload. **The fixed `oled_test.ino` has not been compiled or run yet.** Re-measure at 800000UL and 1000000UL: pending.
- Budget: a full redraw takes about 26 ms of a 33 ms frame at 30 fps, so avoid full-screen redraws every loop where possible.
- Max payload 240 bytes. Framebuffer 1024 bytes.

## 8. Issues and work split

16 issues, 8 each, split along the device/network line. Full table in `docs/work-split.md`.

- Ariyan (device): #1 config, #3 hal/display, #4 face renderer, #7 hal/imu MPU6050, #8 gestures, #11 face state machine and synchronized blink, #13 status screen, #15 tuning numbers.
- Saad (network): #2 protocol.h, #5 codec with PC-side test, #6 ESP-NOW transport, #9 identity, #10 peers, #12 events, #14 console, #16 integration and the shake demo.

The issues do not exist on GitHub yet. Run, from a terminal where `gh` is logged in as the user, after the push:

```
SAAD_LOGIN=<saad-github-username> bash tools/create_issues.sh
```

(`DRY_RUN=1` prints the plan.) It refuses to run if the repo already has issues, because bodies refer to #1 to #16. Saad must be a collaborator or the assignment fails. If Claude Code has GitHub access, it can run this itself once the username is known.

## 9. Face design decisions (from the design discussion)

The design reasoning the user asked for ("think like a pro designer, psychology, design philosophies"). Implement in `src/face/`.

**Eyes only, no mouth.** Eyes carry most readable emotion. A mouth costs rows of pixels and pushes toward an uncanny cartoon face. The lid cut works as the eyebrow: a diagonal lid reads angry, a drooping outer lid reads sad, a raised lower lid reads happy. Large eyes set slightly low and wide apart read as friendly. Aliveness comes from timing: irregular blinks, small gaze jumps, a tiny dip before each move.

**Four concepts were previewed** (A solid rounded eyes, B visor frame, C star-iris rings, D bridge arch). Recommended: **C (star iris) with D's bridge arch as a status overlay**. Use A or B on a 128x32 panel. Layout numbers at 128x64:

| Concept | Eye size, radius, gap, centre y |
|---------|---------------------------------|
| A solid | 34x38, r 11, gap 16, cy 34 (eye centres x = 39 and 89) |
| B visor | eyes 30x28, r 9, gap 18, cy 32, inside a 2 px outlined frame (width 2w+gap+18, height eyeH+14, r 10) |
| C star iris | 36x36 circles (r 18), gap 14, cy 34, 2.2 px ring. Pupil is a four-point star (astroid, a about 6.5 px) offset by gaze. One orbiting 2x2 dot pair per connected peer, orbit radius min(w,h)/2 - 2.2, 1.4 rad/s |
| D bridge | eyes 26x34, r 9, gap 36, cy 32. A 2 px arch between the eyes (height 8) at y = cy + 0.18h. Modes: none, marching dashes (searching), solid (linked), broken with flicker (weak or lost) |

**Parametric eye math** (draw per pixel, strictly 1-bit; canvas anti-aliasing is not used):
- Inside a rounded rect (w, h, r) at (cx, cy).
- `s = (x - cx) / (w/2) * dir`, where `dir` is +1 for the left eye and -1 for the right (so s = +1 is the inner edge).
- `cutTop = top + lt*h + sl*h*0.4*s`, `cutBottom = bottom - lb*h*(1 - 0.5*s*s)`. Pixel is lit if y is between them.
- If `cutBottom - cutTop < 1.3`, draw a 1 px line at cy so a closed eye is a line, not nothing.
- `lt` top lid 0..1, `sl` lid slope (positive = angry, negative = sad), `lb` bottom lid 0..1 (smile).
- Blink: `blink = sin(pi * t / 0.16 s)`, then `lt' = lt + blink*(1-lt)*0.55`, `lb' = lb + blink*(1-lb)*0.55`.
- Ring eyes: draw the outer shape minus an inner shape inset by 2.2 px, scaling `lt`, `sl`, `lb` by `h/(h - 4.4)` so the lid edge stays closed.

**State parameter sets** (lids and gaze are eased with exponential smoothing, about 14/s for lids and 22/s for gaze):

| State | lt | sl | lb | Gaze | Extras |
|-------|----|----|----|------|--------|
| idle | 0 | 0 | 0 | random saccades, hold 0.6 to 2.2 s | independent random blinks every 2 to 5.5 s |
| searching | 0.22 | 0.1 | 0 | sweep `sin(1.7t)*0.9` | marching-dash bridge |
| handshake | 0 | 0 | 0.22 | toward peer (0.9) | eyes scale 1.1, synchronized blink, pixel hops across |
| linked | 0 | 0 | 0.3 | toward peer (0.4) | solid bridge, edge port lit, shared blink schedule every 2.4 to 4.8 s |
| data | as linked | | 0.25 | follows the hopping pixel | comet-style pixel crosses from A's right edge to B's left edge, 2.2 s cycle |
| weak | 0.4 | -0.45 | 0 | down (0, 0.6) | checkerboard dither on lit pixels, flickering broken bridge |
| lost | 0.3 | -0.55 | 0 | away and down | row-tear glitch, no bridge |
| startled | 0 | 0 | 0 | jitter +-0.15 | eyes scale 1.18 (pickup or freefall) |
| sleep | 0.93 | 0 | 0 | centre | face-down, lower contrast |

**Network meaning on the face** (the novel part; the eye shapes themselves are not novel):
- Synchronized blink means bonded: unlinked bots blink at random, linked bots blink on the same frame.
- Gaze direction shows who a bot is talking to.
- A pixel hops from one screen's edge to the neighbouring screen's edge to show data transfer (needs the screens side by side).
- Dither means weak signal. Row-tear glitch means link lost.
- The MPU6050 drives gaze (tilt), startle (pickup, freefall), dizzy (shake) and sleep (face-down).

**OLED notes:** blue pixels age fastest. Shift the whole image by 1 px every few minutes, lower contrast in sleep, keep idle eyes moving. A checkerboard gives a fake third grey level on a 1-bit screen. A runnable interactive preview of these concepts existed only in the chat; the parameters above are enough to rebuild it.

## 10. Recommended next steps

1. Get the repo pushed (section 4), then create the issues (section 8).
2. Write `src/config` (issue #1), then `src/hal/display` (#3) and `src/face` renderer (#4), replacing the bring-up code with module code. Commit each as it lands and log frame time in `task_report.md`.
3. Compile and run the fixed `oled_test.ino` at 800000UL and 1000000UL and record fps and any garbling (#15).
4. Attach the MPU6050, run the I2C scan, confirm 0x68, and write `hal/imu` (#7) then `gestures` (#8).
5. Network modules (identity, net, peers, events, console, integration) are Saad's. If he has not started, ask the user which to take.

## 11. Open questions for the user

- Saad's GitHub username, and whether he is already a collaborator.
- Install the Claude GitHub App on `ariyan-cmd/starbridge` if the user wants Claude to push and manage issues from sessions.
- MPU6050 AD0 pin level (0x68 or 0x69).
- Any DHT or LDR fitted, and on which pins.
- Which gesture toggles the status screen (suggestion: double tap).

## 12. Gotchas worth remembering

- Arduino IDE needs the sketch folder name to equal the `.ino` name, and it compiles `src/` recursively. Include modules as `"src/<module>/x.h"` from the `.ino`, and with relative paths inside `src/`.
- ESP-NOW on the ESP8266 uses different roles and callback signatures from ESP32. Never do work (parsing, printing) inside the receive callback; queue the frame and handle it in `loop()`.
- `LED_BUILTIN` on NodeMCU is active low.
- ESP8266 has about 80 KB RAM: no heap allocation in the event queue, peer table or dedupe table.
- `tools/create_issues.sh` is bash. On Windows run it from Git Bash.
