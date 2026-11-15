// Star-Bridge main sketch.
// Kept thin on purpose: each module owns its logic under src/.
// Modules get wired in here as they are written, in the order listed in docs/v0.1-plan.md.

#include "src/config/config.h"
#include "src/hal/display.h"
#include "src/face/face.h"

static void haltBlink() {
  pinMode(LED_BUILTIN, OUTPUT);
  for (;;) {  // fast blink = display not found (LED is active low)
    digitalWrite(LED_BUILTIN, LOW);  delay(120);
    digitalWrite(LED_BUILTIN, HIGH); delay(120);
  }
}

// TEMPORARY preview of the face renderer (#4). Replaced by the face state machine in #11.
struct Preset { const char *name; FaceParams p; };
static const Preset presets[] = {
  //            lt     sl     lb    gx    gy    scale  blink ox oy dither bridge         peers
  {"idle",      {0,     0,     0,    0,    0,    1.0f,  0,    0, 0, false, BRIDGE_NONE,   0}},
  {"searching", {0.22f, 0.1f,  0,    0.9f, 0,    1.0f,  0,    0, 0, false, BRIDGE_SEARCH, 0}},
  {"linked",    {0,     0,     0.3f, 0.4f, 0,    1.0f,  0,    0, 0, false, BRIDGE_LINKED, 2}},
  {"angry",     {0,     0.9f,  0,    0,    0,    1.0f,  0,    0, 0, false, BRIDGE_NONE,   0}},
  {"weak",      {0.4f, -0.45f, 0,    0,    0.6f, 1.0f,  0,    0, 0, true,  BRIDGE_WEAK,   0}},
  {"startled",  {0,     0,     0,    0,    0,    1.18f, 0,    0, 0, false, BRIDGE_NONE,   0}},
  {"sleep",     {0.93f, 0,     0,    0,    0,    1.0f,  0,    0, 0, false, BRIDGE_NONE,   0}},
};
static const int NPRESETS = sizeof(presets) / sizeof(presets[0]);
static const uint32_t PRESET_MS = 4000;

static FaceParams cur = presets[0].p;
static int idx = 0;
static uint32_t presetStart, lastFrame, renderUs, flushUs, frames;

static void ease(float &v, float target, float rate, float dt) { v += (target - v) * (1 - expf(-rate * dt)); }

void setup() {
  Serial.begin(115200);
  delay(300);
  Serial.printf("\nStar-Bridge proto v%d, screen %dx%d, I2C %lu Hz\n",
                PROTO_VERSION, SCREEN_WIDTH, SCREEN_HEIGHT, (unsigned long)I2C_CLOCK_HZ);
  if (!displayInit()) {
    Serial.println(F("Display init failed: check wiring, OLED_I2C_ADDR and SCREEN_HEIGHT"));
    haltBlink();
  }
  faceInit();
  Serial.printf("free heap: %u bytes\n", ESP.getFreeHeap());
  presetStart = lastFrame = millis();
}

void loop() {
  uint32_t now = millis();
  float dt = (now - lastFrame) / 1000.0f;
  lastFrame = now;

  if (now - presetStart >= PRESET_MS) {
    uint32_t n = frames ? frames : 1;
    Serial.printf("preset %-9s: %lu frames in %lu ms = %.1f fps, render %lu us, flush %lu us\n",
                  presets[idx].name, (unsigned long)frames, (unsigned long)(now - presetStart),
                  frames * 1000.0f / (now - presetStart),
                  (unsigned long)(renderUs / n), (unsigned long)(flushUs / n));
    idx = (idx + 1) % NPRESETS;
    presetStart = now; frames = renderUs = flushUs = 0;
    if (idx == 0) Serial.printf("free heap: %u bytes\n", ESP.getFreeHeap());
  }

  const FaceParams &t = presets[idx].p;
  ease(cur.lt, t.lt, 14, dt); ease(cur.sl, t.sl, 14, dt); ease(cur.lb, t.lb, 14, dt);
  ease(cur.gx, t.gx, 22, dt); ease(cur.gy, t.gy, 22, dt); ease(cur.scale, t.scale, 14, dt);
  cur.dither = t.dither; cur.bridge = t.bridge; cur.peers = t.peers;
  uint32_t ph = now % 2300;                                    // blink every 2.3 s, 160 ms long
  cur.blink = ph < 160 ? sinf(PI * ph / 160.0f) : 0;

  uint32_t a = micros();
  faceRender(cur, now);
  uint32_t b = micros();
  displayFlush();
  uint32_t c = micros();
  renderUs += b - a; flushUs += c - b; frames++;
}
