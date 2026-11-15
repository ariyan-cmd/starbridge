#pragma once
// hal/board: ESP8266 chip services. Chip ID and a small non-volatile store (emulated EEPROM).
#include <stddef.h>
#include <stdint.h>

uint32_t chipId();  // ESP.getChipId(): low 24 bits of the MAC.

// Non-volatile store, NV_SIZE bytes, kept open for the whole run so every owner shares one image.
// A write erases and rewrites the whole flash sector (all NV_SIZE bytes are kept), so write rarely.
// Offsets: 0..17 identity. Add new owners after the last range and list them here.
constexpr size_t NV_SIZE = 64;
bool nvRead(size_t off, void *buf, size_t n);         // False if off + n > NV_SIZE.
bool nvWrite(size_t off, const void *buf, size_t n);  // Writes and commits. False if out of range or flash fails.
