// Star-Bridge main sketch.
// Kept thin on purpose: each module owns its logic under src/.
// Modules get wired in here as they are written, in the order listed in docs/v0.1-plan.md.

#include "src/config/config.h"

void setup() {
  Serial.begin(115200);
  Serial.printf("\nStar-Bridge proto v%d, screen %dx%d\n", PROTO_VERSION, SCREEN_WIDTH, SCREEN_HEIGHT);
}

void loop() {
}
