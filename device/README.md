# Running on the hardware

Only relevant if you were sent a unit. **The simulator is the development
environment** - it reproduces the device's timing, its colours and its
constraints, and every check runs there. Hardware is for the last mile: how it
feels in the hand, and whether the colours read the way they did on your
monitor.

```sh
cd device
pio run -t upload        # build and flash
pio device monitor       # serial log
```

The same game sources are compiled for both targets - `src_dir` points one
level up - so there is nothing to copy and nothing to keep in sync.

On the device:

| Control | Does |
| --- | --- |
| Turn the knob | your game's `onKnob()` |
| Press the button | your game's `onButton()` |
| Press the knob in | next game in `games/registry.cpp` |

Best scores are kept in flash, one per game key, and survive a reflash.

If the panel stays dark, check `TFT_BL` and the SPI pins in `platformio.ini`
against your unit before suspecting anything else. If text draws as blank
boxes, a `LOAD_*` font flag went missing. If the colours look inverted, see
`include/gamekit/Colors.h` - that is the panel, not a bug, and the kit already
compensates for it.
