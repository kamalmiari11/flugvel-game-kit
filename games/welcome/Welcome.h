#pragma once
#include "gamekit/Game.h"

// START HERE - the first thing the device shows, and the first thing to run
// in the simulator.
//
// It is not a game. It is a live picture of everything the docs describe:
// where you may draw, what the controls report, how often you are actually
// called, what the theme colours are, and whether the buzzer works. Plug a
// device in and this tells you in five seconds whether the hardware, the
// build and the toolchain are all good.
//
// It is also a second worked example, smaller than the reference game and
// easier to read first: fixed-size state, dirty-rect drawing, a repaint()
// that rebuilds the screen, and input handlers that only set variables.
//
// Feel free to delete it once you are building the real thing - remove the
// row from games/registry.cpp and the folder.

namespace games {

class Welcome : public gk::Game {
public:
    const char* name() const override { return "Start Here"; }

    void start(gk::Host& host) override;
    void repaint(gk::Host& host, gk::Gfx& gfx) override;
    void update(gk::Host& host, gk::Gfx& gfx) override;
    void onKnob(gk::Host& host, int dir) override;
    void onButton(gk::Host& host) override;

private:
    static const uint32_t FRAME_MS = 120;   // readouts, not animation
    static const uint32_t BEEP_MS  = 400;   // how long the BEEP flag stays lit
    static const int KNOB_RANGE = 24;       // detents from end to end of the bar

    void drawKnob(gk::Gfx& gfx, const gk::Theme& t);
    void drawPresses(gk::Gfx& gfx, const gk::Theme& t);
    void drawBeep(gk::Gfx& gfx, const gk::Theme& t);
    void drawLoop(gk::Gfx& gfx, const gk::Theme& t);

    gk::Rect _area {0, 0, 0, 0};
    int _rowKnob = 0, _rowButton = 0, _rowLoop = 0, _rowSwatch = 0, _footerY = 0;

    int  _knob = 0;             // -KNOB_RANGE/2 .. +KNOB_RANGE/2
    int  _presses = 0;
    bool _beeping = false;
    uint32_t _beepUntil = 0;

    // Measured interval between calls. This is the number worth staring at:
    // it is the frame rate the docs talk about, live, on real hardware.
    //
    // The gaps are accumulated on every call but the DISPLAYED average is
    // only recomputed at the moment it is drawn. That is not fussiness -
    // anything shown on screen must only change when it is painted, or
    // repaint() will rebuild the screen from a newer value than update()
    // last drew and the two will disagree. The test bench catches exactly
    // that, and caught it here.
    uint32_t _lastCallAt = 0;
    uint32_t _gapSum = 0;
    uint32_t _gapCount = 0;
    uint32_t _intervalMs = 0;
    uint32_t _lastFrame = 0;

    int  _drawnKnob = 0x7FFF;
    int  _drawnPresses = -1;
    bool _drawnBeep = false;
    uint32_t _drawnInterval = 0xFFFFFFFF;
};

} // namespace games
