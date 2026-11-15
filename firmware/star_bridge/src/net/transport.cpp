#include "transport.h"

#include <ESP8266WiFi.h>
#include <espnow.h>

#include "../config/config.h"
#include "codec.h"
#include "frame_queue.h"

static FrameQueue<RX_QUEUE_LEN> rxQueue;
static PacketHandler handler;
static volatile uint32_t rxCount, sendOk, sendFail;
static uint32_t parseErr;
static uint8_t broadcastMac[6] = {0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF};

// ESP8266 signature (ESP32 differs). Copy and return: no parsing or printing here.
static void onRecv(uint8_t *mac, uint8_t *data, uint8_t len) {
  (void)mac;
  if (rxQueue.push(data, len)) rxCount = rxCount + 1;
}

static void onSent(uint8_t *mac, uint8_t status) {
  (void)mac;
  if (status == 0) sendOk = sendOk + 1;
  else sendFail = sendFail + 1;
}

bool transportInit(PacketHandler onPacket) {
  handler = onPacket;
  WiFi.persistent(false);  // do not join a saved AP: that would move us off ESPNOW_CHANNEL
  WiFi.mode(WIFI_STA);
  WiFi.disconnect();
  wifi_set_channel(ESPNOW_CHANNEL);
  if (esp_now_init() != 0) return false;
  esp_now_set_self_role(ESP_NOW_ROLE_COMBO);  // send and receive
  esp_now_register_recv_cb(onRecv);
  esp_now_register_send_cb(onSent);
  return esp_now_add_peer(broadcastMac, ESP_NOW_ROLE_COMBO, ESPNOW_CHANNEL, nullptr, 0) == 0;
}

bool transportSend(const void *frame, uint8_t len) {
  if (len == 0 || len > ESPNOW_MAX_FRAME ||
      esp_now_send(broadcastMac, (uint8_t *)frame, len) != 0) {
    sendFail = sendFail + 1;
    return false;
  }
  return true;
}

void transportPoll() {
  Packet p;
  while (const auto *s = rxQueue.front()) {
    ParseResult r = codecParse(s->data, s->len, p);
    rxQueue.pop();
    if (r != PARSE_OK) parseErr++;
    else if (handler) handler(p);
  }
}

TransportStats transportStats() {
  return {rxCount, rxQueue.dropped, parseErr, sendOk, sendFail, rxQueue.size()};
}
