# peers

Who else is out there.

- HELLO on boot, BEAT every few seconds.
- Peer table: id, name, capabilities, last seen, RSSI.
- Timeout: drop a peer after N missed beats and tell `events` so the face can react.

Depends on: config, identity, net.
