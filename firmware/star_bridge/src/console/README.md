# console

Serial command line for debugging a distributed system.

- Commands: `peers`, `caps`, `set name <text>`, `send <event>`, `help`.
- Non-blocking line reader called from `loop()`.
- Talks to other modules only through their public headers.

Depends on: identity, peers, events.
