#pragma once
// hal/display: the only place that touches the SSD1306. Other modules draw via displayGfx().
#include <Adafruit_GFX.h>

bool displayInit();             // Wire + SSD1306 at OLED_I2C_ADDR. False if the panel does not answer.
void displayClear();            // Clears the framebuffer only; call displayFlush() to show it.
void displayFlush();            // Sends the whole framebuffer over I2C (~26 ms at 400 kHz, measured).
void displaySetContrast(uint8_t c);
void displayDim(bool on);
Adafruit_GFX &displayGfx();     // Draw target (SCREEN_WIDTH x SCREEN_HEIGHT, 1-bit).
void displayDither(int y0, int y1, uint8_t level);  // 1-bit fade: keep level/16 of the lit pixels in rows y0..y1 (4x4 Bayer). 16 = untouched.
