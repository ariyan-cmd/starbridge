# config

Build-time settings in one place. No logic.

- `pins.h`: I2C pins and any sensor pins
- `config.h`: `SCREEN_WIDTH`, `SCREEN_HEIGHT` (64), `PROTO_VERSION`, `NODE_NAME_DEFAULT`, `SENSOR_CAPS`, beat interval, missed-beat limit, gesture thresholds
- `local_config.h` (git-ignored): per-node overrides such as the default node name and optional sensors

Depends on: nothing.
