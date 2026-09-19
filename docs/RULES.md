# The rules

Fourteen rules. Each one exists because of something that goes wrong on this
hardware and nowhere else, so each is written with its reason - if a rule
ever stops making sense for your game, that is worth a conversation rather
than a workaround.

Every rule is checked. `make check` runs all of it.

---

### 1. Never block

No `delay()`, no `sleep()`, no waiting for anything, no loop that runs until
a condition changes.

Your `update()` is called from the device's one main loop. While you are
inside it, nothing else on the device happens: not the knob, not the button,
not the screen, not the background work. A 200 ms wait inside a game is 200 ms
of a frozen device.

*Pace with `host.millis()` and return.*
**Checked by:** `make lint`.

### 2. Never allocate

No `new`, no `malloc`, no `String`, no `std::vector`, `std::string`, or any
other container.

The heap is shared with the system and the network stack. It fragments, and
an allocation that fails mid-game is a reboot, not an exception you can
catch. Fixed-size members cost nothing and cannot fail.

*Arrays sized at compile time. `char buf[24]` and `snprintf` instead of
strings.*
**Checked by:** `make lint`, and `make test` counts every allocation during a
run and fails on any.

### 3. Redraw only what changed

Erase the box the sprite was in and draw it where it is now. Never clear the
play area in `update()`.

A full clear is 60,800 pixels, about 24 ms on the bus, during which the whole
device is stopped. Do it every frame and the knob answers a third of a second
late. Do it once, in `repaint()`, and nobody notices.

*Keep a second copy of "what is on screen" beside "what the state is" - that
difference is what tells you which pixels to take back. The reference game
shows the pattern.*
**Checked by:** `make test` measures the pixels drawn per frame. A typical
frame must stay under 12,000; 95% of frames under 25,000.

### 4. Draw the HUD last, and unconditionally

Anything inside the playfield that other objects move through - a score, a
badge, a timer - gets painted over by them. If you only redraw it "when it
changes", it develops holes and never recovers.

*Redraw it every frame, last. A score plate is about 300 px; it is not the
expensive thing.*
**Checked by:** `make test` wipes the screen, calls `repaint()`, and compares
- a HUD with holes in it shows up as a mismatch.

### 5. Stay inside `host.play()`, and ask for it every time

The strips above and below the play area belong to the device and are
repainted over you.

`host.play()` returns your rectangle. Do not hardcode its numbers: the
firmware's layout has changed before and will again, and a hardcoded 20 is a
game drawing under the legend on the day it does.

**Checked by:** `make test` fails any draw that lands on screen outside the
rectangle, and runs your game a second time against a different layout.

### 6. `repaint()` must rebuild the screen from state

It is called after `start()`, on a theme change, and any time something has
painted over you. It must leave the screen showing exactly what `update()`
would have left, without changing any state.

*If you cannot draw your screen from your member variables alone, your state
is incomplete - fix that, it will bite you elsewhere too.*
**Checked by:** `make test` runs your game, wipes the play area, calls
`repaint()`, and requires a pixel-exact match.

### 7. Input handlers do not draw

`onKnob()` and `onButton()` record what happened. `update()` paints it. They
are not given a `Gfx` for exactly this reason.

A fast knob spin delivers several detents in one pass; drawing from the
handler paints the same object five times in one go, which is the stall you
were trying to avoid by being responsive.

*If an input needs a full repaint - a restart, say - set a flag and let the
next `update()` act on it.*

### 8. Pace off `host.millis()`, never off frame counts

You are called every 50-90 ms, irregularly. A counter that increments per
call is a clock that speeds up and slows down with whatever else the device
is doing.

And when a long gap happens, run **one** frame, not six. Catching up
multiplies the drawing in a single pass over the bus.

**Checked by:** `make lint` bans bare `millis()`/`micros()`.

### 9. Randomness comes from `host.random(n)`

Not `rand()`, not a clock, not anything else.

The host's generator is seeded and reproducible, which is what makes a
recorded input script replay identically - and that is the difference between
a bug report you can act on and "it sometimes crashes".

**Checked by:** `make lint`, and `make test` runs each game twice and requires
identical pixels.

### 10. Colours come from `host.theme()`

By meaning, not by appearance: `accent` is "watch out", `danger` is "you
lost", `fgDim` is supporting text. The user can switch themes mid-run.

A raw 16-bit colour is almost certainly wrong anyway - see the panel note in
[HARDWARE.md](HARDWARE.md). If you genuinely need your own colour, use
`PANEL_HEX(0xrrggbb)`.

**Checked by:** `make lint` flags raw colour-shaped constants for a human to
look at.

### 11. Store the score once, at the end of a run

`host.submitScore(n)` writes to flash. It stalls the device for milliseconds
and the part has a finite number of writes in it. Once per run, when the run
is over.

**Checked by:** write the test for your own game - the reference game has one.

### 12. Keep the legend honest

`host.setLegend(knob, button)` is how the player knows what the controls do.
Update it whenever the answer changes - playing, paused, game over - and keep
each side under about 22 characters.

*A game whose legend says "BOOST" on the game-over screen is a bug report
waiting to happen.*

### 13. Sound is fired on events, never on frames

`host.playTone()` queues a note. The buzzer plays one at a time and the host
holds eight behind it; a beep per frame overruns that within a second and the
tail is silently dropped, which is how you end up with feedback that works
sometimes and cannot be debugged by ear.

*A point scored, a life lost, a run ending. Not "while boosting".*
**Checked by:** `make test` fails a run that drops a note.

### 14. The game must play identically with the sound off

Players mute things. Audio is feedback for something the screen is already
showing - never the only place a piece of information appears, and never
something the game's own state depends on.

**Checked by:** `make test` runs each game twice with the same seed and the
same input, once muted, and requires the two screens to match pixel for
pixel. `./build/sim --mute` is the same thing by hand.

---

## The device talks to you, not the other way round

Everything you may touch is on `gk::Host` and `gk::Gfx`. If you find yourself
reaching for `Serial`, `WiFi`, `EEPROM`, `Preferences`, the display driver,
`tone()`, a GPIO pin, or `Arduino.h`, stop - either it is on the Host under a different name, or it is
deliberately not available. Ask.

**Checked by:** `make lint`.
