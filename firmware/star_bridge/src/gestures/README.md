# gestures

Turns raw acceleration into discrete events: SHAKE, TAP, TILT, PICKUP, FACE_DOWN, FREEFALL.

- Input: samples from `hal/imu`, polled from `loop()` (no blocking).
- Output: one callback `(GestureCode, int16 arg)`; `events` (#12) registers it. Codes and args are in `gestures.h` and map to the EVENT packet.
- Thresholds live in `config`, so tuning does not touch code.

Depends on: config, hal (imu).
