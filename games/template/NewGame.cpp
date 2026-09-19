#include "NewGame.h"
#include <cstdio>

namespace games {

void NewGame::start(gk::Host& host) {
    _area = host.play();        // ask where you may draw; never hardcode it
    _lastFrame = host.millis();
    _cursor = 0;
    _taps = 0;
    _drawnCursor = -1;          // nothing is on screen yet
    _drawnTaps = -1;

    host.setLegend("MOVE", "TAP");
    // No drawing here - repaint() runs next and puts the first frame up.
}

void NewGame::repaint(gk::Host& host, gk::Gfx& gfx) {
    const gk::Theme& t = host.theme();

    gfx.fillRect(_area.x, _area.y, _area.w, _area.h, t.bg);

    gfx.setTextDatum(gk::Datum::MC);
    gfx.setTextSize(1);
    gfx.setTextColor(t.fgDim);
    gfx.drawString("TEMPLATE - REPLACE ME", _area.cx(), _area.y + 24);
    gfx.setTextDatum(gk::Datum::TL);

    _drawnCursor = -1;          // force the moving parts to repaint
    _drawnTaps = -1;
    update(host, gfx);
}

void NewGame::update(gk::Host& host, gk::Gfx& gfx) {
    const uint32_t now = host.millis();
    if (now - _lastFrame < FRAME_MS && _drawnCursor >= 0) return;
    _lastFrame = now;

    const gk::Theme& t = host.theme();
    const int boxY = _area.cy();
    const int boxW = 18;

    // Erase only what moved, then draw it in its new place.
    if (_cursor != _drawnCursor) {
        if (_drawnCursor >= 0)
            gfx.fillRect(_area.x + 10 + _drawnCursor * boxW, boxY, boxW - 4, boxW - 4, t.bg);
        gfx.fillRect(_area.x + 10 + _cursor * boxW, boxY, boxW - 4, boxW - 4, t.accent);
        _drawnCursor = _cursor;
    }

    if (_taps != _drawnTaps) {
        char buf[24];
        snprintf(buf, sizeof(buf), "TAPS %d", _taps);
        gfx.fillRect(_area.x + 10, _area.bottom() - 40, 120, 16, t.bg);
        gfx.setTextSize(2);
        gfx.setTextColor(t.fg);
        gfx.drawString(buf, _area.x + 10, _area.bottom() - 40);
        _drawnTaps = _taps;
    }
}

void NewGame::onKnob(gk::Host& host, int dir) {
    (void)host;
    _cursor += dir;
    if (_cursor < 0) _cursor = 0;
    if (_cursor > 9) _cursor = 9;
    // State only. update() draws it.
}

void NewGame::onButton(gk::Host& host) {
    _taps++;

    // The buzzer: one short note, fired on the event. It returns immediately -
    // the host plays it from its own loop. Never do this every frame; the
    // queue is eight notes deep and the test bench fails a run that overruns
    // it. A frequency of 0 is a rest, for putting a gap in a jingle:
    //
    //   host.playTone(1200, 50);
    //   host.playTone(0,    30);
    //   host.playTone(1800, 70);
    host.playTone(1800, 50);

    // When your game ends:
    //   host.submitScore(score);   // once, at the end of the run
    //   host.setLegend("BACK TO LIST", "PLAY AGAIN");
    // and on a knob turn from the game-over state:
    //   host.exitGame();
}

} // namespace games
