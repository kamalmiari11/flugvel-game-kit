# Game kit

Everything needed to build a game for a small handheld device, without the
device.

The hardware is a 320x240 colour panel, one knob that clicks round in
detents, one button, and a passive buzzer. No touchscreen, no keyboard, no
network for you to use, no filesystem. A game is one C++ class. The kit gives you a simulator
that reproduces the device's screen, its colours, its controls and - the part
that catches people out - its timing, plus a test bench that fails the build
if a game does something the device cannot afford.

## Sixty seconds

```sh
make run
```

Open <http://127.0.0.1:8080/>. You are looking at the device, running the
start screen: it shows the area you may draw in, what the controls report,
how often your game is actually called, and the theme's colours. Arrow keys
turn the knob, space is the button, and the buzzer plays through your
speakers.

The same screen is what a device boots into, so plugging one in tells you
straight away whether the hardware, the build and the toolchain are good.

```sh
make test     # the test bench
make lint     # the house rules, checked against your source
make check    # both, plus a scripted replay - this is what CI runs
```

No dependencies: a C++17 compiler and `make`. Nothing to install, no SDL, no
Python, no Arduino toolchain (unless you were sent a unit - see
[`device/README.md`](device/README.md)).

## Where to start

1. **[`docs/HARDWARE.md`](docs/HARDWARE.md)** - the device: screen, colours,
   controls, timing, memory. Read this first; most of what makes this
   different from desktop game work is in there.
2. **[`docs/RULES.md`](docs/RULES.md)** - the rules a game must follow, each
   with the reason it exists and how it is checked. Short.
3. **[`games/welcome/`](games/welcome/)** - the start screen you are looking
   at. The smallest complete example: fixed state, dirty-rect drawing, a
   `repaint()` that rebuilds the screen, handlers that only set variables.
4. **[`games/gaterun/`](games/gaterun/)** - the reference game. Small,
   complete, and does every awkward thing correctly, sound included. Read it
   next to the rules.
5. **[`docs/API.md`](docs/API.md)** - the full reference for everything you
   can call.
6. **[`games/template/`](games/template/)** - a skeleton that already builds
   and runs. Start here.
7. **[`BRIEF.md`](BRIEF.md)** - what you are actually being asked to build.
8. **[`docs/DELIVERY.md`](docs/DELIVERY.md)** - what "done" means, and how
   the work is reviewed.

## What is in the box

```
include/gamekit/     the contract: Game, Host, Gfx, Theme, Colors
src/gamekit/         the palette
games/welcome/       the start screen - what the device boots into
games/gaterun/       reference game - read this
games/template/      skeleton - start here
games/registry.cpp   the list of games the sim and the device can run
sim/                 the simulator: renderer, host harness, browser front-end
tests/               the test bench and the source lint
device/              optional: builds and flashes to real hardware
scripts/             recorded input scripts for replays
docs/                the documentation above
```

## Your game in one file

```cpp
class MyGame : public gk::Game {
    const char* name() const override { return "My Game"; }
    void start(gk::Host&) override;                  // reset state
    void repaint(gk::Host&, gk::Gfx&) override;      // paint everything, from state
    void update(gk::Host&, gk::Gfx&) override;       // advance, paint what changed
    void onKnob(gk::Host&, int dir) override;        // one detent, +1 or -1
    void onButton(gk::Host&) override;               // one press
};
```

Add it to `games/registry.cpp` and it appears in the simulator, on the
hardware, and in every test automatically.

## The short version of the rules

Read [`docs/RULES.md`](docs/RULES.md) properly, but if you remember five
things:

1. **Never block.** No `delay`, no waiting, no long loops. You are a guest in
   a main loop that has other work to do.
2. **Never allocate.** No `new`, no `String`, no `std::vector`. Fixed-size
   members only.
3. **Redraw only what changed.** The screen is on the far end of a slow wire.
   Clearing it costs 24 ms, and during those 24 ms nothing else happens -
   including reading the button.
4. **Stay inside `host.play()`.** The strips above and below belong to the
   device.
5. **Take the clock, the dice and the colours from the host.** Never from
   the system.

All five are enforced by `make check`.
