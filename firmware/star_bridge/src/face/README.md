# face

The robot's eyes and status screen.

- Parametric eye renderer: rounded rectangles plus lid cuts, no bitmaps.
- Face states as parameter sets: idle, searching, linked, startled, dizzy, weak, lost, sleep.
- Blink and gaze animation, including the synchronized blink when linked.
- Status screen: node name, peer count, RSSI. Toggled by a gesture.
- Design reference: the interactive eye preview from the design discussion.

Depends on: config, hal (display). Receives state changes from `events`.
