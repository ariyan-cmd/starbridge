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

- Display bring-up test passes at 128x64 (`firmware/bringup/oled_test`).
- I2C clock: 400 kHz. Faster clocks untested in the final build.

## To fill in

- Confirm the MPU6050 address: the sketch prints every I2C device at boot (0x68 expected, 0x69 if AD0 is high; change `MPU_I2C_ADDR` in `local_config.h`). Result pending hardware run
- Whether any analog or digital sensors are fitted (DHT, LDR) and on which pins
- Power source and battery sensing
