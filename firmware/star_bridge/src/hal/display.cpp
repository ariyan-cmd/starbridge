#include "display.h"
#include <Wire.h>
#include <Adafruit_SSD1306.h>
#include "../config/config.h"

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
void displayDither(int y0, int y1, uint8_t level) {
  static const uint8_t BAYER[4][4] = {{0, 8, 2, 10}, {12, 4, 14, 6}, {3, 11, 1, 9}, {15, 7, 13, 5}};
  if (level >= 16) return;
  if (y0 < 0) y0 = 0;
  if (y1 > SCREEN_HEIGHT - 1) y1 = SCREEN_HEIGHT - 1;
  uint8_t *buf = oled.getBuffer();  // SSD1306 page layout: byte x + (y/8)*W, bit y&7
  for (int y = y0; y <= y1; y++)
    for (int x = 0; x < SCREEN_WIDTH; x++)
      if (BAYER[y & 3][x & 3] >= level) buf[x + (y >> 3) * SCREEN_WIDTH] &= ~(1 << (y & 7));
}
