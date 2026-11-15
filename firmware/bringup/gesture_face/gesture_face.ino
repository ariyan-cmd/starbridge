/*
  Star-Bridge / DestroBot  -  gesture face test (single file)
  ------------------------------------------------------------
  Board   : NodeMCU 1.0 (ESP-12E Module)   Serial Monitor: 115200 baud
  Libraries (Library Manager): "Adafruit SSD1306" and "Adafruit GFX Library"
  Wiring  : OLED and MPU6050 both on 3V3, GND, SDA -> D2, SCL -> D1 (MPU6050 AD0 -> GND)

  On every power-up it calibrates for 10 s ("CALIBRATE" on screen): leave the board still, resting the way it will
  normally sit. The average gravity reading becomes "up" (z = +1.00 g exactly, so the +0.07 g offset seen on the
  bench is removed), and x and y are rebuilt from the sensor's x axis around it, so tilts, face-down and the rest
  are all relative to the surface it was on. If it is moved during calibration, the 10 s restart ("HOLD STILL").

  Idle blink runs on its own. Everything else is driven ONLY by accelerometer gestures:

    TILT +x/-x/+y/-y  eyes look that way (levelled again: eyes return to centre)
    SHAKE             dizzy, eyes circle (2 s)
    TAP               startled, eyes widen (0.4 s)
    PICKUP            happy, smile lids (2 s)
    FREEFALL          scared, wide and sad (1.5 s)
    FACE_DOWN         asleep, until the next gesture

  Every event is also printed on Serial. All thresholds are in the GESTURE_* defines below.
  If a tilt looks at the wrong side, swap the signs in gfOnGesture().
  The modules below are copied from firmware/star_bridge/src (config, display, imu, face, gestures); the calibration
  and test sections at the bottom exist only here.
*/
#include <Wire.h>
#include <math.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>

// ---------------- config ----------------
// Pin map. Used by: hal (display, imu), face.

#define PIN_I2C_SDA 4  // D2 (GPIO4). Used by hal.
#define PIN_I2C_SCL 5  // D1 (GPIO5). Used by hal.

// Build-time settings. No logic. Per-node overrides: local_config.h (git-ignored).

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
#ifndef GESTURE_TAP_AFTER_PICKUP_MS
#define GESTURE_TAP_AFTER_PICKUP_MS 500  // A pickup jerk looks like a weak tap; ignore taps this long after a pickup.
#endif
#ifndef GESTURE_FREEFALL_MIN_MS
#define GESTURE_FREEFALL_MIN_MS 60  // |a| must stay under GESTURE_FREEFALL_G this long.
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

// ---------------- types (kept above all functions: Arduino inserts prototypes before the first function) ----------------
// hal/display: the only place that touches the SSD1306. Other modules draw via displayGfx().

bool displayInit();             // Wire + SSD1306 at OLED_I2C_ADDR. False if the panel does not answer.
void displayClear();            // Clears the framebuffer only; call displayFlush() to show it.
void displayFlush();            // Sends the whole framebuffer over I2C (~26 ms at 400 kHz, measured).
void displaySetContrast(uint8_t c);
void displayDim(bool on);
Adafruit_GFX &displayGfx();     // Draw target (SCREEN_WIDTH x SCREEN_HEIGHT, 1-bit).

// hal/imu: the only place that touches the MPU6050. Acceleration in g, gyro in deg/s.
#include <stdint.h>

bool imuInit();                                 // Wake, set +-4 g / 100 Hz. False if absent or WHO_AM_I is not 0x68.
uint8_t imuWhoAmI();                            // Last WHO_AM_I value read (0 if the chip did not answer). For diagnostics.
bool imuRead(float &ax, float &ay, float &az);  // g. False on I2C failure (outputs untouched).
bool imuReadGyro(float &gx, float &gy, float &gz);  // deg/s (+-500 range). False on I2C failure.

#include <stdint.h>

// face: parametric eyes (concept C star-iris rings + concept D bridge arch). Draws into the
// display framebuffer only; the caller flushes. State machine and blink timing live in #11.

enum BridgeMode : uint8_t { BRIDGE_NONE, BRIDGE_SEARCH, BRIDGE_LINKED, BRIDGE_WEAK };

struct FaceParams {
  float lt;      // top lid 0..1
  float sl;      // lid slope, + angry, - sad
  float lb;      // bottom lid 0..1 (smile)
  float gx, gy;  // gaze -1..1
  float scale;   // eye size, 1.0 = 36 px
  float blink;   // 0..1, applied on top of the lids (sin pulse from the caller)
  int8_t ox, oy; // whole-image shift in px (OLED burn-in)
  bool dither;   // checkerboard on lit pixels (weak signal)
  BridgeMode bridge;
  uint8_t peers; // orbiting dot pairs, max 4
};

void faceInit();
void faceRender(const FaceParams &p, uint32_t nowMs);

struct Eye {
  float cx, cy, r, dir, lt, sl, lb;
  bool dither;
};
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

enum Mood : uint8_t { IDLE, DIZZY, STARTLED, HAPPY, SCARED, ASLEEP };

// ---------------- hal/display ----------------

// The library resets the I2C clock to the constructor value before every transfer,
// so the clock must be given here (see task_report, 2026-11-15 debug entry).
static Adafruit_SSD1306 oled(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, -1, I2C_CLOCK_HZ, 400000UL);

bool displayInit() {
  Wire.begin(PIN_I2C_SDA, PIN_I2C_SCL);
  if (!oled.begin(SSD1306_SWITCHCAPVCC, OLED_I2C_ADDR)) return false;
  oled.clearDisplay();
  oled.display();
  return true;
}

void displayClear() { oled.clearDisplay(); }
void displayFlush() { oled.display(); }

void displaySetContrast(uint8_t c) {
  oled.ssd1306_command(SSD1306_SETCONTRAST);
  oled.ssd1306_command(c);
}

void displayDim(bool on) { oled.dim(on); }
Adafruit_GFX &displayGfx() { return oled; }

// ---------------- hal/imu ----------------
#include <Wire.h>

enum : uint8_t {
  REG_SMPLRT_DIV = 0x19, REG_CONFIG = 0x1A, REG_GYRO_CFG = 0x1B, REG_ACCEL_CFG = 0x1C,
  REG_ACCEL_OUT = 0x3B, REG_GYRO_OUT = 0x43, REG_PWR_MGMT_1 = 0x6B, REG_WHO_AM_I = 0x75,
};
static const float ACCEL_LSB_PER_G = 8192.0f;   // +-4 g range
static const float GYRO_LSB_PER_DPS = 65.5f;    // +-500 deg/s range

static uint8_t whoAmI;
static bool ready;  // set only after init fully succeeds, so a half-configured chip never reads as valid

static bool writeReg(uint8_t reg, uint8_t v) {
  Wire.beginTransmission(MPU_I2C_ADDR);
  Wire.write(reg); Wire.write(v);
  return Wire.endTransmission() == 0;
}

static bool readRegs(uint8_t reg, uint8_t *buf, uint8_t n) {
  Wire.beginTransmission(MPU_I2C_ADDR);
  Wire.write(reg);
  if (Wire.endTransmission(false) != 0) return false;
  if (Wire.requestFrom((uint8_t)MPU_I2C_ADDR, n) != n) return false;
  for (uint8_t i = 0; i < n; i++) buf[i] = Wire.read();
  return true;
}

static bool readXyz(uint8_t reg, float lsbPerUnit, float &x, float &y, float &z) {
  uint8_t b[6];
  if (!readRegs(reg, b, 6)) return false;
  x = (int16_t)((b[0] << 8) | b[1]) / lsbPerUnit;
  y = (int16_t)((b[2] << 8) | b[3]) / lsbPerUnit;
  z = (int16_t)((b[4] << 8) | b[5]) / lsbPerUnit;
  return true;
}

bool imuInit() {
  Wire.begin(PIN_I2C_SDA, PIN_I2C_SCL);
  whoAmI = 0; ready = false;
  if (!readRegs(REG_WHO_AM_I, &whoAmI, 1) || whoAmI != 0x68) return false;  // 0x68 even when AD0 is high
  ready = writeReg(REG_PWR_MGMT_1, 0x01)   // wake, clock from gyro X PLL
       && writeReg(REG_CONFIG, 0x03)       // DLPF 44 Hz, base rate 1 kHz
       && writeReg(REG_SMPLRT_DIV, 9)      // 1 kHz / 10 = 100 Hz
       && writeReg(REG_ACCEL_CFG, 0x08)    // +-4 g
       && writeReg(REG_GYRO_CFG, 0x08);    // +-500 deg/s
  if (ready) delay(20);                    // let the PLL clock and first sample settle
  return ready;
}

uint8_t imuWhoAmI() { return whoAmI; }
bool imuRead(float &ax, float &ay, float &az) { return ready && readXyz(REG_ACCEL_OUT, ACCEL_LSB_PER_G, ax, ay, az); }
bool imuReadGyro(float &gx, float &gy, float &gz) { return ready && readXyz(REG_GYRO_OUT, GYRO_LSB_PER_DPS, gx, gy, gz); }

// ---------------- face ----------------
#include <math.h>

// Layout numbers from HANDOFF section 9, concept C at 128x64.
static const float EYE_R = 18.0f;    // 36x36 circles
static const float RING = 2.2f;      // ring thickness
static const int EYE_DX = 25;        // eye centres at 64 -/+ 25 (gap 14)
static const int EYE_CY = 34;
static const float STAR_A = 6.5f;    // astroid size
static const float PUPIL_TRAVEL = 6.0f;

static const int STAR_COLS = 6;
static uint8_t starHalf[STAR_COLS + 1];  // astroid half-height per column offset

static Adafruit_GFX *g;

void faceInit() {
  g = &displayGfx();
  float a23 = powf(STAR_A, 2.0f / 3.0f);
  for (int i = 0; i <= STAR_COLS; i++) {
    float v = a23 - powf((float)i, 2.0f / 3.0f);
    starHalf[i] = v > 0 ? (uint8_t)(powf(v, 1.5f) + 0.5f) : 0;
  }
}

static void put(int x, int y0, int y1, bool dither) {
  if (y1 < y0) return;
  if (!dither) { g->drawFastVLine(x, y0, y1 - y0 + 1, 1); return; }
  for (int y = y0; y <= y1; y++) if (((x + y) & 1) == 0) g->drawPixel(x, y, 1);
}

// Lit rows at column offset dx for a circle of radius rad with the lid cuts. False if empty.
// gap (optional) gets the lid-to-lid distance, used to detect a closed eye.
static bool span(const Eye &e, float rad, float dx, int &yT, int &yB, float *gap = nullptr) {
  float h = 2 * e.r, s = dx / e.r * e.dir, inset = e.r - rad;
  float cutT = e.cy - e.r + inset + e.lt * h + e.sl * h * 0.4f * s;
  float cutB = e.cy + e.r - inset - e.lb * h * (1 - 0.5f * s * s);
  if (gap) *gap = cutB - cutT;
  float h2 = rad * rad - dx * dx;
  if (h2 <= 0) return false;
  float ho = sqrtf(h2);
  yT = (int)ceilf(fmaxf(e.cy - ho, cutT));
  yB = (int)floorf(fminf(e.cy + ho, cutB));
  return yT <= yB;
}

static void drawEye(const Eye &e, float gx, float gy) {
  int ri = (int)ceilf(e.r);
  for (int d = -ri; d <= ri; d++) {
    int x = (int)e.cx + d, oT, oB, iT, iB;
    float gap = 0;
    bool outer = span(e, e.r, d, oT, oB, &gap);
    if (gap < 1.3f) {                       // closed eye: 1 px line, not nothing
      if (e.r * e.r - d * d > 0) put(x, (int)e.cy, (int)e.cy, e.dither);
      continue;
    }
    if (!outer) continue;
    if (span(e, e.r - RING, d, iT, iB)) {   // ring = outer shape minus inset shape
      put(x, oT, min(iT - 1, oB), e.dither);
      put(x, max(iB + 1, oT), oB, e.dither);
    } else {
      put(x, oT, oB, e.dither);
    }
  }
  // star pupil, clipped to the lid cut
  int px = (int)(e.cx + gx * PUPIL_TRAVEL), py = (int)(e.cy + gy * PUPIL_TRAVEL);
  for (int sx = -STAR_COLS; sx <= STAR_COLS; sx++) {
    int yh = starHalf[sx < 0 ? -sx : sx], iT, iB;
    if (!span(e, e.r - RING, px + sx - e.cx, iT, iB)) continue;
    put(px + sx, max(py - yh, iT), min(py + yh, iB), e.dither);
  }
}

static void drawBridge(BridgeMode m, int ox, int oy, uint32_t now) {
  if (m == BRIDGE_NONE) return;
  if (m == BRIDGE_WEAK && ((now / 70) & 3) == 0) return;      // flicker
  const int N = 13, H = 4, base = EYE_CY + 10;
  uint32_t phase = now / 90;
  for (int i = 0; i < N; i++) {
    if (m == BRIDGE_SEARCH && ((i + phase) & 3) >= 2) continue;   // marching dashes
    if (m == BRIDGE_WEAK && i >= 5 && i <= 7) continue;           // broken in the middle
    float u = (i - (N - 1) / 2.0f) / ((N - 1) / 2.0f);
    int y = base - (int)(H * (1 - u * u) + 0.5f) + oy;
    g->drawFastVLine(64 - (N - 1) / 2 + i + ox, y, 2, 1);
  }
}

void faceRender(const FaceParams &p, uint32_t now) {
  displayClear();
  float b = p.blink;
  Eye e = {0, (float)(EYE_CY + p.oy), EYE_R * p.scale, 1,
           p.lt + b * (1 - p.lt) * 0.55f, p.sl, p.lb + b * (1 - p.lb) * 0.55f, p.dither};
  e.cx = (float)(64 - EYE_DX + p.ox); e.dir = 1;  drawEye(e, p.gx, p.gy);
  e.cx = (float)(64 + EYE_DX + p.ox); e.dir = -1; drawEye(e, p.gx, p.gy);
  drawBridge(p.bridge, p.ox, p.oy, now);
  float t = now / 1000.0f;
  for (int k = 0; k < p.peers && k < 4; k++) {      // one orbiting dot pair per peer, on each eye
    float a = 1.4f * t + k * 1.57f, rr = EYE_R - RING;
    for (int side = -1; side <= 1; side += 2)
      for (int j = 0; j < 2; j++) {
        float aa = a + j * 3.14159f;
        g->fillRect((int)(64 + side * EYE_DX + p.ox + rr * cosf(aa)),
                    (int)(EYE_CY + p.oy + rr * sinf(aa)), 2, 2, 1);
      }
  }
}

// ---------------- gestures ----------------
#include <math.h>

// Board orientation: flat and face up reads z = +1 g. Samples arrive at loop rate (about 30 Hz), so every *_MS value
// below is effectively rounded up to a whole number of samples.

static GestureCb cb;
static uint32_t lastFire[7];  // per-code lockout, indexed by GestureCode

static bool gestureFire(GestureCode c, int16_t arg, uint32_t now, uint32_t lockMs) {
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
  if (held(mag < GESTURE_FREEFALL_G, ffSince, now) >= GESTURE_FREEFALL_MIN_MS) gestureFire(GESTURE_FREEFALL, 0, now, GESTURE_FREEFALL_LOCK_MS);

  // SHAKE: N rising edges through the shake threshold inside a window.
  static bool above;
  static uint8_t peaks;
  static uint32_t peakStart;
  bool nowAbove = mag > GESTURE_SHAKE_G;
  if (nowAbove && !above) {
    if (!peaks || now - peakStart > GESTURE_SHAKE_WINDOW_MS) { peaks = 0; peakStart = now; }
    if (++peaks >= GESTURE_SHAKE_PEAKS) { peaks = 0; gestureFire(GESTURE_SHAKE, 0, now, GESTURE_SHAKE_LOCK_MS); }
  }
  above = nowAbove;

  // TAP: a spike over the tap threshold that ends within TAP_MAX_MS.
  static uint32_t tapSince;
  static float tapPeak;
  if (mag > GESTURE_TAP_G) {
    if (!tapSince) { tapSince = now; tapPeak = 0; }
    if (mag > tapPeak) tapPeak = mag;
  } else if (tapSince) {
    bool afterPickup = lastFire[GESTURE_PICKUP] && now - lastFire[GESTURE_PICKUP] < GESTURE_TAP_AFTER_PICKUP_MS;
    if (now - tapSince <= GESTURE_TAP_MAX_MS && !afterPickup) gestureFire(GESTURE_TAP, (int16_t)(tapPeak * 100), now, GESTURE_TAP_LOCK_MS);
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
  if (tiltCand != tilt && now - tiltSince >= GESTURE_TILT_HOLD_MS) { tilt = tiltCand; gestureFire(GESTURE_TILT, tilt, now, 0); }

  // PICKUP: |a| leaves 1 g after a rest period, and stays off for PICKUP_MIN_MS.
  static uint32_t restSince, devSince, restedFor;
  float dev = fabsf(mag - 1.0f);
  if (held(dev < GESTURE_REST_DEV_G, restSince, now) >= GESTURE_REST_MS) restedFor = now;
  uint32_t devFor = held(dev > GESTURE_PICKUP_DEV_G, devSince, now);
  if (restedFor && now - restedFor <= GESTURE_REST_DEV_GRACE_MS && devFor >= GESTURE_PICKUP_MIN_MS) {
    restedFor = 0;
    gestureFire(GESTURE_PICKUP, 0, now, GESTURE_PICKUP_LOCK_MS);
  }

  // FACE_DOWN: z strongly negative for FACE_DOWN_MS. Re-arms once the board is clearly not face down.
  static uint32_t fdSince;
  static bool fdFired;
  if (held(az < GESTURE_FACE_DOWN_Z, fdSince, now) >= GESTURE_FACE_DOWN_MS && !fdFired) { fdFired = true; gestureFire(GESTURE_FACE_DOWN, 0, now, 0); }
  if (az > GESTURE_FACE_DOWN_Z + 0.3f) fdFired = false;
}

void gesturesUpdate(uint32_t now) {
  float ax, ay, az;
  if (imuRead(ax, ay, az)) gesturesFeed(ax, ay, az, now);
}

// ---------------- test sketch ----------------
static void haltBlink() {
  pinMode(LED_BUILTIN, OUTPUT);
  for (;;) { digitalWrite(LED_BUILTIN, LOW); delay(120); digitalWrite(LED_BUILTIN, HIGH); delay(120); }
}
static void ease(float &v, float target, float rate, float dt) { v += (target - v) * (1 - expf(-rate * dt)); }

// ---- calibration: rest vector -> "up", sensor axes -> relative axes ----
static const uint32_t CAL_MS = 10000;
static const float CAL_MAX_SPREAD_G = 0.15f;  // any axis moving more than this restarts the 10 s (placeholder)
static float calR[3][3] = {{1, 0, 0}, {0, 1, 0}, {0, 0, 1}};
static float calG = 1.0f;                     // measured |g| at rest, divided out so rest reads 1.000 g

static void applyCal(float &x, float &y, float &z) {
  float a = x, b = y, c = z;
  x = (calR[0][0] * a + calR[0][1] * b + calR[0][2] * c) / calG;
  y = (calR[1][0] * a + calR[1][1] * b + calR[1][2] * c) / calG;
  z = (calR[2][0] * a + calR[2][1] * b + calR[2][2] * c) / calG;
}

static void showText(const char *big, const char *small) {
  Adafruit_GFX &d = displayGfx();
  displayClear();
  d.setTextColor(1);
  d.setTextSize(2); d.setCursor(4, 12); d.print(big);
  d.setTextSize(1); d.setCursor(4, 44); d.print(small);
  displayFlush();
}

static void calibrate() {
  float sum[3] = {0, 0, 0}, mn[3], mx[3], spread = 0;
  uint32_t n = 0, start = millis(), lastDraw = 0, movedAt = 0;
  for (int i = 0; i < 3; i++) { mn[i] = 99; mx[i] = -99; }
  while (millis() - start < CAL_MS) {
    float a[3];
    if (imuRead(a[0], a[1], a[2])) {
      spread = 0;
      n++;
      for (int i = 0; i < 3; i++) { sum[i] += a[i]; mn[i] = min(mn[i], a[i]); mx[i] = max(mx[i], a[i]); spread = max(spread, mx[i] - mn[i]); }
      if (spread > CAL_MAX_SPREAD_G) {                  // moved: start over
        for (int i = 0; i < 3; i++) { sum[i] = 0; mn[i] = 99; mx[i] = -99; }
        n = 0; start = movedAt = millis();
      }
    }
    if (millis() - lastDraw >= 200) {
      lastDraw = millis();
      char buf[24];
      snprintf(buf, sizeof buf, "keep still %us", (unsigned)((CAL_MS - (millis() - start) + 999) / 1000));
      showText(movedAt && millis() - movedAt < 1500 ? "HOLD STILL" : "CALIBRATE", buf);
    }
    delay(5);
  }
  float m = 0, u[3];
  if (n) { for (int i = 0; i < 3; i++) { u[i] = sum[i] / n; m += u[i] * u[i]; } m = sqrtf(m); }
  if (!n || m < 0.5f) {                                 // no usable reading: keep the sensor's own axes
    Serial.println(F("Calibration failed: no valid rest reading, using raw axes"));
    showText("CAL FAILED", "using raw axes");
    delay(1500);
    return;
  }
  for (int i = 0; i < 3; i++) u[i] /= m;                // unit "up" in sensor axes
  // x' = sensor x with the "up" part removed (sensor y if x is almost parallel to up); z' = up; y' = z' cross x'
  float ex[3] = {1, 0, 0};
  if (fabsf(u[0]) > 0.9f) { ex[0] = 0; ex[1] = 1; }
  float d = ex[0] * u[0] + ex[1] * u[1] + ex[2] * u[2], x[3], xn = 0;
  for (int i = 0; i < 3; i++) { x[i] = ex[i] - d * u[i]; xn += x[i] * x[i]; }
  xn = sqrtf(xn);
  for (int i = 0; i < 3; i++) x[i] /= xn;
  float y[3] = {u[1] * x[2] - u[2] * x[1], u[2] * x[0] - u[0] * x[2], u[0] * x[1] - u[1] * x[0]};
  for (int i = 0; i < 3; i++) { calR[0][i] = x[i]; calR[1][i] = y[i]; calR[2][i] = u[i]; }
  calG = m;
  Serial.printf("Calibrated: rest g = %+.3f %+.3f %+.3f, |g| = %.3f, spread %.3f g, %lu samples\n",
                u[0] * m, u[1] * m, u[2] * m, m, spread, (unsigned long)n);
  showText("CALIBRATED", "starting...");
  delay(800);
}

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
      tiltGx = arg == TILT_PLUS_X ? -1 : arg == TILT_MINUS_X ? 1 : 0;  // flipped after hand test: left/right were reversed
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

void setup() {
  Serial.begin(115200);
  delay(300);
  Serial.println(F("\nGesture face test"));
  if (!displayInit()) { Serial.println(F("Display init failed")); haltBlink(); }
  faceInit();
  if (!imuInit()) { Serial.printf("IMU init failed: WHO_AM_I=0x%02X (want 0x68)\n", imuWhoAmI()); haltBlink(); }
  calibrate();
  gesturesBegin(gfOnGesture);
  gfLast = millis();
}

void loop() {
  uint32_t now = millis();
  float dt = (now - gfLast) / 1000.0f;
  gfLast = now;

  float ax, ay, az;
  if (imuRead(ax, ay, az)) {
    applyCal(ax, ay, az);
    gesturesFeed(ax, ay, az, now);
    // Diagnostic for tuning freefall: report every dip of |a| under 0.7 g with its lowest value and length.
    static float lowMin = 9; static uint8_t lowN; static uint32_t lowT;
    float mag = sqrtf(ax * ax + ay * ay + az * az);
    if (mag < 0.7f) { if (!lowN) lowT = now; lowN++; if (mag < lowMin) lowMin = mag; }
    else if (lowN) { Serial.printf("LOW |a| min=%.2f g, %u samples, %lu ms\n", lowMin, lowN, (unsigned long)(now - lowT)); lowN = 0; lowMin = 9; }
  }
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
