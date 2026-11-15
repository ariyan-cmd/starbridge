// Star-Bridge main sketch.
// Kept thin on purpose: each module owns its logic under src/.
// Modules get wired in here as they are written, in the order listed in docs/v0.1-plan.md.

#include "src/net/protocol.h"  // also compiles its static_asserts on every build
#include "src/config/config.h"
#include "src/hal/display.h"
#include "src/hal/imu.h"
#include "src/gestures/gestures.h"
#include "src/face/face.h"
#include "src/identity/identity.h"
#include "src/net/codec.h"
#include "src/net/transport.h"
#include "src/peers/peers.h"
#include "src/splash/splash.h"

static void onPacket(const Packet &p) { peersOnPacket(p, millis()); }

static bool netOk;  // skip sending when ESP-NOW did not start

static bool sendMsg(uint8_t type, const void *payload, uint8_t len) {
  if (!netOk) return false;
  Packet out;
  size_t n = codecBuild(out, type, nodeId(), payload, len);
  return n && transportSend(&out, n);
}

static HelloPayload selfHello() {
  HelloPayload h = {};
  strncpy(h.name, nodeName(), NODE_NAME_LEN);
  h.caps = caps();
  h.fw_version = FW_VERSION;
  return h;
}

// TEMPORARY until the face (#11) and console (#13): print joins and drops.
static void onPeerChange(const Peer &p, bool joined) {
  Serial.printf("PEER %s %08X \"%s\" caps 0x%04X fw %u t=%lu (%u peers)\n", joined ? "JOIN" : "LOST", p.id, p.name,
                p.caps, p.fw_version, (unsigned long)millis(), peerCount());
}

// TEMPORARY gesture preview (#8): print each event so thresholds can be tuned by hand. Replaced by events (#12).
static void onGesture(GestureCode c, int16_t arg) {
  static const char *names[] = {"?", "SHAKE", "TAP", "TILT", "PICKUP", "FACE_DOWN", "FREEFALL"};
  Serial.printf("GESTURE %s arg=%d t=%lu\n", names[c], arg, (unsigned long)millis());
}

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

// Boot init, one step per splash frame so the animation keeps moving (see splash.h).
// Add a step for anything that must be ready before the UI shows; the splash holds until the last one is done.
static bool bootStep(uint32_t now) {
  static uint8_t stage;
  switch (stage) {
    case 0: {
      faceInit();
      uint32_t idStart = millis();
      identityInit();
      Serial.printf("node %08X \"%s\" caps 0x%04X, identityInit %lu ms\n",
                    nodeId(), nodeName(), caps(), (unsigned long)(millis() - idStart));
      break;
    }
    case 1:
      if (!imuInit()) Serial.printf("IMU init failed: WHO_AM_I=0x%02X (want 0x68), check wiring and MPU_I2C_ADDR\n", imuWhoAmI());
      gesturesBegin(onGesture);
      break;
    case 2: {
      codecInit(ESP.random());  // random first seq, see D-016
      uint32_t netStart = millis();
      netOk = transportInit(onPacket);
      Serial.printf("ESP-NOW %s on channel %d, transportInit %lu ms\n", netOk ? "up" : "FAILED", ESPNOW_CHANNEL,
                    (unsigned long)(millis() - netStart));
      break;
    }
    case 3:
      peersBegin(selfHello(), sendMsg, onPeerChange, now);
      Serial.printf("peers: beat %u ms, timeout %lu ms\n", BEAT_INTERVAL_MS, (unsigned long)peerTimeoutMs());
      break;
    default:  // all ready: keep gestures fed, answer HELLOs and keep the beat going while the splash finishes
      gesturesUpdate(now);
      transportPoll();
      peersUpdate(now);
      return true;
  }
  stage++;
  return false;
}

void setup() {
  Serial.begin(115200);
  delay(300);
  Serial.printf("\nStar-Bridge proto v%d, screen %dx%d, I2C %lu Hz\n",
                PROTO_VERSION, SCREEN_WIDTH, SCREEN_HEIGHT, (unsigned long)I2C_CLOCK_HZ);
  if (!displayInit()) {
    Serial.println(F("Display init failed: check wiring, OLED_I2C_ADDR and SCREEN_HEIGHT"));
    haltBlink();
  }
  uint32_t splashStart = millis();
  splashRun(bootStep);
  Serial.printf("splash %lu ms\n", (unsigned long)(millis() - splashStart));
  Serial.printf("free heap: %u bytes, boot %lu ms\n", ESP.getFreeHeap(), millis());
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

  gesturesUpdate(now);
  transportPoll();
  peersUpdate(now);

  static uint32_t lastStats;  // TEMPORARY link check (#6): stats every 10 s
  if (now - lastStats >= 10000) {
    lastStats = now;
    TransportStats st = transportStats();
    Serial.printf("net rx %lu dropped %lu parseErr %lu sendOk %lu sendFail %lu queued %u\n",
                  (unsigned long)st.rx, (unsigned long)st.rxDropped, (unsigned long)st.parseErr,
                  (unsigned long)st.sendOk, (unsigned long)st.sendFail, st.queued);
  }

  static String line;  // TEMPORARY rename check (#9) until the console (#13): "name <new name>"
  while (Serial.available()) {  // non-blocking: readStringUntil would stall the face for up to 1 s
    char ch = Serial.read();
    if (ch != '\n') { if (line.length() < 64) line += ch; continue; }
    line.trim();
    if (line.startsWith("name ")) {
      String arg = line.substring(5);
      arg.trim();
      bool ok = arg == "-" ? resetNodeName() : setNodeName(arg.c_str());  // "name -" forgets the saved name
      if (ok) peersSetSelf(selfHello(), millis());  // tell peers now, not at next boot
      Serial.printf("set name %s: now \"%s\"\n", ok ? "ok" : "rejected (1..16 printable chars)", nodeName());
    }
    line = "";
  }

  static uint32_t lastImu, imuStart;  // TEMPORARY IMU check (#7): min/max every loop (~30 Hz), print at 5 Hz, min/max per axis every 10 s
  static float mn[3] = {99, 99, 99}, mx[3] = {-99, -99, -99};
  float a3[3];
  if (imuRead(a3[0], a3[1], a3[2])) {
    for (int i = 0; i < 3; i++) { mn[i] = min(mn[i], a3[i]); mx[i] = max(mx[i], a3[i]); }
    if (now - lastImu >= 200) {
      lastImu = now;
      Serial.printf("g %+.3f %+.3f %+.3f\n", a3[0], a3[1], a3[2]);
    }
    if (now - imuStart >= 10000) {
      Serial.printf("10 s min/max g: x %+.3f/%+.3f y %+.3f/%+.3f z %+.3f/%+.3f\n", mn[0], mx[0], mn[1], mx[1], mn[2], mx[2]);
      for (int i = 0; i < 3; i++) { mn[i] = 99; mx[i] = -99; }
      imuStart = now;
    }
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
