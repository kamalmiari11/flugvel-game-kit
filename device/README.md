# Running on the hardware

Only relevant if you were sent a unit. **The simulator is the development
environment** - it reproduces the device's timing, its colours and its
constraints, and every check runs there. Hardware is for the last mile: how it
feels in the hand, and whether the colours read the way they did on your
monitor.

## Getting PlatformIO

One of:

```sh
pip install platformio           # the CLI, which is all this needs
```

or the **PlatformIO IDE** extension for VS Code, which bundles it.

First build downloads the ESP32 toolchain and the Arduino core - a few
hundred megabytes, once. After that it is seconds.

```sh
cd device
pio run                  # build
pio run -t upload        # build and flash over USB
pio device monitor       # serial log, 115200
```

If `upload` cannot find the board: it is a USB cable that carries data (plenty
of charging cables do not), and on Linux your user needs to be in the
`dialout` group. `pio device list` shows what it can see.

## Before you build it for the first time

Run `make device-check` from the repository root. It compiles this harness
against stub headers in `hostcheck/` in about a second, with no toolchain, and
catches the API mistakes that are otherwise a five-minute download away from
being discovered. It runs as part of `make check`, so CI covers it too.

It is not a substitute for `pio run` - it checks that the code matches the
libraries' API, not that it links or runs.

The same game sources are compiled for both targets - `src_dir` points one
level up - so there is nothing to copy and nothing to keep in sync.

## What flashing this actually does

It replaces whatever was on the device with this harness, and nothing else.
The harness is not the product firmware: no Wi-Fi, no setup portal, no
screens, no updates. It boots straight into the start screen
(`games/welcome/`), the first entry in `games/registry.cpp`.

That screen is the hardware check: it draws a border around the exact area a
game may use, reports the knob and the button as you work them, beeps when
you press, shows every colour in the current theme, and - the number worth
looking at - how many milliseconds apart your game is actually being called.
If all of that looks right, the device, the build and the toolchain are
good.

On boot you get the backlight on, the panel in landscape, a status bar with
the game name and the current theme, the control legend along the bottom, and
the game running between them. Then:

| | |
| --- | --- |
| turn the knob | your game's `onKnob()` |
| press the button | your game's `onButton()` |
| press the knob in | next game in the registry |
| press the knob **and** the button together | sound on / off |

Best scores are kept in flash under their own namespace, one per game key, and
survive a reflash. The serial monitor prints which game started and its stored
best, and nothing else - the harness stays quiet so your own output is easy to
see.

Flashing the product firmware back over this restores the device completely;
the two builds use the same flash layout, so nothing has to be erased in
between.

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
