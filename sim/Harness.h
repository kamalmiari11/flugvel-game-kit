#pragma once
#include "gamekit/Game.h"
#include "SoftGfx.h"
#include "Recorder.h"
#include "Buzzer.h"
#include <string>

namespace sim {

// Stands in for the device firmware: owns the screen, draws the chrome the
// game is not allowed to touch, feeds input through the same debounce the
// real button has, and calls the game the same way and at the same rate the
// real main loop does.
//
// The timing model matters more than anything else in here. On the device,
// loop() ends in a 50 ms delay and shares the CPU with background network
// work, so a game's update() is called roughly every 50-90 ms, irregularly -
// NOT at a frame rate. The harness reproduces that, jitter included, because
// a game tuned against a steady 60 fps desktop loop plays completely
// differently on hardware.
struct HarnessConfig {
    int      themeIndex = 0;
    uint32_t seed = 12345;
    uint32_t loopMs = 50;      // the device's fixed per-iteration delay
    uint32_t jitterMs = 40;    // extra, irregular: background work stealing time
    int      startingBest = 0;

    // The chrome the firmware reserves. They are configurable here for one
    // reason: to prove a game reads Host::play() instead of hardcoding 20 and
    // 30. A future firmware can change these, and a game that baked them in
    // will draw under the legend on the day it does.
    int      headerH = 20;
    int      legendH = 30;

    // Whether the player has sound switched on. Run with it off now and
    // then: a game that stops making sense muted is not finished.
    bool     soundOn = true;
};

class Harness : public gk::Host {
public:
    Harness(gk::Game& game, const HarnessConfig& cfg);

    // Panel geometry, fixed by the firmware. The top strip is the status bar
    // and the bottom strip is the control legend; between them is the game's.
    static const int HEADER_H = 20;
    static const int LEGEND_H = 30;

    gk::Rect play() const override {
        return { 0, _cfg.headerH, SoftGfx::W, SoftGfx::H - _cfg.headerH - _cfg.legendH };
    }
    const gk::Theme& theme() const override { return *_theme; }
    uint32_t millis() const override { return _now; }
    uint32_t random(uint32_t n) override;
    int bestScore() const override { return _best; }
    void submitScore(int score) override;
    void setLegend(const char* knob, const char* button) override;
    void playTone(uint16_t freqHz, uint16_t ms) override {
        if (!_cfg.soundOn) return;      // the device drops them silently too
        _buzzer.queue(freqHz, ms);
    }
    void stopSound() override { _buzzer.stop(); }
    bool soundEnabled() const override { return _cfg.soundOn; }

    void exitGame() override { _exited = true; }

    // ---- driving it ----
    void begin();                       // start() + repaint() + chrome
    void step(uint32_t advanceMs);      // one iteration of the device main loop
    void queueKnob(int dir);            // one detent, as the encoder would report it
    void queueButton();                 // one press, before debouncing

    // Start the run over from scratch, as re-entering the game on the device
    // would. Also the "R" key in the browser front-end.
    void restart() { _exited = false; _chromeDrawn = false; begin(); }

    // Switch palette mid-run. This is the case repaint() exists for: the
    // pixels on the glass are now the wrong colour, but no game state
    // changed, so the game is asked to paint itself again from where it is.
    void setThemeIndex(int idx);
    int themeIndex() const { return _themeIndex; }

    // Mute from the front-end. Muting stops whatever is sounding, the way
    // switching sound off on the device does.
    void setSoundOn(bool on) { _cfg.soundOn = on; if (!on) _buzzer.stop(); }

    // Ask the game to paint itself from scratch, the way the firmware does
    // after something else has covered the screen. Used by the test bench to
    // prove a game can actually rebuild its own display.
    void forceRepaint() { _game.repaint(*this, _rec); }

    bool exited() const { return _exited; }
    int  scoresSubmitted() const { return _submits; }
    int  lastScore() const { return _lastScore; }
    uint32_t now() const { return _now; }
    uint32_t frames() const { return _frames; }

    SoftGfx& screen() { return _screen; }
    Recorder& recorder() { return _rec; }
    Buzzer& buzzer() { return _buzzer; }

private:
    void drawChrome();

    gk::Game& _game;
    HarnessConfig _cfg;
    const gk::Theme* _theme;
    int _themeIndex = 0;
    SoftGfx _screen;
    Recorder _rec;
    Buzzer _buzzer;

    uint32_t _now = 0;
    uint32_t _rng;
    uint32_t _frames = 0;
    int _best = 0;
    int _submits = 0;
    int _lastScore = 0;
    bool _exited = false;

    // Input, queued between iterations exactly like the real poll: rotation
    // is counted in detents and delivered in full, the button is debounced.
    int _pendingKnob = 0;
    bool _pendingButton = false;
    uint32_t _lastButtonAt = 0;
    static const uint32_t BUTTON_DEBOUNCE_MS = 200;

    std::string _legendKnob, _legendButton, _drawnKnob, _drawnButton;
    bool _chromeDrawn = false;
};

} // namespace sim
