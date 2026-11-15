#include "gestures.h"
#include <math.h>
#include "../config/config.h"
#include "../hal/imu.h"

// Board orientation: flat and face up reads z = +1 g. Samples arrive at loop rate (about 30 Hz), so every *_MS value
// below is effectively rounded up to a whole number of samples.

static GestureCb cb;
static uint32_t lastFire[7];  // per-code lockout, indexed by GestureCode

static bool fire(GestureCode c, int16_t arg, uint32_t now, uint32_t lockMs) {
  if (lastFire[c] && now - lastFire[c] < lockMs) return false;
  lastFire[c] = now;
  if (cb) cb(c, arg);
  return true;
}

// Time the condition has held, 0 if it is false. `since` keeps the start of the current run.
static uint32_t held(bool cond, uint32_t &since, uint32_t now) {
  if (!cond) { since = 0; return 0; }
  if (!since) since = now;
  return now - since + 1;  // +1 so a run that just began counts as 1 ms, not "false"
}

void gesturesBegin(GestureCb c) {
  cb = c;
  for (auto &t : lastFire) t = 0;
}

void gesturesFeed(float ax, float ay, float az, uint32_t now) {
  float mag = sqrtf(ax * ax + ay * ay + az * az);
  if (now == 0) now = 1;  // 0 is the "never fired" marker

  // FREEFALL: |a| near zero for a short run.
  static uint32_t ffSince;
  if (held(mag < GESTURE_FREEFALL_G, ffSince, now) >= GESTURE_FREEFALL_MIN_MS) fire(GESTURE_FREEFALL, 0, now, GESTURE_FREEFALL_LOCK_MS);

  // SHAKE: N rising edges through the shake threshold inside a window.
  static bool above;
  static uint8_t peaks;
  static uint32_t peakStart;
  bool nowAbove = mag > GESTURE_SHAKE_G;
  if (nowAbove && !above) {
    if (!peaks || now - peakStart > GESTURE_SHAKE_WINDOW_MS) { peaks = 0; peakStart = now; }
    if (++peaks >= GESTURE_SHAKE_PEAKS) { peaks = 0; fire(GESTURE_SHAKE, 0, now, GESTURE_SHAKE_LOCK_MS); }
  }
  above = nowAbove;

  // TAP: a spike over the tap threshold that ends within TAP_MAX_MS.
  static uint32_t tapSince;
  static float tapPeak;
  if (mag > GESTURE_TAP_G) {
    if (!tapSince) { tapSince = now; tapPeak = 0; }
    if (mag > tapPeak) tapPeak = mag;
  } else if (tapSince) {
    if (now - tapSince <= GESTURE_TAP_MAX_MS) fire(GESTURE_TAP, (int16_t)(tapPeak * 100), now, GESTURE_TAP_LOCK_MS);
    tapSince = 0;
  }

  // TILT: dominant of x/y beyond TILT_G while roughly face up, held for TILT_HOLD_MS. Fires on change only.
  static int16_t tilt = TILT_NONE, tiltCand = TILT_NONE;
  static uint32_t tiltSince;
  int16_t dir = TILT_NONE;
  if (az > 0.5f) {
    if (fabsf(ax) >= fabsf(ay)) { if (fabsf(ax) > GESTURE_TILT_G) dir = ax > 0 ? TILT_PLUS_X : TILT_MINUS_X; }
    else if (fabsf(ay) > GESTURE_TILT_G) dir = ay > 0 ? TILT_PLUS_Y : TILT_MINUS_Y;
  }
  if (dir != tiltCand) { tiltCand = dir; tiltSince = now; }
  if (tiltCand != tilt && now - tiltSince >= GESTURE_TILT_HOLD_MS) { tilt = tiltCand; fire(GESTURE_TILT, tilt, now, 0); }

  // PICKUP: |a| leaves 1 g after a rest period, and stays off for PICKUP_MIN_MS.
  static uint32_t restSince, devSince, restedFor;
  float dev = fabsf(mag - 1.0f);
  if (held(dev < GESTURE_REST_DEV_G, restSince, now) >= GESTURE_REST_MS) restedFor = now;
  uint32_t devFor = held(dev > GESTURE_PICKUP_DEV_G, devSince, now);
  if (restedFor && now - restedFor <= GESTURE_REST_DEV_GRACE_MS && devFor >= GESTURE_PICKUP_MIN_MS) {
    restedFor = 0;
    fire(GESTURE_PICKUP, 0, now, GESTURE_PICKUP_LOCK_MS);
  }

  // FACE_DOWN: z strongly negative for FACE_DOWN_MS. Re-arms once the board is clearly not face down.
  static uint32_t fdSince;
  static bool fdFired;
  if (held(az < GESTURE_FACE_DOWN_Z, fdSince, now) >= GESTURE_FACE_DOWN_MS && !fdFired) { fdFired = true; fire(GESTURE_FACE_DOWN, 0, now, 0); }
  if (az > GESTURE_FACE_DOWN_Z + 0.3f) fdFired = false;
}

void gesturesUpdate(uint32_t now) {
  float ax, ay, az;
  if (imuRead(ax, ay, az)) gesturesFeed(ax, ay, az, now);
}
