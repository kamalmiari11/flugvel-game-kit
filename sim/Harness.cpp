#include "Harness.h"
#include <cstdio>

namespace sim {

Harness::Harness(gk::Game& game, const HarnessConfig& cfg)
    : _game(game), _cfg(cfg), _theme(&gk::themes::byIndex(cfg.themeIndex)),
      _rec(_screen, { 0, cfg.headerH, SoftGfx::W, SoftGfx::H - cfg.headerH - cfg.legendH }),
      _rng(cfg.seed ? cfg.seed : 1), _best(cfg.startingBest) {
    _themeIndex = cfg.themeIndex;
}

void Harness::setThemeIndex(int idx) {
    if (idx < 0) idx = 0;
    if (idx >= gk::themes::count()) idx = 0;
    _themeIndex = idx;
    _theme = &gk::themes::byIndex(idx);

    _rec.setWatching(false);
    _screen.fillRect(0, 0, SoftGfx::W, SoftGfx::H, _theme->bg);
    _rec.setWatching(true);

    _game.repaint(*this, _rec);
    _chromeDrawn = false;
    drawChrome();
}

uint32_t Harness::random(uint32_t n) {
    // xorshift32 - the same sequence every run for a given seed, which is what
    // makes a recorded input script reproduce a bug exactly.
    _rng ^= _rng << 13; _rng ^= _rng >> 17; _rng ^= _rng << 5;
    return n ? (_rng % n) : 0;
}

void Harness::submitScore(int score) {
    _submits++;
    _lastScore = score;
    if (score > _best) _best = score;
}

void Harness::setLegend(const char* knob, const char* button) {
    _legendKnob = knob ? knob : "";
    _legendButton = button ? button : "";
}

void Harness::begin() {
    _buzzer.stop();
    _rec.setWatching(false);
    _screen.fillRect(0, 0, SoftGfx::W, SoftGfx::H, _theme->bg);
    _rec.setWatching(true);

    _game.start(*this);
    _game.repaint(*this, _rec);
    _rec.notePeak();
    _rec.resetFrame();
    drawChrome();
}

void Harness::step(uint32_t advanceMs) {
    _now += advanceMs;

    // Rotation first, then the button - the order the firmware polls in.
    while (_pendingKnob != 0) {
        int dir = _pendingKnob > 0 ? 1 : -1;
        _pendingKnob -= dir;
        _game.onKnob(*this, dir);
    }
    if (_pendingButton) {
        _pendingButton = false;
        if (_now - _lastButtonAt > BUTTON_DEBOUNCE_MS || _lastButtonAt == 0) {
            _lastButtonAt = _now;
            _game.onButton(*this);
        }
        // else: swallowed by the debounce, exactly as the hardware would.
    }

    // The buzzer runs on its own time, like the hardware: the host services
    // it every iteration whatever the game is doing.
    _buzzer.advance(advanceMs);

    _rec.resetFrame();
    _game.update(*this, _rec);
    _rec.notePeak();
    _frames++;

    // The chrome is repainted last on every iteration, over whatever the game
    // left behind - which is why anything drawn up there disappears.
    drawChrome();
}

void Harness::queueKnob(int dir) { _pendingKnob += (dir > 0 ? 1 : -1); }
void Harness::queueButton() { _pendingButton = true; }

void Harness::drawChrome() {
    bool first = !_chromeDrawn;
    bool legendChanged = (_legendKnob != _drawnKnob) || (_legendButton != _drawnButton);
    if (!first && !legendChanged) return;   // the device only repaints on change

    _rec.setWatching(false);   // the host is allowed up here; the game is not
    const gk::Theme& t = *_theme;

    const int headerH = _cfg.headerH;
    if (first) {
        // Status bar: a stand-in for the real one. Same height, same habit of
        // repainting itself over anything in its way.
        _rec.fillRect(0, 0, SoftGfx::W, headerH, t.selectBg);
        _rec.setTextSize(1);
        _rec.setTextColor(t.selectFg);
        _rec.setTextDatum(gk::Datum::ML);
        _rec.drawString(_game.name(), 6, headerH / 2);
        for (int i = 0; i < 5; i++) {
            int cx = SoftGfx::W - 10 - i * 9;
            if (i == 2) _rec.fillCircle(cx, headerH / 2, 2, t.selectFg);
            else _rec.drawCircle(cx, headerH / 2, 2, t.selectFg);
        }
    }

    const int ly = SoftGfx::H - _cfg.legendH;
    _rec.fillRect(0, ly, SoftGfx::W, _cfg.legendH, t.bg);
    _rec.drawFastHLine(0, ly, SoftGfx::W, t.rule);
    _rec.setTextSize(1);
    _rec.setTextDatum(gk::Datum::TL);
    _rec.setTextColor(t.fgDim);
    _rec.drawString(_legendKnob.empty() ? "KNOB  -" : ("KNOB  " + _legendKnob).c_str(), 6, ly + 5);
    _rec.setTextColor(t.fg);
    _rec.drawString(_legendButton.empty() ? "PRESS -" : ("PRESS " + _legendButton).c_str(), 6, ly + 16);

    _rec.setWatching(true);
    _drawnKnob = _legendKnob;
    _drawnButton = _legendButton;
    _chromeDrawn = true;
}

} // namespace sim
