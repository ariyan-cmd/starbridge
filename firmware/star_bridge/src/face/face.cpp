#include "face.h"
#include <math.h>
#include "../config/config.h"
#include "../hal/display.h"

// Layout numbers from HANDOFF section 9, concept C at 128x64.
static const float EYE_R = 18.0f;    // 36x36 circles
static const float RING = 2.2f;      // ring thickness
static const int EYE_DX = 25;        // eye centres at 64 -/+ 25 (gap 14)
static const int EYE_CY = 34;
static const float STAR_A = 6.5f;    // astroid size
static const float PUPIL_TRAVEL = 6.0f;

static const int STAR_COLS = 6;
static uint8_t starHalf[STAR_COLS + 1];  // astroid half-height per column offset

struct Eye {
  float cx, cy, r, dir, lt, sl, lb;
  bool dither;
};

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
