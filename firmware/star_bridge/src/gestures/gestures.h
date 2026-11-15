#pragma once
// gestures: turns acceleration (g) into discrete events. Non-blocking: call gesturesUpdate() every loop().
#include <stdint.h>

// Codes are proposed here and must be agreed with events (#12). They map 1:1 to the EVENT packet's code (u8) and arg (i16).
enum GestureCode : uint8_t {
  GESTURE_SHAKE = 1,      // arg 0
  GESTURE_TAP = 2,        // arg = peak |a| x100
  GESTURE_TILT = 3,       // arg = TiltDir; TILT_NONE when levelled again
  GESTURE_PICKUP = 4,     // arg 0
  GESTURE_FACE_DOWN = 5,  // arg 0
  GESTURE_FREEFALL = 6,   // arg 0
};
enum TiltDir : int16_t { TILT_NONE = 0, TILT_PLUS_X = 1, TILT_MINUS_X = 2, TILT_PLUS_Y = 3, TILT_MINUS_Y = 4 };  // board axes

typedef void (*GestureCb)(GestureCode code, int16_t arg);

void gesturesBegin(GestureCb cb);                                   // Register the event callback and reset state.
void gesturesUpdate(uint32_t nowMs);                                // Reads hal/imu once and feeds the detectors.
void gesturesFeed(float ax, float ay, float az, uint32_t nowMs);    // Detectors only (no I2C), for tests and replay.
