# Hardware: DestroBot v0.1 node

| Part | Detail |
|------|--------|
| MCU | NodeMCU, ESP8266MOD |
| Display | 0.96" SSD1306, 128x64, single-colour blue, I2C, address 0x3C |
| Accelerometer | MPU6050 (accel + gyro), I2C 0x68 (0x69 if AD0 is high) |

## Pins (NodeMCU)

| Signal | Label | GPIO |
|--------|-------|------|
| I2C SDA | D2 | 4 |
| I2C SCL | D1 | 5 |

Display and accelerometer share the same I2C bus (different addresses).

## Verified

- MPU6050 answers at 0x68 (AD0 low), WHO_AM_I = 0x68, shares the bus with the OLED at 0x3C (2026-11-15, I2C scan).
- Display bring-up test passes at 128x64 (`firmware/bringup/oled_test`).
- I2C clock: 400 kHz. Faster clocks untested in the final build.

## To fill in

- Whether any analog or digital sensors are fitted (DHT, LDR) and on which pins
- Power source and battery sensing
