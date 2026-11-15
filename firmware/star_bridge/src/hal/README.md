# hal

Hardware access behind small interfaces, so swapping a part touches only this folder.

- `display`: init, clear, flush, contrast, dim. Wraps Adafruit SSD1306.
- `imu`: init, read acceleration (x, y, z in g). Wraps whichever accelerometer is fitted.
- `i2c_scan`: lists devices on the bus, used by identity.

Depends on: config.
