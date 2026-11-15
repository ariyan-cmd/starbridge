# Star-Bridge: instructions for Claude

Star-Bridge is a network system that lets small robots find each other, share events, and show it on their faces. First node: DestroBot. Current target: v0.1, see `docs/v0.1-plan.md`.

## Standing decisions

1. **Commit regularly with brief descriptions.** Make small commits as work lands: one module, one fix, or one doc change per commit. Do not batch a session into one commit. Message format: one imperative line, 72 characters or fewer (for example `Add face renderer with lid cuts`). Add a body only when the why is not obvious from the diff.
2. **`task_report.md` is the one stop for why and what it cost.** Keep it updated in the same commit as the change it records. It stores:
   - decisions, with the alternatives considered and what each choice cost
   - deviations from the plan
   - numbers: measurements, thresholds, timings, memory, RSSI, fps
   - debug findings: symptom, cause, fix, and the values seen
3. **Store debug numbers and values.** When a test or debug session produces a number (fps at a given I2C clock, free heap, a gesture threshold that worked, RSSI at a distance), write it in `task_report.md` with the date, the setup, and the value. A number that is not written down did not happen. Never invent a number; mark unmeasured items as pending.

## Hardware (confirmed)

- MCU: NodeMCU with ESP8266MOD (use the ESP8266 Arduino core and its ESP-NOW API, not the ESP32 one)
- Display: SSD1306, 128x64, single-colour blue, I2C 0x3C
- Accelerometer: MPU6050, I2C 0x68 (0x69 if AD0 is pulled high)
- I2C: SDA = D2 (GPIO4), SCL = D1 (GPIO5), 400 kHz

## Code layout rules

See `README.md`. In short: the sketch is `firmware/star_bridge/`, modules live in `src/<module>/`, modules talk only through their headers, hardware access stays in `hal/`, and any wire-format change bumps `PROTO_VERSION` and updates `docs/protocol.md`.
