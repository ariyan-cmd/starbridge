# gestures

Turns raw acceleration into discrete events: SHAKE, TAP, TILT, PICKUP, FACE_DOWN, FREEFALL.

- Input: samples from `hal/imu`, polled from `loop()` (no blocking).
- Output: gesture events pushed to the `events` module.
- Thresholds live in `config`, so tuning does not touch code.

Depends on: config, hal (imu).
