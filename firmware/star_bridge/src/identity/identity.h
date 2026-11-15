#pragma once
// identity: who this node is (ID, name) and what it can do (caps for HELLO).
#include <stdint.h>

// Reads the chip ID, loads the saved name (or NODE_NAME_DEFAULT) and scans I2C for caps.
// Call once in setup(), after the display is up. Prints the scan result to Serial.
void identityInit();

uint32_t nodeId();          // ESP8266 chip ID, same value as Header::src_id.
const char *nodeName();     // NUL-terminated, 1..NODE_NAME_LEN chars.
uint16_t caps();            // CapBits: scanned display/IMU plus SENSOR_CAPS from config.

// Sets and saves the name to flash. Accepts 1..NODE_NAME_LEN printable ASCII chars.
// False (name unchanged) if invalid or the flash write fails.
bool setNodeName(const char *name);
