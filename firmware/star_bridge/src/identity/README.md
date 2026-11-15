# identity

Who this node is and what it can do.

- Node ID from the chip ID (`ESP.getChipId()`), same as `src_id`.
- Node name: saved name, else `NODE_NAME_DEFAULT` from local_config, else `starbridge-<chip ID hex>`; changeable from the console, saved via `hal/board` NV store (offset 0..17).
- Capability manifest: bitmask built from the I2C scan (display, IMU) plus config lines for sensors that cannot be detected (temp, light).

Depends on: config, net/protocol.h (CapBits), hal (board, i2c_scan).
