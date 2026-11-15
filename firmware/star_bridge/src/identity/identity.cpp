#include "identity.h"
#include <Arduino.h>
#include <EEPROM.h>
#include <string.h>
#include "../config/config.h"
#include "../net/protocol.h"
#include "../hal/i2c_scan.h"

static_assert(sizeof(NODE_NAME_DEFAULT) - 1 >= 1 && sizeof(NODE_NAME_DEFAULT) - 1 <= NODE_NAME_LEN,
              "NODE_NAME_DEFAULT must be 1..16 chars");

// EEPROM layout (emulated in one 4 KB flash sector): magic byte at 0, then the NUL-terminated name.
static const uint8_t NAME_MAGIC = 0xB5;  // anything but 0xFF (erased flash) and 0x00
static const int EEPROM_SIZE = 1 + NODE_NAME_LEN + 1;

static uint32_t id;
static uint16_t capMask;
static char name[NODE_NAME_LEN + 1];

static bool validName(const char *s) {
  size_t n = strnlen(s, NODE_NAME_LEN + 1);
  if (n < 1 || n > NODE_NAME_LEN) return false;
  for (size_t i = 0; i < n; i++) if (s[i] < 0x20 || s[i] > 0x7E) return false;
  return true;
}

void identityInit() {
  id = ESP.getChipId();

  char saved[NODE_NAME_LEN + 1];
  EEPROM.begin(EEPROM_SIZE);
  bool ok = EEPROM.read(0) == NAME_MAGIC;
  for (int i = 0; i <= NODE_NAME_LEN; i++) saved[i] = EEPROM.read(1 + i);
  EEPROM.end();
  saved[NODE_NAME_LEN] = '\0';  // a corrupt record can never overrun
  strcpy(name, ok && validName(saved) ? saved : NODE_NAME_DEFAULT);

  uint8_t found[16];
  uint8_t n = i2cScan(found, sizeof(found));
  capMask = SENSOR_CAPS & (CAP_TEMP | CAP_LIGHT);  // only these cannot be detected
  Serial.print(F("I2C scan:"));
  for (uint8_t i = 0; i < n; i++) {
    Serial.printf(" 0x%02X", found[i]);
    if (found[i] == OLED_I2C_ADDR) capMask |= CAP_DISPLAY;
    if (found[i] == MPU_I2C_ADDR) capMask |= CAP_IMU;
  }
  Serial.println(n ? "" : " none");
}

uint32_t nodeId() { return id; }
const char *nodeName() { return name; }
uint16_t caps() { return capMask; }

bool setNodeName(const char *s) {
  if (!validName(s)) return false;
  EEPROM.begin(EEPROM_SIZE);
  EEPROM.write(0, NAME_MAGIC);
  size_t n = strlen(s);
  for (int i = 0; i <= NODE_NAME_LEN; i++) EEPROM.write(1 + i, i < (int)n ? s[i] : 0);
  bool ok = EEPROM.end();  // end() commits, then frees the RAM copy
  if (ok) strcpy(name, s);
  return ok;
}
