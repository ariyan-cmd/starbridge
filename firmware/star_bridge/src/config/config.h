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

// Name until one is set at runtime: NODE_NAME_DEFAULT if local_config.h defines it (1..16 chars),
// else "<NODE_NAME_PREFIX>-<chip ID hex>" so every node is distinct out of the box. Used by identity.
#ifndef NODE_NAME_PREFIX
#define NODE_NAME_PREFIX "star"  // 1..9 chars, so "-" and all 6 chip-ID hex digits fit in 16: "star-fcf9a8".
#endif
#ifndef SENSOR_CAPS
#define SENSOR_CAPS 0  // Fitted sensors I2C cannot detect: CAP_TEMP and/or CAP_LIGHT (net/protocol.h). Used by identity.
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
#define GESTURE_FREEFALL_G 0.3f  // |a| below this means freefall.
#endif
