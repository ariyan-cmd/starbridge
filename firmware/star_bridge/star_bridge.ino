// Star-Bridge main sketch.
// Kept thin on purpose: each module owns its logic under src/.
// Modules get wired in here as they are written, in the order listed in docs/v0.1-plan.md.

#include "src/config/config.h"
#include "src/hal/display.h"

static void haltBlink() {
  pinMode(LED_BUILTIN, OUTPUT);
  for (;;) {  // fast blink = display not found (LED is active low)
    digitalWrite(LED_BUILTIN, LOW);  delay(120);
    digitalWrite(LED_BUILTIN, HIGH); delay(120);
  }
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

  // Bring-up screen: border, text, then time 100 full flushes for task_report.
  Adafruit_GFX &g = displayGfx();
  displayClear();
  g.drawRect(0, 0, SCREEN_WIDTH, SCREEN_HEIGHT, 1);
  g.setTextColor(1);
  g.setTextSize(1);
  g.setCursor(8, 8);   g.print(F("Star-Bridge"));
  g.setCursor(8, 24);  g.print(F("hal/display OK"));
  displayFlush();
  uint32_t t0 = micros();
  for (int i = 0; i < 100; i++) displayFlush();
  Serial.printf("displayFlush: %lu us/frame over 100 flushes\n", (unsigned long)((micros() - t0) / 100));
  Serial.printf("free heap: %u bytes\n", ESP.getFreeHeap());
}

void loop() {
}
