# hal

Hardware access behind small interfaces, so swapping a part touches only this folder.

- `display`: init, clear, flush, contrast, dim, raw framebuffer (for dither fades). Wraps Adafruit SSD1306.
- `imu`: init (wake, +-4 g, 100 Hz, WHO_AM_I check), read acceleration (x, y, z in g), read gyro (deg/s). Wraps whichever accelerometer is fitted.
- `board`: chip ID and a 64-byte NV store (emulated EEPROM, opened once, owners listed in `board.h`).
- `i2c_scan`: lists devices that ACK on 0x08..0x77, used by identity.

Depends on: config.
