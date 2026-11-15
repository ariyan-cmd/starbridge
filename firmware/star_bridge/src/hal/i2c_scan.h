#pragma once
// hal/i2c_scan: lists the devices that ACK on the shared I2C bus. Used by identity.
#include <stdint.h>

// Probes 0x08..0x77 and writes up to max responding addresses into found. Returns the count stored.
uint8_t i2cScan(uint8_t *found, uint8_t max);
