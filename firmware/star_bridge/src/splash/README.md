# splash

Boot animation, about 5 s: warp starfield, a 4-point star that swipes up, "STARBRIDGE" and the version fading in, then a dithered fade-out.

- `splashRun(work)` blocks until the splash ends. It calls `work(nowMs)` once per frame so boot init runs behind the animation, one step per call.
- If `work` has not returned true by the fade-out, the splash holds on its last scene (star pulsing, sparkles twinkling) until it does. If everything is ready in time, no extra time is added.

Depends on: config, hal (display).
