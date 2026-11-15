#pragma once
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
