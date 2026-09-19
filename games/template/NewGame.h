#pragma once
#include "gamekit/Game.h"

// YOUR GAME GOES HERE.
//
// This skeleton builds and runs as-is (a cursor the knob moves and a counter
// the button bumps) so you have something on screen from minute one. Replace
// the body, keep the shape.
//
// Rename the class and the files to your game if you like - update
// games/registry.cpp to match. Everything you need is on gk::Host (passed
// into every call) and gk::Gfx (passed into the two that draw).
//
// Before you start: docs/RULES.md. It is short, and every line of it comes
// from something that went wrong on this hardware.

namespace games {

class NewGame : public gk::Game {
public:
    const char* name() const override { return "New Game"; }   // <= 16 chars

    void start(gk::Host& host) override;
    void repaint(gk::Host& host, gk::Gfx& gfx) override;
    void update(gk::Host& host, gk::Gfx& gfx) override;
    void onKnob(gk::Host& host, int dir) override;
    void onButton(gk::Host& host) override;

private:
    static const uint32_t FRAME_MS = 50;   // your target pace

    gk::Rect _area {0, 0, 0, 0};
    uint32_t _lastFrame = 0;

    int _cursor = 0;        // moved by the knob
    int _taps = 0;          // bumped by the button
    int _drawnCursor = -1;  // what is actually on screen right now
    int _drawnTaps = -1;
};

} // namespace games
