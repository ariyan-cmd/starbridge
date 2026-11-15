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
uint8_t *displayBuffer() { return oled.getBuffer(); }
