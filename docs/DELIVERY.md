# What "done" means

## The checklist

A game is finished when all of these are true. They are the same checks the
review runs, so there should be no surprises.

**It builds and passes, from a clean checkout**

- [ ] `make check` is green - lint, the device API check, the test bench, and
      a strict replay of every game in the registry
- [ ] no compiler warnings (the build uses `-Wall -Wextra -Wshadow`)
- [ ] the game is registered in `games/registry.cpp` and appears in
      `./build/sim --list`

**It behaves on the hardware**

- [ ] a typical frame stays under 12,000 pixels; the only frames over 25,000
      are full repaints
- [ ] nothing is ever drawn outside `host.play()`
- [ ] `repaint()` rebuilds the screen pixel-for-pixel from state
- [ ] pressing `t` in the simulator mid-run leaves the game looking correct
      in both themes
- [ ] it still plays correctly at `--jitter 150`, not just at the default
- [ ] nothing is allocated at any point during a run
- [ ] no notes are dropped - sound is fired on events, not on frames
- [ ] it plays identically at `./build/sim --mute`, and still makes sense

**It is a finished game, not a demo**

- [ ] the run ends, and ending it is clear
- [ ] the score is stored exactly once per run
- [ ] the best score is shown, and beating it is acknowledged
- [ ] the legend is correct in every state - playing, game over, anything else
- [ ] the buzzer says something useful: scoring, losing and a new best should
      be distinguishable with your eyes shut, and none of it should be
      annoying on the twentieth run
- [ ] turning the knob on the game-over screen leaves the game
      (`host.exitGame()`), and the button restarts it
- [ ] the difficulty goes somewhere: a player who is good at it should be
      pushed, a player who is not should still finish a run

**It can be maintained by someone else**

- [ ] all state in fixed-size members, no magic numbers without a named
      constant
- [ ] tests for what the game itself does, in `tests/`, in the style of
      `tests/gaterun.cpp` - not just the conformance suite it gets for free
- [ ] comments explain *why* where the code is doing something non-obvious,
      particularly anything timing- or redraw-related
- [ ] a recorded input script in `scripts/` that plays a representative run

## What to hand over

A branch or a patch containing:

```
games/<yourgame>/      the game
games/registry.cpp     one line added
tests/<yourgame>.cpp   your tests
scripts/<yourgame>.input   a recorded run
```

Please do not change `include/gamekit/`, `sim/`, `tests/conformance.cpp`,
`tests/noalloc.cpp`, `tests/selfcheck.cpp` or the `Makefile`. If a change
there seems necessary, that is a conversation - it usually means a rule needs
discussing, and that is a fair thing to raise.

Include a short note covering:

* how the game plays, in a paragraph - the pitch, not the mechanics list
* the numbers you tuned and what they do (speeds, intervals, difficulty
  ramp), so they can be adjusted later without re-deriving them
* anything you were unsure about, or that you would do differently with more
  time
* anything you could not check because you did not have the hardware

## How it gets reviewed

1. `make check` on a clean checkout of your branch.
2. A play session in the simulator, both themes, and at raised jitter.
3. A read of the source against [RULES.md](RULES.md).
4. If hardware is available: flashed and played.

The common reasons work comes back are always the same three, and all three
are visible in the simulator before you send it: a frame budget blown during
play, a `repaint()` that does not rebuild the screen, and a game tuned
against a desktop frame rate that falls apart at 15 fps.
