#pragma once
// Gesture face test (throwaway, #8). Idle blink runs on its own; everything else is driven ONLY by accelerometer gestures.
//
// Enable: add `#define GESTURE_FACE_TEST` to src/config/local_config.h (git-ignored), then upload star_bridge as usual.
// Remove the line to get the normal sketch back.
//
//   TILT +x/-x/+y/-y  eyes look that way (levelled again: eyes return to centre)
//   SHAKE             dizzy, eyes circle (2 s)
//   TAP               startled, eyes widen (0.4 s)
//   PICKUP            happy, smile lids (2 s)
//   FREEFALL          scared, wide and sad (1.5 s)
//   FACE_DOWN         asleep, until the next gesture
//
// Every event is also printed on Serial so thresholds in src/config/config.h can be tuned.
// Uses haltBlink() and ease() from star_bridge.ino (include this file after them).
// If a tilt looks at the wrong side, swap the signs in gfOnGesture().

enum Mood : uint8_t { IDLE, DIZZY, STARTLED, HAPPY, SCARED, ASLEEP };
static Mood mood = IDLE;
static uint32_t moodUntil;          // 0 = no timeout
static float tiltGx, tiltGy;        // gaze from the last TILT
static FaceParams gfCur = {};         // eased, what is drawn
static uint32_t gfLast;

static void setMood(Mood m, uint32_t ms) { mood = m; moodUntil = ms ? millis() + ms : 0; }

static void gfOnGesture(GestureCode c, int16_t arg) {
  static const char *names[] = {"?", "SHAKE", "TAP", "TILT", "PICKUP", "FACE_DOWN", "FREEFALL"};
  Serial.printf("GESTURE %s arg=%d t=%lu\n", names[c], arg, (unsigned long)millis());
  switch (c) {
    case GESTURE_TILT:
      tiltGx = arg == TILT_PLUS_X ? 1 : arg == TILT_MINUS_X ? -1 : 0;
      tiltGy = arg == TILT_PLUS_Y ? 1 : arg == TILT_MINUS_Y ? -1 : 0;
      if (mood == ASLEEP) setMood(IDLE, 0);
      break;
    case GESTURE_SHAKE:     setMood(DIZZY, 2000); break;
    case GESTURE_TAP:       setMood(STARTLED, 400); break;
    case GESTURE_PICKUP:    setMood(HAPPY, 2000); break;
    case GESTURE_FREEFALL:  setMood(SCARED, 1500); break;
    case GESTURE_FACE_DOWN: setMood(ASLEEP, 0); break;
  }
}

static void gestureFaceSetup() {
  Serial.begin(115200);
  delay(300);
  Serial.println(F("\nGesture face test"));
  if (!displayInit()) { Serial.println(F("Display init failed")); haltBlink(); }
  faceInit();
  if (!imuInit()) { Serial.printf("IMU init failed: WHO_AM_I=0x%02X (want 0x68)\n", imuWhoAmI()); haltBlink(); }
  gesturesBegin(gfOnGesture);
  gfLast = millis();
}

static void gestureFaceLoop() {
  uint32_t now = millis();
  float dt = (now - gfLast) / 1000.0f;
  gfLast = now;

  gesturesUpdate(now);
  if (moodUntil && now >= moodUntil) setMood(IDLE, 0);

  float lt = 0, sl = 0, lb = 0, gx = tiltGx, gy = tiltGy, scale = 1.0f;
  switch (mood) {
    case DIZZY:    gx = 0.9f * sinf(now / 120.0f); gy = 0.9f * cosf(now / 120.0f); sl = 0.3f; break;
    case STARTLED: scale = 1.18f; break;
    case HAPPY:    lb = 0.3f; break;
    case SCARED:   scale = 1.18f; sl = -0.45f; lt = 0.15f; break;
    case ASLEEP:   lt = 0.93f; gx = gy = 0; break;
    default: break;
  }
  ease(gfCur.lt, lt, 14, dt); ease(gfCur.sl, sl, 14, dt); ease(gfCur.lb, lb, 14, dt);
  ease(gfCur.gx, gx, 22, dt); ease(gfCur.gy, gy, 22, dt); ease(gfCur.scale, scale, 14, dt);
  uint32_t ph = now % 2300;                                  // idle blink every 2.3 s, 160 ms long
  gfCur.blink = (mood == ASLEEP || ph >= 160) ? 0 : sinf(PI * ph / 160.0f);

  faceRender(gfCur, now);
  displayFlush();
}
