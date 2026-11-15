#include "splash.h"
#include <Arduino.h>
#include "../config/config.h"
#include "../hal/display.h"

// Timeline (ms)
//    0 - 1200  starfield warps outward, accelerating
// 1200 - 1700  4-point star grows in the centre (warp stars finish flying out)
// 1700 - 2000  star holds and pulses
// 2000 - 2500  star swipes up to the top
// 2300 - 2800  name fades up; background sparkles start twinkling in
// 2800 - 3300  version fades up
// 3300 - 4300  hold (extended while boot work is unfinished)
// 4300 - 5000  everything fades out
//
// The OLED is 1-bit, so "fade" = ordered dithering (4x4 Bayer): pixels are dropped in a fixed
// pattern until none are left.

static const char *NAME = "STARBRIDGE";  // 10 chars * 12 px = 120 px at size 2
static const char *VERSION = FW_VERSION_STR;

static const int NAME_Y = 33;  // final y of the name (size 2, 16 px tall)
static const int CAP_Y = 54;   // final y of the version (size 1, 8 px tall)

static const uint16_t T_SPAWN_END = 1200;  // stop spawning new warp stars
static const uint16_t T_STARS_END = 2000;  // stop drawing warp stars
static const uint16_t T_GROW_START = 1200;
static const uint16_t T_GROW_END = 1700;
static const uint16_t T_SWIPE_START = 2000;
static const uint16_t T_SWIPE_END = 2500;
static const uint16_t T_NAME_START = 2300;
static const uint16_t T_NAME_END = 2800;
static const uint16_t T_AMBIENT = 2300;  // background sparkles start appearing (with the name)
static const uint16_t T_CAP_START = 2800;
static const uint16_t T_CAP_END = 3300;
static const uint16_t T_FADE_START = 4300;
static const uint16_t T_END = 5000;

static Adafruit_GFX *g;

static float ramp(uint32_t t, uint16_t a, uint16_t b) {  // 0..1 between a and b
  if (t <= a) return 0.0f;
  if (t >= b) return 1.0f;
  return (t - a) / (float)(b - a);
}

static float easeOut(float t) {  // fast start, soft landing
  float u = 1.0f - t;
  return 1.0f - u * u * u;
}

static float smooth(float t) { return t * t * (3.0f - 2.0f * t); }

// Centered text that rises `rise` px while fading in. p = 0..1.
static void fadeUpText(const char *s, uint8_t size, int y, int rise, float p) {
  if (p <= 0.0f) return;
  int w = strlen(s) * 6 * size;
  int off = (int)(rise * (1.0f - easeOut(p)) + 0.5f);
  g->setTextSize(size);
  g->setCursor((SCREEN_WIDTH - w) / 2, y + off);
  g->print(s);
  displayDither(y, y + 8 * size + rise, (uint8_t)(p * 16.0f + 0.5f));
}

// ---------------- warp starfield ----------------
struct Star { float x, y, z; };  // x,y in [-1,1], z in (0,1], smaller z = closer
static const uint8_t N_STARS = 45;
static Star *stars;  // points at splashRun's stack array: no RAM held after boot

static const float K_X = 24.0f;  // projection scale, x
static const float K_Y = 14.0f;  // projection scale, y (screen is half as tall)

static void spawnStar(uint8_t i, bool anyDepth) {
  stars[i].x = random(-1000, 1001) / 1000.0f;
  stars[i].y = random(-1000, 1001) / 1000.0f;
  stars[i].z = anyDepth ? random(100, 1001) / 1000.0f : 1.0f;
}

static void project(float sx, float sy, float z, int &px, int &py) {
  px = SCREEN_WIDTH / 2 + (int)(sx / z * K_X);
  py = SCREEN_HEIGHT / 2 + (int)(sy / z * K_Y);
}

static void updateStars(float dt, float speed, bool spawn) {
  for (uint8_t i = 0; i < N_STARS; i++) {
    Star &s = stars[i];
    if (s.z <= 0) {  // dead star
      if (spawn) spawnStar(i, false);
      continue;
    }
    float zTail = s.z + speed * 0.05f;  // where the star was ~50 ms ago
    s.z -= speed * dt;

    int x1, y1, x0, y0;
    bool alive = s.z > 0.02f;
    if (alive) {
      project(s.x, s.y, s.z, x1, y1);
      alive = x1 >= 0 && x1 < SCREEN_WIDTH && y1 >= 0 && y1 < SCREEN_HEIGHT;
    }
    if (!alive) {
      if (spawn) spawnStar(i, false); else s.z = -1;
      continue;
    }
    project(s.x, s.y, zTail, x0, y0);
    g->drawLine(x0, y0, x1, y1, 1);
    if (s.z < 0.25f) g->drawPixel(x1 + 1, y1, 1);  // near stars look bigger
  }
}

// ---------------- 4-point star ----------------
static void drawSparkle(int cx, int cy, int r) {
  if (r < 1) return;
  int w = r / 3; if (w < 1) w = 1;
  g->fillTriangle(cx, cy - r, cx - w, cy, cx + w, cy, 1);  // up
  g->fillTriangle(cx, cy + r, cx - w, cy, cx + w, cy, 1);  // down
  g->fillTriangle(cx - r, cy, cx, cy - w, cx, cy + w, 1);  // left
  g->fillTriangle(cx + r, cy, cx, cy - w, cx, cy + w, 1);  // right
  if (r > 8) {                                             // gleam dots
    int d = r * 45 / 100;
    g->drawPixel(cx - d, cy - d, 1);
    g->drawPixel(cx + d, cy - d, 1);
    g->drawPixel(cx - d, cy + d, 1);
    g->drawPixel(cx + d, cy + d, 1);
  }
}

// ---------------- background sparkles (shown with the name) ----------------
// Each one cycles: dot -> small plus -> bigger plus -> small plus -> dot -> dark.
static const uint8_t N_AMB = 14;
static uint8_t *ambX, *ambY, *ambPh;  // also on splashRun's stack

static bool ambAllowed(int x, int y) {
  // keep clear of the big star (top centre), the name (y 33-48) and the version (y 54-61, centre)
  if (y < 30) return x < 44 || x > 84;   // top corners
  if (y >= 51) return x < 40 || x > 88;  // bottom corners
  return false;
}

static void initAmbient() {
  for (uint8_t i = 0; i < N_AMB; i++) {
    int x = 0, y = 0;
    for (uint8_t tries = 0; tries < 40; tries++) {  // reject spots too close to earlier ones
      x = random(1, SCREEN_WIDTH - 1); y = random(2, SCREEN_HEIGHT - 2);
      bool ok = ambAllowed(x, y);
      for (uint8_t j = 0; ok && j < i; j++)
        if (abs(x - ambX[j]) < 9 && abs(y - ambY[j]) < 9) ok = false;
      if (ok) break;
    }
    while (!ambAllowed(x, y)) { x = random(1, SCREEN_WIDTH - 1); y = random(2, SCREEN_HEIGHT - 2); }  // spacing relaxed
    ambX[i] = x; ambY[i] = y; ambPh[i] = random(0, 8);
  }
}

// t: timeline time (staggered appearance). anim: real time, keeps twinkling while the timeline holds.
static void drawAmbient(uint32_t t, uint32_t anim) {
  for (uint8_t i = 0; i < N_AMB; i++) {
    if (t < T_AMBIENT + (uint32_t)i * 70) continue;
    uint8_t k = ((anim / 140) + ambPh[i]) % 8;
    if (k > 4) continue;  // dark phase
    int x = ambX[i], y = ambY[i];
    g->drawPixel(x, y, 1);
    int arm = (k == 2 || k == 4) ? 1 : (k == 3 ? 2 : 0);
    for (int a = 1; a <= arm; a++) {
      g->drawPixel(x - a, y, 1);
      g->drawPixel(x + a, y, 1);
      g->drawPixel(x, y - a, 1);
      g->drawPixel(x, y + a, 1);
    }
  }
}

// ---------------- splash ----------------
void splashRun(bool (*work)(uint32_t nowMs)) {
  g = &displayGfx();
  Star starBuf[N_STARS];
  uint8_t amb[3][N_AMB];
  stars = starBuf; ambX = amb[0]; ambY = amb[1]; ambPh = amb[2];
  for (uint8_t i = 0; i < N_STARS; i++) spawnStar(i, true);
  initAmbient();
  g->setTextWrap(false);
  g->setTextColor(1);

  uint32_t start = millis(), t0 = start, last = start, t = 0;
  while (t < T_END) {
    bool ready = work(millis());
    uint32_t now = millis();  // after work: a step may take ~100 ms (transportInit)
    t = now - t0;
    if (!ready && t > T_FADE_START) { t0 = now - T_FADE_START; t = T_FADE_START; }  // hold before the fade
    uint32_t anim = now - start;
    float dt = (now - last) / 1000.0f;
    last = now;
    if (dt > 0.1f) dt = 0.1f;

    displayClear();

    // Text first, each followed by its own fade mask, so the masks never touch
    // anything drawn later (the star and warp lines are drawn on top).
    fadeUpText(NAME, 2, NAME_Y, 10, ramp(t, T_NAME_START, T_NAME_END));
    fadeUpText(VERSION, 1, CAP_Y, 8, ramp(t, T_CAP_START, T_CAP_END));

    // 4-point star: grows in the centre, pulses, then swipes up
    float gr = ramp(t, T_GROW_START, T_GROW_END);
    if (gr > 0.0f) {
      float r = 13.0f * easeOut(gr);
      if (gr >= 1.0f) r += 1.5f * sinf((anim - T_GROW_END) * 0.012f);  // starts at 0, no jump
      float s = smooth(ramp(t, T_SWIPE_START, T_SWIPE_END));
      int cy = SCREEN_HEIGHT / 2 + (int)((15 - SCREEN_HEIGHT / 2) * s);
      drawSparkle(SCREEN_WIDTH / 2, cy, (int)(r + 0.5f));
    }

    if (t >= T_AMBIENT) drawAmbient(t, anim);

    if (t < T_STARS_END) {
      float p = ramp(t, 0, T_SPAWN_END);
      updateStars(dt, 0.35f + 1.2f * p, t < T_SPAWN_END);
    }

    if (t >= T_FADE_START) {  // final fade-out of the whole screen
      float p = ramp(t, T_FADE_START, T_END);
      displayDither(0, SCREEN_HEIGHT - 1, (uint8_t)((1.0f - p) * 16.0f + 0.5f));
    }

    displayFlush();
    yield();  // ESP watchdog
  }
  displayClear();
  displayFlush();
  g->setTextSize(1);  // back to GFX defaults for whoever draws text next
  g->setTextWrap(true);
  g->setTextColor(1, 0);
}
