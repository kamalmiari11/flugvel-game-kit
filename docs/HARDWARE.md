# The device

A small desk gadget. For your purposes it is four things: a screen, a knob, a
button and a buzzer, driven by a microcontroller that is doing other work at
the same time.

Read this before designing anything. Several of the constraints below rule
out whole categories of game, and it is much cheaper to find that out now.

## The screen

| | |
| --- | --- |
| Size | **320 x 240**, landscape |
| Colour | RGB565, 16 bits per pixel |
| Connection | SPI at 40 MHz |
| Backlight | on, fixed, from your point of view |

There is **no framebuffer**. Every drawing call goes immediately down the
wire to the panel, and the processor waits while it does. That single fact
drives most of the rules:

* the bus moves about **2,500 pixels per millisecond**
* filling the whole play area (320 x 190) is **60,800 pixels ≈ 24 ms**
* during those 24 ms nothing else happens - the knob is not read, the button
  is not read, the rest of the device is stopped

So a game that clears and redraws the screen every frame does not just run
slowly, it makes the whole device unresponsive. The technique that works is
to repaint only the pixels that changed: erase the 12x12 box the sprite was
in, not the area it was somewhere inside. The reference game does this and
costs about 2,000 pixels - under a millisecond - in a typical frame.

### Your part of the screen

```
 y=0    +--------------------------------------+
        |  status bar - not yours              |  20 px
 y=20   +--------------------------------------+
        |                                      |
        |   your play area: 320 x 190          |
        |                                      |
 y=210  +--------------------------------------+
        |  control legend - not yours          |  30 px
 y=240  +--------------------------------------+
```

The device repaints both strips over whatever is in them, so anything you
draw up there gets sliced in half a frame later. **Ask for the rectangle
rather than hardcoding it**: `host.play()` returns it, and the test bench
runs your game against a deliberately different layout to check that you did.

The bottom strip is how the player knows what the controls do. You do not
draw it - you tell the host what to put there with `host.setLegend()`, and
you update it whenever the answer changes.

### Colours are inverted, and red and blue are swapped

The panel is a clone part that displays `swapRB(~value)`. Send it "red" and
it shows cyan. Nothing in the driver corrects this, so every colour is
pre-distorted on the way out.

You mostly do not need to care, because **colours come from the theme**
(`host.theme()`) and those are already correct. If you ever do need a colour
of your own, write the colour you want to *see* and wrap it:

```cpp
gfx.fillRect(x, y, w, h, PANEL_HEX(0xff8800));   // shows orange
gfx.fillRect(x, y, w, h, 0xfc00);                // shows who knows what
```

The simulator applies the same transform when it renders, so what you see on
your monitor is what the glass shows. A raw 16-bit colour constant in game
code is almost always a bug; the lint flags them.

### Themes

The user can change theme at any time, including while your game is running.
Take every colour from `host.theme()` by what it *means* - `bg`, `fg`,
`fgDim`, `accent` (watch out), `accent2` (good), `danger` (lost) - and your
game works on all of them. Hardcode "green" and it vanishes on one.

A theme change arrives as a call to your `repaint()`. That is the whole
reason that method exists.

### Text

One font, in one size family: a 5x7 glyph in a 6x8 cell, scaled by an integer.
Size 1 is 6x8 px per character, size 2 is 12x16, size 3 is 18x24. No other
font is compiled into the firmware and adding one costs flash the device does
not have. Plan your layout in multiples of 6 and 8.

## The controls

**The knob** turns in detents - physical clicks. One click gives you exactly
one `onKnob(host, +1)` or `onKnob(host, -1)`. It is smooth, it has no limit,
and it is the precise control: use it for anything that needs aiming.

**The button** is one momentary switch. It is debounced at **200 ms**, so
presses arrive at most five times a second no matter how fast the player
taps. Do not design a game that needs faster tapping - it is not slow
software, it is the contact bounce filter, and without it one press
registers as four.

There is a second button (pressing the knob in) but **it is not yours**: on
the device it moves between screens. Your game gets the knob's rotation and
the one button. That is the whole input surface.

A convention worth keeping: on a game-over screen, the button restarts and
**turning the knob leaves the game**. The button is busy with "play again",
so the knob has to be the way out, or there is no way out.

## The buzzer

A passive buzzer on one pin. It is a beeper, not a speaker: a square wave at
whatever frequency you ask for, no volume control, no samples, no polyphony,
one note at a time.

You never touch the pin. `host.playTone(freqHz, ms)` queues a note and
returns immediately - the host plays it from its own loop, so sound never
costs you a frame and never blocks. A frequency of `0` is a rest, which is
how you space the notes of a jingle:

```cpp
host.playTone(1200, 60);   // blip
host.playTone(0,    40);   // gap
host.playTone(1800, 80);   // higher blip
```

What to keep in mind:

* **Fire on events, not on frames.** A point scored, a life lost, a run
  ending. The host holds eight notes and drops the rest, and the test bench
  fails a run that overruns it.
* **Keep notes short.** 40-120 ms reads as feedback; anything approaching a
  second reads as a fault. One note may not exceed two seconds.
* **Useful range is roughly 100 Hz to 5 kHz**, and a buzzer this size is
  loudest around 2.7 kHz - worth knowing for the beep that has to be heard
  across a room. Values outside the range are clamped, not rejected.
* **`host.stopSound()`** cuts the current note and everything queued behind
  it, for a run ending mid-jingle.
* **The player can mute the device**, and plenty do. Sound is feedback for
  something the screen is already showing - never the only place information
  appears. The test bench runs your game twice, once muted, and requires the
  pixels to match.

Pitch carries meaning better than volume can here: rising means good, falling
means bad, and two games' worth of experience says players read that
instantly without being told.

## Timing

This is the constraint people get wrong, because on a desktop it is
invisible.

Your `update()` is **not** called at a frame rate. It is called from a shared
main loop that ends in a fixed 50 ms delay and does other work in between -
so in practice you are called every **50 to 90 ms**, irregularly, and
occasionally much later than that when something else takes a while.

That means:

* **Never count frames.** `_tick++` as a clock produces a game that runs at a
  different speed depending on what the rest of the device is doing.
* **Pace off `host.millis()`.** Decide your own frame interval, check whether
  it has elapsed, and return immediately if it has not. The reference game
  does this in four lines.
* **Do not try to catch up.** If 300 ms passed, running six frames back to
  back doubles or triples the drawing in one pass over the bus, which is
  exactly the stall you were avoiding. Run one frame and carry on.
* **A realistic ceiling is 20 frames per second**, and 12-15 is normal. Design
  movement that reads well at that rate: bigger steps, fewer objects, no
  smooth 60 fps easing.

The simulator reproduces all of this, jitter included. `--loop-ms` and
`--jitter` let you see what happens on a busier device; they do not let you
make the real one faster.

## Memory

The chip has around 320 KB of RAM, and most of it is already spoken for by
the system, the network stack and its buffers. Flash is tight enough that the
firmware compiles in exactly one font.

* **A game gets a budget of about 4 KB of state.** Keep it in fixed-size
  members.
* **No heap. At all.** No `new`, no `malloc`, no `String`, no `std::vector`.
  The heap is shared with the network stack, it fragments, and a failed
  allocation mid-game is a reboot rather than an exception. The test bench
  counts allocations and fails on any.
* **No doubles.** The chip has no hardware double; `float` is fine and `double`
  quietly costs ten times as much.
* Your game object is constructed once at boot and lives forever. `start()`
  resets it for a new run; it is not a constructor.

## Storage

One number per game: the player's best score. Read it with
`host.bestScore()`, write it with `host.submitScore(n)` **once, when a run
ends**. It goes to flash, which stalls the device for milliseconds and wears
the part out if you do it per point.

There is nothing else. No save files, no settings of your own, no assets
loaded at runtime - anything your game needs must be compiled into it.

## What you cannot do

No network, no images loaded from anywhere, no filesystem, no threads, no
exceptions, no RTTI, no standard library containers. Sprites are 1-bit
bitmaps compiled into the source, or shapes drawn with rectangles and
circles. Audio is the buzzer described above and nothing more - no samples,
no music, no more than one note at a time.

That is a tighter budget than it sounds, and it is also why a good game for
this device looks nothing like a scaled-down phone game. Simple shapes, high
contrast, big movements, one clear thing to do with a knob and a button.
