#include "imu.h"
#include <Wire.h>
#include "../config/config.h"

enum : uint8_t {
  REG_SMPLRT_DIV = 0x19, REG_CONFIG = 0x1A, REG_GYRO_CFG = 0x1B, REG_ACCEL_CFG = 0x1C,
  REG_ACCEL_OUT = 0x3B, REG_GYRO_OUT = 0x43, REG_PWR_MGMT_1 = 0x6B, REG_WHO_AM_I = 0x75,
};
static const float ACCEL_LSB_PER_G = 8192.0f;   // +-4 g range
static const float GYRO_LSB_PER_DPS = 65.5f;    // +-500 deg/s range

static uint8_t whoAmI;

static bool writeReg(uint8_t reg, uint8_t v) {
  Wire.beginTransmission(MPU_I2C_ADDR);
  Wire.write(reg); Wire.write(v);
  return Wire.endTransmission() == 0;
}

static bool readRegs(uint8_t reg, uint8_t *buf, uint8_t n) {
  Wire.beginTransmission(MPU_I2C_ADDR);
  Wire.write(reg);
  if (Wire.endTransmission(false) != 0) return false;
  if (Wire.requestFrom((uint8_t)MPU_I2C_ADDR, n) != n) return false;
  for (uint8_t i = 0; i < n; i++) buf[i] = Wire.read();
  return true;
}

static bool readXyz(uint8_t reg, float lsbPerUnit, float &x, float &y, float &z) {
  uint8_t b[6];
  if (!readRegs(reg, b, 6)) return false;
  x = (int16_t)((b[0] << 8) | b[1]) / lsbPerUnit;
  y = (int16_t)((b[2] << 8) | b[3]) / lsbPerUnit;
  z = (int16_t)((b[4] << 8) | b[5]) / lsbPerUnit;
  return true;
}

bool imuInit() {
  Wire.begin(PIN_I2C_SDA, PIN_I2C_SCL);
  whoAmI = 0;
  if (!readRegs(REG_WHO_AM_I, &whoAmI, 1) || whoAmI != 0x68) return false;  // 0x68 even when AD0 is high
  return writeReg(REG_PWR_MGMT_1, 0x01)    // wake, clock from gyro X PLL
      && writeReg(REG_CONFIG, 0x03)        // DLPF 44 Hz, base rate 1 kHz
      && writeReg(REG_SMPLRT_DIV, 9)       // 1 kHz / 10 = 100 Hz
      && writeReg(REG_ACCEL_CFG, 0x08)     // +-4 g
      && writeReg(REG_GYRO_CFG, 0x08);     // +-500 deg/s
}

uint8_t imuWhoAmI() { return whoAmI; }
bool imuRead(float &ax, float &ay, float &az) { return readXyz(REG_ACCEL_OUT, ACCEL_LSB_PER_G, ax, ay, az); }
bool imuReadGyro(float &gx, float &gy, float &gz) { return readXyz(REG_GYRO_OUT, GYRO_LSB_PER_DPS, gx, gy, gz); }
