# Work split for v0.1

Two people, one contract. The line between the halves is the wire format in `docs/protocol.md` and the module headers, so both sides can work in parallel.

- **Ariyan: device side.** Display, face, MPU6050, gestures.
- **Saad: network side.** Protocol, ESP-NOW, identity, peers, events, console, integration test.

Issues were created on 2026-11-15 (8 each). Numbers below are the GitHub issue numbers.

| # | Issue | Owner | Depends on |
|---|-------|-------|------------|
| 1 | config: pins, build settings, local override | Ariyan | none |
| 2 | net: protocol.h | Saad | 1 |
| 3 | hal: display wrapper | Ariyan | 1 |
| 4 | face: parametric eye renderer | Ariyan | 3 |
| 5 | net: codec with host-side test | Saad | 2 |
| 6 | net: ESP-NOW transport (ESP8266) | Saad | 2 |
| 7 | hal: MPU6050 driver | Ariyan | 1 |
| 8 | gestures | Ariyan | 7 |
| 9 | identity | Saad | 1, 2 |
| 10 | peers: HELLO, BEAT, table, timeout | Saad | 5, 6, 9 |
| 11 | face: state machine, blink, synchronized blink | Ariyan | 4, 8 |
| 12 | events: bus and telemetry | Saad | 5, 10 |
| 13 | face: status screen | Ariyan | 11, 10 |
| 14 | console | Saad | 9, 10, 12 |
| 15 | tuning: fps and numbers | Ariyan | 4, 8 |
| 16 | integration: two-node test and shake demo | Saad | 6, 8, 10, 11, 12 |

## Where the halves meet

- **#8 and #12** agree the event codes (what a gesture is called on the wire).
- **#11 and #16** meet at the synchronized-blink hook.
- **#10 and #13** meet at the peer count and RSSI shown on the status screen.

## Suggested order

1. Day 1: #1 (Ariyan, small) unblocks everything. Saad starts #2 with stubbed constants.
2. In parallel: Ariyan #3, #4, #7. Saad #5, #6, #9.
3. Then Ariyan #8, #11. Saad #10, #12.
4. Then Ariyan #13, #15. Saad #14.
5. Finish together on #16.

## Open

- Saad's GitHub username is needed for the script (`SAAD_LOGIN`).
- The split is by interface, not by known strengths. Swap any issues that suit the other person better and update this table.
