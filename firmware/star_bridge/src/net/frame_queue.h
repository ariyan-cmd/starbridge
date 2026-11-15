// Fixed ring of raw frames: one producer (radio receive callback), one consumer (loop). No heap,
// no Arduino calls (host-testable). When full, the new frame is dropped and counted.
#pragma once

#include <stdint.h>
#include <string.h>
#include "protocol.h"

template <uint8_t N>
struct FrameQueue {
  static_assert(N > 0 && N <= 128 && (N & (N - 1)) == 0, "N must be a power of two, at most 128");
  struct Slot {
    uint8_t len;
    uint8_t data[ESPNOW_MAX_FRAME];
  };

  Slot slot[N];
  volatile uint8_t head = 0;      // free-running, written only by push
  volatile uint8_t tail = 0;      // free-running, written only by pop
  volatile uint32_t dropped = 0;  // frames refused: queue full or len out of range

  // Producer. Copies the frame; false (and dropped++) if full or len is 0 or over ESPNOW_MAX_FRAME.
  bool push(const uint8_t *d, uint8_t len) {
    uint8_t h = head;
    if (len == 0 || len > ESPNOW_MAX_FRAME || uint8_t(h - tail) >= N) {
      dropped = dropped + 1;
      return false;
    }
    Slot &s = slot[h & (N - 1)];
    s.len = len;
    memcpy(s.data, d, len);
    asm volatile("" ::: "memory");  // slot contents must be written before head publishes it
    head = h + 1;
    return true;
  }

  // Consumer. Oldest frame or nullptr; it stays valid until pop().
  const Slot *front() const { return head == tail ? nullptr : &slot[tail & (N - 1)]; }
  void pop() { if (head != tail) tail = tail + 1; }
  uint8_t size() const { return uint8_t(head - tail); }
};
