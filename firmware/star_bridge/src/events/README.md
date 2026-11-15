# events

The small bus that connects everything else.

- Local gesture -> EVENT packet broadcast.
- Incoming EVENT or peer change -> face reaction.
- Periodic TELEMETRY (temperature) out, peer readings in.
- Fixed-size queue, no heap allocation.

Depends on: gestures, face, net, peers.
