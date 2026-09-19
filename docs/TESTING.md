# Testing

Three commands. `make check` runs all of them and is what CI runs, so if it
is green locally it is green there.

```sh
make run      # play it
make test     # the test bench
make lint     # the house rules, checked against your source
make check    # lint + test + a strict scripted replay of every game
```

## Playing it

```sh
make run                 # the first game in the registry
make run GAME=gaterun    # a specific one
make run PORT=9000       # if 8080 is taken
```

Open <http://127.0.0.1:8080/>. The page is the device, pixel for pixel, with
the panel's colour transform applied - what you see is what the glass shows.

| Key | |
| --- | --- |
| up / down arrow, or `.` / `,` | one detent of the knob |
| space | the button |
| `r` | restart the run |
| `t` | switch theme - a good habit, it exercises `repaint()` |
| `m` | mute - the game must still play the same |
| `-` / `=` | zoom |

One key press is exactly one detent, the way the real encoder reports it.
Holding a key repeats at your keyboard's rate, which is faster than a human
can turn a knob - do not tune difficulty against it.

The right-hand panel is the part to watch:

* **pixels this frame / bus time** - how long that frame cost the device.
  Green is comfortable, amber is noticeable, red means the knob is lagging.
* **worst frame so far** - a single red spike at the start is your
  `repaint()`, which is fine. A red spike during play is a bug.
* **rule breaks** - anything drawn outside the play area, live.
* **notes played / dropped** - a dropped note means the buzzer queue
  overran, which means sound is being fired per frame rather than per event.

Sound starts on your first key press; browsers will not make noise before
one. What you hear is a plain square wave because that is all a passive
buzzer can do - if it sounds a bit cheap, it is accurate.

## Replaying

A recorded run, deterministic to the pixel:

```sh
make replay                              # runs scripts/demo.input
./build/sim --game mygame --script scripts/mine.input --duration 30000 \
            --record build/run.html --shot build/final.bmp
```

`--record` writes a self-contained HTML filmstrip: open it in a browser,
scrub or play through the run, click any thumbnail. It is the right thing to
attach to "it goes wrong about ten seconds in".

The script format is one event per line - `<ms> <action>`:

```
300   knob +2       # two detents clockwise
700   press
1200  theme 1       # switch palette mid-run
3000  shot playing  # keep this frame in the filmstrip, labelled
9000  end
```

Times are on the *device* clock, so a script replays identically on a fast
laptop and a slow one. Same seed plus same script equals the same pixels,
every time - which is what makes a bug report reproducible. `--seed` changes
the run; `--best N` pretends there is already a stored high score.

`--strict` makes it exit non-zero on any rule break, blown frame budget or
dropped note. That is what CI uses. `--mute` replays with the sound off.

## Simulating a busier device

```sh
./build/sim --game mygame --loop-ms 50 --jitter 120
```

`--loop-ms` is the device's fixed per-iteration delay and `--jitter` is the
irregular extra from background work. The defaults (50 and 40) match a device
going about its normal business. Raising the jitter shows you what your game
does when the device is busy - a game that only looks right at the default is
not finished.

Lowering them does not make the real device faster.

## The test bench

```sh
make test
```

`tests/conformance.cpp` runs **every game in the registry**, including yours
as soon as you add it, through the checks described in
[RULES.md](RULES.md): staying inside the play area, not hardcoding the
layout, rebuilding the screen from state, frame budget, determinism, input
debouncing. `tests/noalloc.cpp` counts every allocation during a run and
fails on any.

`tests/selfcheck.cpp` keeps a deliberately broken game around and asserts
that the bench catches it - so the suite going green means something.

`tests/gaterun.cpp` is the other half of the job: tests for what a specific
game actually *does*. **Write the equivalent for yours** - that the score is
stored exactly once per run, that the player cannot leave the play area, that
the rules of your game hold. The conformance suite proves your game behaves
on the hardware; only you can prove it does what it is supposed to.

Adding a test:

```cpp
TEST(my_game_scores_when_the_player_does_the_thing) {
    games::MyGame game;
    HarnessConfig cfg;
    Harness h(game, cfg);
    h.begin();

    h.queueKnob(+1);
    h.step(50);                 // one iteration of the device loop
    CHECK(..., "why this matters");
}
```

`Harness` is the simulator's host: `step(ms)` runs one main-loop iteration,
`queueKnob`/`queueButton` press things, `screen().pixel(x, y)` reads the
panel, `recorder().violations()` is the rule-break list.

## The lint

```sh
make lint
```

Scans `games/` for the things that compile fine on a desktop and misbehave on
the device: blocking calls, allocation, containers, `String`, direct hardware
access, `fillScreen`, bare `millis()`, the global RNG. Each hit prints the
rule and the reason.

Comments are stripped before scanning, so prose can discuss anything.
