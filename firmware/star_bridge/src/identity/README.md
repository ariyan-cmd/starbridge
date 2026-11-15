# identity

Who this node is and what it can do.

- Node ID from the chip ID or MAC.
- Node name: default from config, changeable from the console, persisted in flash.
- Capability manifest: bitmask built from the I2C scan (display, IMU) plus config lines for sensors that cannot be detected (temp, light).

Depends on: config, hal (i2c_scan).
