#pragma once
#include <stdint.h>

// splash: boot animation (warp starfield, 4-point star, name, version, fade out). About 5 s.
// Draws and flushes the display itself, so call it after displayInit() and before the face starts.
//
// work(nowMs) runs once per frame, before the frame is drawn. Do one init step per call and return
// true once everything is ready (keep returning true; it may keep polling, e.g. the network).
// If work is still false when the splash reaches its fade-out, the splash holds on the final scene
// (star pulsing, sparkles twinkling) until it returns true, so the UI never appears half-ready.
// A step that blocks stalls the animation for that long; long waits must be split across calls.
void splashRun(bool (*work)(uint32_t nowMs));
