#include "Welcome.h"
#include <cstdio>

namespace games {

void Welcome::start(gk::Host& host) {
    _area = host.play();

    // Every row is placed off the play area the host reported - and off its
    // HEIGHT, not just its top edge. Offsets measured down from the top look
    // like they read host.play(), then run off the bottom the moment the
    // chrome gets taller. The header block is anchored to the top, the
    // footer to the bottom, and the four readout rows share what is left.
    const int top    = _area.y + 46;
    const int bottom = _area.bottom() - 34;
    int step = (bottom - top) / 4;
    if (step < 16) step = 16;          // the rows are 14px tall

    _rowKnob   = top;
    _rowButton = top + step;
    _rowLoop   = top + step * 2;
    _rowSwatch = top + step * 3;
    _footerY   = _area.bottom() - 26;

    _knob = 0;
    _presses = 0;
    _beeping = false;
    _beepUntil = 0;
    _lastCallAt = host.millis();
    _gapSum = 0;
    _gapCount = 0;
    _intervalMs = 0;
    _lastFrame = host.millis();

    _drawnKnob = 0x7FFF;          // nothing on screen yet
    _drawnPresses = -1;
    _drawnBeep = false;
    _drawnInterval = 0xFFFFFFFF;

    host.setLegend("MOVE THE BAR", "BEEP");
}

void Welcome::repaint(gk::Host& host, gk::Gfx& gfx) {
    const gk::Theme& t = host.theme();
    char buf[48];

    gfx.fillRect(_area.x, _area.y, _area.w, _area.h, t.bg);

    // A border on the exact edge of the play area. Everything you draw lives
    // inside this rectangle; the strips above and below belong to the device.
    gfx.drawRect(_area.x, _area.y, _area.w, _area.h, t.rule);

    gfx.setTextDatum(gk::Datum::TL);
    gfx.setTextSize(2);
    gfx.setTextColor(t.fg);
    gfx.drawString("START HERE", _area.x + 10, _area.y + 10);

    gfx.setTextSize(1);
    gfx.setTextColor(t.fgDim);
    snprintf(buf, sizeof(buf), "PLAY AREA %dx%d AT %d,%d - ALL YOURS",
             _area.w, _area.h, _area.x, _area.y);
    gfx.drawString(buf, _area.x + 10, _area.y + 32);

    // Row labels, painted once. Only the values to the right of them change.
    gfx.setTextColor(t.fgDim);
    gfx.drawString("KNOB",   _area.x + 10, _rowKnob + 3);
    gfx.drawString("BUTTON", _area.x + 10, _rowButton + 3);
    gfx.drawString("LOOP",   _area.x + 10, _rowLoop + 3);
    gfx.drawString("THEME",  _area.x + 10, _rowSwatch + 3);

    // The knob's track. The marker inside it is redrawn as it moves.
    gfx.drawRect(_area.x + 58, _rowKnob, 168, 14, t.rule);

    // Every colour the theme offers, in the order they appear in the struct.
    // Switch theme (t in the simulator) and the whole row changes - which is
    // the reason a game must never hardcode a colour.
    const gk::color_t swatches[] = {
        t.fg, t.fgDim, t.rule, t.selectBg, t.selectFg,
        t.selectMark, t.accent, t.accent2, t.danger
    };
    const int n = (int)(sizeof(swatches) / sizeof(swatches[0]));
    for (int i = 0; i < n; i++) {
        int x = _area.x + 58 + i * 20;
        gfx.fillRect(x, _rowSwatch, 18, 14, swatches[i]);
        gfx.drawRect(x, _rowSwatch, 18, 14, t.rule);
    }

    gfx.setTextColor(t.fgDim);
    gfx.drawString("TURN THE KNOB AND PRESS THE BUTTON.", _area.x + 10, _footerY);
    gfx.drawString("THEN READ README.MD AND DOCS/RULES.MD.", _area.x + 10, _footerY + 12);

    // Force every live readout to paint, then paint them.
    _drawnKnob = 0x7FFF;
    _drawnPresses = -1;
    _drawnBeep = !_beeping;
    _drawnInterval = 0xFFFFFFFF;
    drawKnob(gfx, t);
    drawPresses(gfx, t);
    drawBeep(gfx, t);
    drawLoop(gfx, t);
}

void Welcome::update(gk::Host& host, gk::Gfx& gfx) {
    const uint32_t now = host.millis();

    // Sample the gap on every call - that is the whole point of the
    // measurement - but only fold it into the displayed number below, at
    // the moment it is painted.
    uint32_t gap = now - _lastCallAt;
    _lastCallAt = now;
    if (gap > 0 && gap < 1000) { _gapSum += gap; _gapCount++; }

    if (now - _lastFrame < FRAME_MS && _drawnPresses >= 0) return;
    _lastFrame = now;

    // From here on, state that is visible on screen may change - and every
    // change is painted before this call returns.
    if (_gapCount) {
        _intervalMs = _gapSum / _gapCount;
        _gapSum = 0;
        _gapCount = 0;
    }
    _beeping = now < _beepUntil;

    const gk::Theme& t = host.theme();
    drawKnob(gfx, t);
    drawPresses(gfx, t);
    drawBeep(gfx, t);
    drawLoop(gfx, t);
}

void Welcome::drawKnob(gk::Gfx& gfx, const gk::Theme& t) {
    if (_knob == _drawnKnob) return;

    const int trackX = _area.x + 60, trackW = 164;
    const int span = trackW - 10;
    auto markerX = [&](int knob) {
        return trackX + (knob + KNOB_RANGE / 2) * span / KNOB_RANGE;
    };

    if (_drawnKnob != 0x7FFF)
        gfx.fillRect(markerX(_drawnKnob), _rowKnob + 2, 10, 10, t.bg);
    gfx.fillRect(markerX(_knob), _rowKnob + 2, 10, 10, t.accent);

    char buf[16];
    snprintf(buf, sizeof(buf), "%+d  ", _knob);
    gfx.setTextSize(1);
    gfx.setTextDatum(gk::Datum::TL);
    gfx.fillRect(_area.x + 232, _rowKnob + 3, 36, 8, t.bg);
    gfx.setTextColor(t.fg);
    gfx.drawString(buf, _area.x + 232, _rowKnob + 3);

    _drawnKnob = _knob;
}

void Welcome::drawPresses(gk::Gfx& gfx, const gk::Theme& t) {
    if (_presses == _drawnPresses) return;

    char buf[32];
    snprintf(buf, sizeof(buf), "%d", _presses);
    gfx.setTextSize(1);
    gfx.setTextDatum(gk::Datum::TL);
    gfx.fillRect(_area.x + 60, _rowButton + 3, 60, 8, t.bg);
    gfx.setTextColor(t.fg);
    gfx.drawString(buf, _area.x + 60, _rowButton + 3);

    _drawnPresses = _presses;
}

void Welcome::drawBeep(gk::Gfx& gfx, const gk::Theme& t) {
    if (_beeping == _drawnBeep) return;

    const int x = _area.x + 120, w = 42, h = 14;
    gfx.fillRect(x, _rowButton - 1, w, h, _beeping ? t.accent2 : t.bg);
    if (_beeping) {
        gfx.setTextSize(1);
        gfx.setTextDatum(gk::Datum::MC);
        gfx.setTextColor(t.bg);
        gfx.drawString("BEEP", x + w / 2, _rowButton - 1 + h / 2);
        gfx.setTextDatum(gk::Datum::TL);
    }
    _drawnBeep = _beeping;
}

void Welcome::drawLoop(gk::Gfx& gfx, const gk::Theme& t) {
    if (_intervalMs == _drawnInterval) return;

    char buf[40];
    unsigned fps = _intervalMs ? (unsigned)(1000 / _intervalMs) : 0;
    snprintf(buf, sizeof(buf), "%u MS BETWEEN CALLS - ABOUT %u FPS",
             (unsigned)_intervalMs, fps);
    gfx.setTextSize(1);
    gfx.setTextDatum(gk::Datum::TL);
    gfx.fillRect(_area.x + 60, _rowLoop + 3, 230, 8, t.bg);
    // Amber once it is slower than the device's own floor, which is where a
    // game written against a desktop frame rate starts to feel wrong.
    gfx.setTextColor(_intervalMs > 110 ? t.accent : t.fg);
    gfx.drawString(buf, _area.x + 60, _rowLoop + 3);

    _drawnInterval = _intervalMs;
}

void Welcome::onKnob(gk::Host& host, int dir) {
    (void)host;
    _knob += dir;
    if (_knob >  KNOB_RANGE / 2) _knob =  KNOB_RANGE / 2;
    if (_knob < -KNOB_RANGE / 2) _knob = -KNOB_RANGE / 2;
    // State only - update() paints it. See docs/RULES.md rule 7.
}

void Welcome::onButton(gk::Host& host) {
    _presses++;
    _beepUntil = host.millis() + BEEP_MS;
    // Two short notes, queued in one go. If you hear nothing here, the
    // buzzer or its wiring is the problem - not your game.
    host.playTone(1800, 45);
    host.playTone(0,    25);
    host.playTone(2400, 60);
}

} // namespace games
