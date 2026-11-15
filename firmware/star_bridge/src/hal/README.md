# hal

Hardware access behind small interfaces, so swapping a part touches only this folder.

- `display`: init, clear, flush, contrast, dim. Wraps Adafruit SSD1306.
- `imu`: init (wake, +-4 g, 100 Hz, WHO_AM_I check), read acceleration (x, y, z in g), read gyro (deg/s). Wraps whichever accelerometer is fitted.
- `i2c_scan`: lists devices on the bus, used by identity.

Depends on: config.
