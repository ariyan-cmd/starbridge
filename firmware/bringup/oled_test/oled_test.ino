/*
  Star-Bridge / DestroBot  -  OLED bring-up test
  ------------------------------------------------
  Board   : NodeMCU 1.0 (ESP-12E Module)   Serial Monitor: 115200 baud
  Library : "Adafruit SSD1306" + "Adafruit GFX Library" (Library Manager)
  Wiring  : VCC -> 3V3, GND -> GND, SDA -> D2 (GPIO4), SCL -> D1 (GPIO5)

  Order of tests (watch the screen, and the Serial Monitor for the log):
    I2C scan -> display init
    TEXT   : sizes, glyphs, numbers, inverted, marquee
    SHAPES : pixels, lines, rects, round rects, circles, triangles, bitmap, bounce
    DISPLAY: dither (half-tone), hardware scroll, contrast sweep
    DESIGN : eye preview (blink, gaze, angry lids)
    SPEED  : frames per second at 400 kHz / 800 kHz / 1 MHz

  Type 'r' in the Serial Monitor to rerun everything. After the tests the
  eye preview loops forever.

  For the 0.96" 128x64 panel later, change SCREEN_HEIGHT to 64. Nothing else.
*/

#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>

#define SCREEN_WIDTH   128
#define SCREEN_HEIGHT  64          // 0.96" panel: 128x64
#define OLED_RESET     -1          // no reset pin, shares board reset
#define I2C_SDA        4           // D2
#define I2C_SCL        5           // D1
#define NORMAL_CONTRAST ((SCREEN_HEIGHT == 32) ? 0x8F : 0xCF)

Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);

uint8_t oledAddr = 0x3C;
float fps400 = 0, fps800 = 0, fps1000 = 0;
int testNo = 0;

// 16x16 four-point star (the "Star" in Star-Bridge)
static const unsigned char PROGMEM starBmp[] = {
  0x01, 0x80, 0x01, 0x80, 0x03, 0xC0, 0x07, 0xE0,
  0x0F, 0xF0, 0x3F, 0xFC, 0xFF, 0xFF, 0x7F, 0xFE,
  0x7F, 0xFE, 0xFF, 0xFF, 0x3F, 0xFC, 0x0F, 0xF0,
  0x07, 0xE0, 0x03, 0xC0, 0x01, 0x80, 0x01, 0x80
};

// ---------------------------------------------------------------- helpers
void halt(const __FlashStringHelper *msg) {
  Serial.println(msg);
  pinMode(LED_BUILTIN, OUTPUT);
  while (true) {                       // fast blink = something is wrong
    digitalWrite(LED_BUILTIN, LOW);  delay(120);
    digitalWrite(LED_BUILTIN, HIGH); delay(120);
  }
}

uint8_t scanI2C() {
  uint8_t found = 0;
  int count = 0;
  Serial.println(F("Scanning I2C bus..."));
  for (uint8_t a = 1; a < 127; a++) {
    Wire.beginTransmission(a);
    if (Wire.endTransmission() == 0) {
      Serial.print(F("  device at 0x"));
      Serial.println(a, HEX);
      count++;
      if (!found && (a == 0x3C || a == 0x3D)) found = a;
    }
  }
  if (!count) Serial.println(F("  nothing found"));
  return found;
}

void banner(const char *name) {
  testNo++;
  Serial.print(F("["));
  Serial.print(testNo);
  Serial.print(F("] "));
  Serial.println(name);
  display.clearDisplay();
  display.setTextSize(1);
  display.setTextColor(SSD1306_WHITE);
  display.setTextWrap(true);
  display.setCursor(0, 0);
  display.print(testNo);
  display.print(F(". "));
  display.print(name);
  display.display();
  delay(700);
  display.clearDisplay();
  display.display();
}

void setContrast(uint8_t c) {
  display.ssd1306_command(SSD1306_SETCONTRAST);
  display.ssd1306_command(c);
}

int maxDim() { return SCREEN_WIDTH > SCREEN_HEIGHT ? SCREEN_WIDTH : SCREEN_HEIGHT; }

// ---------------------------------------------------------------- TEXT
void testTextSizes() {
  banner("Text sizes");
  display.clearDisplay();
  display.setTextColor(SSD1306_WHITE);
  display.setTextSize(1);
  display.setCursor(0, 0);
  display.println(F("Size 1  DestroBot v0.1"));
  display.println(F("abcdefghijklmnopqrstu"));
  display.println(F("ABCDEFGHIJKLMNOPQRSTU"));
  display.println(F("0123456789 !@#$%^&*()"));
  display.display();
  delay(1800);

  display.clearDisplay();
  display.setTextSize(2);
  display.setCursor(0, 0);
  display.println(F("DestroBot"));
  display.println(F("StarBridge"));
  display.display();
  delay(1500);

  display.clearDisplay();
  display.setTextSize(3);
  display.setCursor(0, 0);
  display.println(F("OK 123"));
  display.display();
  delay(1500);
}

void testGlyphsAndNumbers() {
  banner("Glyphs + numbers");
  display.clearDisplay();
  display.setTextSize(1);
  display.setTextColor(SSD1306_WHITE);
  display.cp437(true);                       // use the full CP437 glyph table
  display.setCursor(0, 0);
  for (uint8_t i = 1; i <= 20; i++) display.write(i);   // smileys, hearts, arrows
  display.println();
  display.print(F("pi="));   display.println(3.14159f, 3);
  display.print(F("hex=0x")); display.println(0xBEEF, HEX);
  display.print(F("int="));  display.println(-12345);
  display.print(F("up="));   display.print(millis() / 1000); display.println(F("s"));
  display.display();
  delay(2500);
  display.cp437(false);
}

void testInverted() {
  banner("Inverted text");
  display.clearDisplay();
  display.fillRect(0, 0, SCREEN_WIDTH, 12, SSD1306_WHITE);
  display.setTextColor(SSD1306_BLACK, SSD1306_WHITE);   // dark text on lit bar
  display.setTextSize(1);
  display.setCursor(2, 2);
  display.print(F(" LINK  ESTABLISHED "));
  display.setTextColor(SSD1306_WHITE);
  display.setCursor(0, 16);
  display.print(F("normal text below bar"));
  display.display();
  delay(1500);
  display.invertDisplay(true);                          // whole-panel invert
  delay(1000);
  display.invertDisplay(false);
  delay(500);
}

void testMarquee() {
  banner("Scrolling text");
  const char msg[] = "STAR-BRIDGE ONLINE   ";
  int w = strlen(msg) * 12;                 // size 2 = 12 px per char
  display.setTextSize(2);
  display.setTextColor(SSD1306_WHITE);
  display.setTextWrap(false);
  for (int x = SCREEN_WIDTH; x > -w; x -= 3) {
    display.clearDisplay();
    display.setCursor(x, (SCREEN_HEIGHT - 16) / 2);
    display.print(msg);
    display.display();
  }
  display.setTextWrap(true);
}

// ---------------------------------------------------------------- SHAPES
void testPixelsAndBorder() {
  banner("Pixels / dead-pixel check");
  display.fillScreen(SSD1306_WHITE);        // look for dark or stuck pixels
  display.display();
  delay(1200);
  display.clearDisplay();
  display.drawRect(0, 0, SCREEN_WIDTH, SCREEN_HEIGHT, SSD1306_WHITE);   // all 4 edges visible?
  display.drawPixel(2, 2, SSD1306_WHITE);
  display.drawPixel(SCREEN_WIDTH - 3, 2, SSD1306_WHITE);
  display.drawPixel(2, SCREEN_HEIGHT - 3, SSD1306_WHITE);
  display.drawPixel(SCREEN_WIDTH - 3, SCREEN_HEIGHT - 3, SSD1306_WHITE);
  display.drawLine(0, 0, SCREEN_WIDTH - 1, SCREEN_HEIGHT - 1, SSD1306_WHITE);
  display.drawLine(0, SCREEN_HEIGHT - 1, SCREEN_WIDTH - 1, 0, SSD1306_WHITE);
  display.display();
  delay(1800);
}

void testLines() {
  banner("Lines");
  display.clearDisplay();
  for (int i = 0; i < SCREEN_WIDTH; i += 6) {
    display.drawLine(0, 0, i, SCREEN_HEIGHT - 1, SSD1306_WHITE);
    display.display();
    delay(15);
  }
  for (int i = SCREEN_HEIGHT - 1; i >= 0; i -= 4) {
    display.drawLine(0, 0, SCREEN_WIDTH - 1, i, SSD1306_WHITE);
    display.display();
    delay(15);
  }
  delay(800);
  display.clearDisplay();
  for (int i = 0; i < SCREEN_WIDTH; i += 6) {
    display.drawLine(SCREEN_WIDTH - 1, SCREEN_HEIGHT - 1, i, 0, SSD1306_WHITE);
    display.display();
    delay(15);
  }
  delay(800);
}

void testRects() {
  banner("Rectangles");
  display.clearDisplay();
  for (int i = 0; i < SCREEN_HEIGHT / 2; i += 3) {
    display.drawRect(i, i, SCREEN_WIDTH - 2 * i, SCREEN_HEIGHT - 2 * i, SSD1306_WHITE);
    display.display();
    delay(60);
  }
  delay(500);
  display.clearDisplay();
  for (int i = 0; i < SCREEN_HEIGHT / 2; i += 3) {       // INVERSE makes nested stripes
    display.fillRect(i, i, SCREEN_WIDTH - 2 * i, SCREEN_HEIGHT - 2 * i, SSD1306_INVERSE);
    display.display();
    delay(60);
  }
  delay(800);
}

void testRoundRects() {
  banner("Round rects (eyes!)");
  display.clearDisplay();
  for (int i = 0; i < SCREEN_HEIGHT / 2 - 2; i += 3) {
    display.drawRoundRect(i, i, SCREEN_WIDTH - 2 * i, SCREEN_HEIGHT - 2 * i,
                          SCREEN_HEIGHT / 4, SSD1306_WHITE);
    display.display();
    delay(60);
  }
  delay(500);
  display.clearDisplay();                                  // two filled eyes
  int h = SCREEN_HEIGHT - 8;
  display.fillRoundRect(22, 4, 34, h, h / 3, SSD1306_WHITE);
  display.fillRoundRect(72, 4, 34, h, h / 3, SSD1306_WHITE);
  display.display();
  delay(1500);
}

void testCircles() {
  banner("Circles");
  display.clearDisplay();
  for (int i = 2; i < SCREEN_HEIGHT / 2 + 12; i += 3) {
    display.drawCircle(SCREEN_WIDTH / 2, SCREEN_HEIGHT / 2, i, SSD1306_WHITE);
    display.display();
    delay(50);
  }
  delay(500);
  display.clearDisplay();
  for (int i = maxDim() / 2; i > 0; i -= 3) {
    display.fillCircle(SCREEN_WIDTH / 2, SCREEN_HEIGHT / 2, i, SSD1306_INVERSE);
    display.display();
    delay(30);
  }
  delay(800);
}

void testTriangles() {
  banner("Triangles");
  display.clearDisplay();
  for (int i = 0; i < maxDim() / 2; i += 4) {
    display.drawTriangle(SCREEN_WIDTH / 2, SCREEN_HEIGHT / 2 - i,
                         SCREEN_WIDTH / 2 - i, SCREEN_HEIGHT / 2 + i,
                         SCREEN_WIDTH / 2 + i, SCREEN_HEIGHT / 2 + i, SSD1306_WHITE);
    display.display();
    delay(50);
  }
  delay(400);
  display.clearDisplay();
  for (int i = maxDim() / 2; i > 0; i -= 4) {
    display.fillTriangle(SCREEN_WIDTH / 2, SCREEN_HEIGHT / 2 - i,
                         SCREEN_WIDTH / 2 - i, SCREEN_HEIGHT / 2 + i,
                         SCREEN_WIDTH / 2 + i, SCREEN_HEIGHT / 2 + i, SSD1306_INVERSE);
    display.display();
    delay(50);
  }
  delay(800);
}

void testBitmap() {
  banner("Bitmap (star)");
  int y = (SCREEN_HEIGHT - 16) / 2;
  for (int x = 0; x < SCREEN_WIDTH - 16; x += 2) {
    display.clearDisplay();
    display.drawBitmap(x, y, starBmp, 16, 16, SSD1306_WHITE);
    display.drawBitmap(SCREEN_WIDTH - 16 - x, y, starBmp, 16, 16, SSD1306_WHITE);
    display.display();
  }
  delay(500);
}

void testBounce() {
  banner("Motion / ghosting");
  float x = 20, y = 10, vx = 2.3f, vy = 1.7f;
  int r = 4;
  uint32_t t0 = millis();
  while (millis() - t0 < 3000) {
    x += vx; y += vy;
    if (x < r || x > SCREEN_WIDTH - 1 - r)  { vx = -vx; x += vx; }
    if (y < r || y > SCREEN_HEIGHT - 1 - r) { vy = -vy; y += vy; }
    display.clearDisplay();
    display.drawRect(0, 0, SCREEN_WIDTH, SCREEN_HEIGHT, SSD1306_WHITE);
    display.fillCircle((int)x, (int)y, r, SSD1306_WHITE);
    display.display();
  }
}

// ---------------------------------------------------------------- DISPLAY FEATURES
void testDither() {
  banner("Dither (half-tone)");
  display.clearDisplay();
  display.fillRect(0, 0, 42, SCREEN_HEIGHT, SSD1306_WHITE);                  // solid
  for (int y = 0; y < SCREEN_HEIGHT; y++)                                     // checkerboard
    for (int x = 43; x < 86; x++)
      if ((x + y) & 1) display.drawPixel(x, y, SSD1306_WHITE);
  display.drawRect(86, 0, 42, SCREEN_HEIGHT, SSD1306_WHITE);                  // empty
  display.display();
  Serial.println(F("   left=solid, middle=checker (should look dimmer), right=outline"));
  delay(2500);
}

void testHardwareScroll() {
  banner("Hardware scroll");
  display.clearDisplay();
  display.setTextSize(2);
  display.setTextColor(SSD1306_WHITE);
  display.setCursor(0, (SCREEN_HEIGHT - 16) / 2);
  display.println(F("scroll"));
  display.display();
  delay(300);
  display.startscrollright(0x00, 0x0F);  delay(2000);
  display.stopscroll();                  delay(300);
  display.startscrollleft(0x00, 0x0F);   delay(2000);
  display.stopscroll();                  delay(300);
  display.startscrolldiagright(0x00, 0x07); delay(2000);
  display.stopscroll();
  delay(300);
}

void testContrast() {
  banner("Contrast sweep");
  display.clearDisplay();
  display.setTextSize(1);
  display.setTextColor(SSD1306_WHITE);
  for (int pass = 0; pass < 2; pass++) {
    for (int c = (pass ? 255 : 0); pass ? c >= 0 : c <= 255; c += (pass ? -5 : 5)) {
      setContrast((uint8_t)c);
      display.clearDisplay();
      display.setCursor(0, 0);
      display.print(F("contrast ")); display.print(c);
      display.fillRect(0, SCREEN_HEIGHT - 12, map(c, 0, 255, 2, SCREEN_WIDTH), 10, SSD1306_WHITE);
      display.display();
      delay(25);
    }
  }
  setContrast(NORMAL_CONTRAST);
  display.dim(true);                                   // lowest-power brightness
  display.clearDisplay();
  display.setCursor(0, 0);
  display.print(F("dim(true)"));
  display.fillRect(0, 12, SCREEN_WIDTH, SCREEN_HEIGHT - 12, SSD1306_WHITE);
  display.display();
  delay(1500);
  display.dim(false);
  setContrast(NORMAL_CONTRAST);
}

// ---------------------------------------------------------------- DESIGN PREVIEW
void drawEye(int cx, int cy, int w, int h) {
  int r = h / 3;
  if (r > h / 2) r = h / 2;
  display.fillRoundRect(cx - w / 2, cy - h / 2, w, h, r, SSD1306_WHITE);
}

void eyesDemo(uint32_t durationMs) {
  const int w = 34, gap = 16;
  const int h = (SCREEN_HEIGHT == 32) ? 22 : 38;
  const int cy = SCREEN_HEIGHT / 2;
  const int cxL = SCREEN_WIDTH / 2 - gap / 2 - w / 2;
  const int cxR = SCREEN_WIDTH / 2 + gap / 2 + w / 2;
  uint32_t t0 = millis();
  while (millis() - t0 < durationMs) {
    uint32_t t = millis() - t0;
    int gx = (int)(sin(t / 650.0) * 5);
    int gy = (int)(sin(t / 910.0) * 2);
    uint32_t ph = t % 2300;                       // blink every 2.3 s, 160 ms long
    float blink = (ph < 160) ? sin(PI * ph / 160.0) : 0;
    int hh = (int)(h * (1.0 - 0.9 * blink));
    if (hh < 3) hh = 3;
    display.clearDisplay();
    drawEye(cxL + gx, cy + gy, w, hh);
    drawEye(cxR + gx, cy + gy, w, hh);
    if (t > durationMs * 2 / 3) {                 // last third: angry lids
      int top = cy + gy - hh / 2 - 1;
      int cut = hh * 4 / 10;
      display.fillTriangle(cxL + gx - w / 2 - 1, top, cxL + gx + w / 2 + 1, top,
                           cxL + gx + w / 2 + 1, top + cut, SSD1306_BLACK);
      display.fillTriangle(cxR + gx + w / 2 + 1, top, cxR + gx - w / 2 - 1, top,
                           cxR + gx - w / 2 - 1, top + cut, SSD1306_BLACK);
    }
    display.display();
  }
}

void testEyes() {
  banner("Eye preview");
  eyesDemo(7000);
}

// ---------------------------------------------------------------- SPEED
float fpsTest(uint32_t clk) {
  Wire.setClock(clk);
  uint32_t frames = 0, t0 = millis();
  while (millis() - t0 < 1500) {
    display.clearDisplay();
    display.fillRect((frames * 4) % (SCREEN_WIDTH - 12), 0, 12, SCREEN_HEIGHT, SSD1306_WHITE);
    display.display();
    frames++;
  }
  float fps = frames * 1000.0f / (millis() - t0);
  Wire.setClock(400000);
  return fps;
}

void testSpeed() {
  banner("Speed (fps)");
  fps400  = fpsTest(400000);
  fps800  = fpsTest(800000);
  fps1000 = fpsTest(1000000);
  Serial.print(F("   400 kHz: ")); Serial.print(fps400);  Serial.println(F(" fps"));
  Serial.print(F("   800 kHz: ")); Serial.print(fps800);  Serial.println(F(" fps"));
  Serial.print(F("   1 MHz  : ")); Serial.print(fps1000); Serial.println(F(" fps"));
  Serial.println(F("   If the bar looked torn or garbled at 800k/1M, stay on 400k."));
  display.clearDisplay();
  display.setTextSize(1);
  display.setTextColor(SSD1306_WHITE);
  display.setCursor(0, 0);
  display.print(F("400k: ")); display.print(fps400, 0);  display.println(F(" fps"));
  display.print(F("800k: ")); display.print(fps800, 0);  display.println(F(" fps"));
  display.print(F("1M  : ")); display.print(fps1000, 0); display.println(F(" fps"));
  display.display();
  delay(3500);
}

// ---------------------------------------------------------------- RUN
void runAll() {
  testNo = 0;
  Serial.println(F("\n=== TEXT ==="));
  testTextSizes();
  testGlyphsAndNumbers();
  testInverted();
  testMarquee();
  Serial.println(F("\n=== SHAPES ==="));
  testPixelsAndBorder();
  testLines();
  testRects();
  testRoundRects();
  testCircles();
  testTriangles();
  testBitmap();
  testBounce();
  Serial.println(F("\n=== DISPLAY FEATURES ==="));
  testDither();
  testHardwareScroll();
  testContrast();
  Serial.println(F("\n=== DESIGN PREVIEW ==="));
  testEyes();
  Serial.println(F("\n=== SPEED ==="));
  testSpeed();

  display.clearDisplay();
  display.setTextSize(2);
  display.setTextColor(SSD1306_WHITE);
  display.setCursor(0, 0);
  display.println(F("ALL DONE"));
  display.setTextSize(1);
  display.println(F("eyes loop; send 'r'"));
  display.display();
  Serial.println(F("\nAll tests finished. Eye preview loops. Send 'r' to rerun."));
  delay(2500);
}

void setup() {
  Serial.begin(115200);
  delay(300);
  Serial.println(F("\n\nStar-Bridge OLED test"));

  Wire.begin(I2C_SDA, I2C_SCL);
  uint8_t found = scanI2C();
  if (!found) halt(F("No SSD1306 at 0x3C/0x3D. Check wiring: SDA->D2, SCL->D1, VCC->3V3, GND."));
  oledAddr = found;

  if (!display.begin(SSD1306_SWITCHCAPVCC, oledAddr)) {
    halt(F("SSD1306 init failed. Check SCREEN_HEIGHT (32 vs 64) and the address."));
  }
  Wire.setClock(400000);
  Serial.print(F("Display OK at 0x")); Serial.println(oledAddr, HEX);

  display.clearDisplay();
  display.display();
  runAll();
}

void loop() {
  if (Serial.available() && Serial.read() == 'r') runAll();
  eyesDemo(7000);
}
