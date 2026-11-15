#include "board.h"
#include <Arduino.h>
#include <EEPROM.h>

uint32_t chipId() { return ESP.getChipId(); }

// Never EEPROM.end(): end() frees the shared buffer, and a smaller begin() would make
// commit() rewrite fewer bytes and wipe the rest of the sector.
static void open() {
  static bool opened;
  if (!opened) { EEPROM.begin(NV_SIZE); opened = true; }
}

bool nvRead(size_t off, void *buf, size_t n) {
  if (off > NV_SIZE || n > NV_SIZE - off) return false;
  open();
  memcpy(buf, EEPROM.getConstDataPtr() + off, n);
  return true;
}

bool nvWrite(size_t off, const void *buf, size_t n) {
  if (off > NV_SIZE || n > NV_SIZE - off) return false;
  open();
  const uint8_t *p = static_cast<const uint8_t *>(buf);
  for (size_t i = 0; i < n; i++) EEPROM.write(off + i, p[i]);  // write() only marks dirty on a change
  return EEPROM.commit();
}
