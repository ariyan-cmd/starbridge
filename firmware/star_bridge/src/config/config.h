#pragma once
// Build-time settings. No logic. Per-node overrides: local_config.h (git-ignored).
#include "pins.h"

#if __has_include("local_config.h")
#include "local_config.h"
#endif

#ifndef SCREEN_WIDTH
#define SCREEN_WIDTH 128  // OLED width in px. Used by hal/display, face.
#endif
#ifndef SCREEN_HEIGHT
#define SCREEN_HEIGHT 64  // OLED height in px (128x64 panel, D-004). Used by hal/display, face.
#endif
#ifndef OLED_I2C_ADDR
#define OLED_I2C_ADDR 0x3C  // SSD1306 address. Used by hal/display.
#endif
#ifndef MPU_I2C_ADDR
#define MPU_I2C_ADDR 0x68  // MPU6050 address, 0x69 if AD0 is high (0x68 confirmed on the first node). Used by hal/imu.
#endif
#ifndef I2C_CLOCK_HZ
#define I2C_CLOCK_HZ 400000UL  // I2C clock passed to the display constructor. Used by hal/display.
#endif

#define PROTO_VERSION 1  // Wire format version, see docs/protocol.md. Used by net.
#ifndef BEAT_INTERVAL_MS
#define BEAT_INTERVAL_MS 3000  // Heartbeat period, a guess (untested). Used by peers.
#endif
#ifndef MISSED_BEAT_LIMIT
#define MISSED_BEAT_LIMIT 3  // Beats missed before a peer is dropped, a guess. Used by peers.
#endif

// Gesture thresholds in g. PLACEHOLDERS, not measured yet (task_report: pending). Used by gestures.
#ifndef GESTURE_SHAKE_G
#define GESTURE_SHAKE_G 2.0f  // Peak |a| for a shake.
#endif
#ifndef GESTURE_TAP_G
#define GESTURE_TAP_G 1.8f  // Short spike for a tap.
#endif
#ifndef GESTURE_FREEFALL_G
#define GESTURE_FREEFALL_G 0.5f  // |a| below this means freefall (was 0.3, never fired by hand; a cable also drags the board).
#endif

// Detector timing and extra thresholds (#8). PLACEHOLDERS until tuned by hand: see task_report.md. Used by gestures.
#ifndef GESTURE_SHAKE_PEAKS
#define GESTURE_SHAKE_PEAKS 3  // Peaks over GESTURE_SHAKE_G needed...
#endif
#ifndef GESTURE_SHAKE_WINDOW_MS
#define GESTURE_SHAKE_WINDOW_MS 1000  // ...inside this window.
#endif
#ifndef GESTURE_SHAKE_LOCK_MS
#define GESTURE_SHAKE_LOCK_MS 1500  // Debounce after a shake.
#endif
#ifndef GESTURE_TAP_MAX_MS
#define GESTURE_TAP_MAX_MS 80  // A spike longer than this is not a tap.
#endif
#ifndef GESTURE_TAP_LOCK_MS
#define GESTURE_TAP_LOCK_MS 200
#endif
#ifndef GESTURE_TAP_QUIET_MS
#define GESTURE_TAP_QUIET_MS 500  // A pickup or shake jerk looks like weak taps; ignore taps this long after one.
#endif
#ifndef GESTURE_FREEFALL_MIN_MS
#define GESTURE_FREEFALL_MIN_MS 30  // |a| under GESTURE_FREEFALL_G for 2 samples (about 67 ms). Measured dips: 1 to 2 samples.
#endif
#ifndef GESTURE_FREEFALL_LOCK_MS
#define GESTURE_FREEFALL_LOCK_MS 1000
#endif
#ifndef GESTURE_TILT_G
#define GESTURE_TILT_G 0.35f  // x or y component beyond this (face up) is a tilt.
#endif
#ifndef GESTURE_TILT_HOLD_MS
#define GESTURE_TILT_HOLD_MS 150  // Direction must hold this long before it fires.
#endif
#ifndef GESTURE_REST_DEV_G
#define GESTURE_REST_DEV_G 0.12f  // | |a| - 1 | under this counts as at rest (measured resting spread up to 0.08 g).
#endif
#ifndef GESTURE_REST_MS
#define GESTURE_REST_MS 800  // Rest needed before a pickup can fire.
#endif
#ifndef GESTURE_REST_DEV_GRACE_MS
#define GESTURE_REST_DEV_GRACE_MS 500  // A pickup must start within this long of the last rest sample.
#endif
#ifndef GESTURE_PICKUP_DEV_G
#define GESTURE_PICKUP_DEV_G 0.2f  // | |a| - 1 | beyond this after rest starts a pickup.
#endif
#ifndef GESTURE_PICKUP_MIN_MS
#define GESTURE_PICKUP_MIN_MS 60  // ...and must last this long (a tap is shorter).
#endif
#ifndef GESTURE_PICKUP_LOCK_MS
#define GESTURE_PICKUP_LOCK_MS 1000
#endif
#ifndef GESTURE_FACE_DOWN_Z
#define GESTURE_FACE_DOWN_Z -0.8f  // z below this means face down.
#endif
#ifndef GESTURE_FACE_DOWN_MS
#define GESTURE_FACE_DOWN_MS 500
#endif
