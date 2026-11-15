// Host-side test for net/frame_queue.h (the transport receive ring).
// Run: g++ -std=c++11 -Wall -Wextra -I firmware/star_bridge/src/net test/frame_queue_test.cpp -o /tmp/fq_test && /tmp/fq_test
#include <stdio.h>

#include "frame_queue.h"

static int fails;
#define CHECK(c) do { if (!(c)) { printf("FAIL %s:%d: %s\n", __FILE__, __LINE__, #c); fails++; } } while (0)

int main() {
  FrameQueue<4> q;
  uint8_t f[ESPNOW_MAX_FRAME];
  CHECK(q.front() == nullptr && q.size() == 0);

  // rejects bad lengths
  CHECK(!q.push(f, 0) && q.dropped == 1);

  // FIFO order, full queue drops the newest frame
  for (uint8_t i = 1; i <= 4; i++) { f[0] = i; CHECK(q.push(f, i)); }
  f[0] = 5;
  CHECK(!q.push(f, 5) && q.dropped == 2 && q.size() == 4);
  for (uint8_t i = 1; i <= 4; i++) {
    const auto *s = q.front();
    CHECK(s && s->len == i && s->data[0] == i);
    q.pop();
  }
  CHECK(q.front() == nullptr);
  q.pop();  // pop on empty is a no-op
  CHECK(q.size() == 0);

  // counters wrap past 255 and a full-size frame round-trips
  for (int i = 0; i < 300; i++) {
    f[0] = uint8_t(i); f[ESPNOW_MAX_FRAME - 1] = uint8_t(~i);
    CHECK(q.push(f, ESPNOW_MAX_FRAME));
    const auto *s = q.front();
    CHECK(s && s->len == ESPNOW_MAX_FRAME && s->data[0] == uint8_t(i) && s->data[ESPNOW_MAX_FRAME - 1] == uint8_t(~i));
    q.pop();
  }
  CHECK(q.dropped == 2);

  printf(fails ? "%d FAILED\n" : "all passed\n", fails);
  return fails != 0;
}
