#pragma once
// hal/imu: the only place that touches the MPU6050. Acceleration in g, gyro in deg/s.
#include <stdint.h>

bool imuInit();                                 // Wake, set +-4 g / 100 Hz. False if absent or WHO_AM_I is not 0x68.
uint8_t imuWhoAmI();                            // Last WHO_AM_I value read (0 if the chip did not answer). For diagnostics.
bool imuRead(float &ax, float &ay, float &az);  // g. False on I2C failure (outputs untouched).
bool imuReadGyro(float &gx, float &gy, float &gz);  // deg/s (+-500 range). False on I2C failure.
