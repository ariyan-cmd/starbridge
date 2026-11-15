#include "identity.h"
#include <Arduino.h>
#include <string.h>
#include "../config/config.h"
#include "../net/protocol.h"
#include "../hal/board.h"
#include "../hal/i2c_scan.h"

static_assert(sizeof(NODE_NAME_DEFAULT) - 1 >= 1 && sizeof(NODE_NAME_DEFAULT) - 1 <= NODE_NAME_LEN,
              "NODE_NAME_DEFAULT must be 1..16 chars");

// Saved at hal/board NV offset 0 (identity owns 0..17).
struct __attribute__((packed)) NameRecord {
  uint8_t magic;                  // NAME_MAGIC once a name was saved; erased flash reads 0xFF
  char name[NODE_NAME_LEN + 1];   // NUL-terminated
};
static const uint8_t NAME_MAGIC = 0xB5;
static const size_t NV_OFF_NAME = 0;
static_assert(NV_OFF_NAME + sizeof(NameRecord) <= NV_SIZE, "name record must fit the NV store");

static uint32_t id;
static uint16_t capMask;
static char name[NODE_NAME_LEN + 1];

static bool validName(const char *s) {
  if (!s) return false;
  size_t n = strnlen(s, NODE_NAME_LEN + 1);
  if (n < 1 || n > NODE_NAME_LEN) return false;
  for (size_t i = 0; i < n; i++) if (s[i] < 0x20 || s[i] > 0x7E) return false;
  return true;
}

void identityInit() {
  id = chipId();

  NameRecord r;
  bool ok = nvRead(NV_OFF_NAME, &r, sizeof(r)) && r.magic == NAME_MAGIC;
  r.name[NODE_NAME_LEN] = '\0';  // a corrupt record can never overrun
  strcpy(name, ok && validName(r.name) ? r.name : NODE_NAME_DEFAULT);

  uint8_t found[0x78 - 0x08];  // room for every probed address, so 0x3C/0x68 are always checked
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
  NameRecord r = {NAME_MAGIC, {}};
  strcpy(r.name, s);
  if (!nvWrite(NV_OFF_NAME, &r, sizeof(r))) return false;
  strcpy(name, s);
  return true;
}
