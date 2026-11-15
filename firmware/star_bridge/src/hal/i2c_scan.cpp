#include "i2c_scan.h"
#include <Wire.h>
#include "../config/config.h"

uint8_t i2cScan(uint8_t *found, uint8_t max) {
  Wire.begin(PIN_I2C_SDA, PIN_I2C_SCL);
  uint8_t n = 0;
  for (uint8_t a = 0x08; a <= 0x77 && n < max; a++) {  // 0x00-0x07 and 0x78-0x7F are reserved
    Wire.beginTransmission(a);
    if (Wire.endTransmission() == 0) found[n++] = a;
  }
  return n;
}
