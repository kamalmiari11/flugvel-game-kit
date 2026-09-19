#include "GateRun.h"
#include <cstdio>

namespace games {

// 12x12, one bit per pixel, rows padded to a whole byte, MSB first - the
// format drawBitmap() takes. Drawing it in the background colour erases
// exactly the ship and nothing around it.
static const uint8_t kShip[] = {
    0x0C, 0x00,   // ....##......
    0x0E, 0x00,   // ....###.....
    0x3F, 0x00,   // ..######....
    0x3F, 0x80,   // ..#######...
    0x7F, 0xC0,   // .#########..
    0x7F, 0xE0,   // .##########.
    0x7F, 0xE0,   // .##########.
    0x7F, 0xC0,   // .#########..
    0x3F, 0x80,   // ..#######...
    0x3F, 0x00,   // ..######....
    0x0E, 0x00,   // ....###.....
    0x0C, 0x00,   // ....##......
};

void GateRun::layoutFrom(gk::Host& host) {
    _area = host.play();
    _shipX = _area.x + SHIP_X_OFF;
    _topY = _area.y;
    _bottomY = _area.bottom();
}

float GateRun::speed() const {
    // Ramps with the score so it keeps getting harder, capped so it stays
    // playable. Pixels per frame, not per second: the frame length is fixed
    // below, and tying speed to real elapsed time would make the game speed
    // up whenever the host had a slow iteration.
    //
    // The starting value is worth a word, because it is the number most
    // likely to be wrong in a game written on a desktop. The host calls this
    // game about fourteen times a second, so 4.2 px per frame is roughly
    // 60 px/s: the first gate reaches the ship in four seconds and they come
    // about every second and a half after that. At the 2.4 px it started life
    // with - which felt perfectly brisk with the simulator running flat out -
    // nothing happened at all for the first seven seconds and the game read
    // as broken. Always sanity-check a speed in seconds, not in pixels.
    float s = 4.2f + _score * 0.18f;
    if (s > 7.2f) s = 7.2f;
    return s;
}

void GateRun::placeGate(gk::Host& host, int i, float x) {
    _gateX[i] = x;
    // Keep the opening clear of both edges by half a gap plus a margin.
    int margin = GATE_GAP / 2 + 8;
    int span = (_bottomY - _topY) - 2 * margin;
    _gateGapY[i] = _topY + margin + (int)host.random(span > 1 ? (uint32_t)span : 1);
    _gateScored[i] = false;
}

void GateRun::start(gk::Host& host) {
    layoutFrom(host);

    _phase = Phase::Playing;
    _score = 0;
    _best = host.bestScore();
    _shipY = (float)((_topY + _bottomY) / 2);
    _boostUntil = 0;
    _lastFrame = host.millis();

    for (int i = 0; i < GATE_COUNT; i++) {
        placeGate(host, i, (float)(_area.right() + i * GATE_SPACING));
        _gateTeleported[i] = false;
    }

    // Nothing is on screen yet, so nothing is "drawn" - repaint() is called
    // straight after this and fills these in.
    _drawnShipY = _shipY;
    _drawnScore = -1;
    _drawnBoost = false;

    publishLegend(host);
}

void GateRun::publishLegend(gk::Host& host) const {
    if (_phase == Phase::Playing) host.setLegend("UP / DOWN", "BOOST");
    else                          host.setLegend("BACK TO LIST", "PLAY AGAIN");
}

void GateRun::drawGateColumn(gk::Gfx& gfx, const gk::Theme& t, int x, int gapCenter, gk::color_t c) const {
    (void)t;
    int gapTop = gapCenter - GATE_GAP / 2;
    int gapBottom = gapCenter + GATE_GAP / 2;
    gfx.fillRect(x, _topY, GATE_W, gapTop - _topY, c);
    gfx.fillRect(x, gapBottom, GATE_W, _bottomY - gapBottom, c);
}

void GateRun::drawScore(gk::Gfx& gfx, const gk::Theme& t, int score, int previous) const {
    // The plate is sized to the digits, not to some fixed box, so it does not
    // punch a hole in the playfield. When the number gets shorter the wider
    // old plate has to be cleared first or a stale digit is left behind.
    auto plateW = [](int v) { int d = 1; if (v < 0) v = -v; while (v >= 10) { v /= 10; d++; } return d * 12 + 4; };
    int x = _area.x + 8, y = _topY + 4;
    if (previous >= 0 && plateW(previous) > plateW(score))
        gfx.fillRect(x, y, plateW(previous), 18, t.bg);
    gfx.fillRect(x, y, plateW(score), 18, t.bg);
    gfx.setTextSize(2);
    gfx.setTextColor(t.fg);
    gfx.setCursor(x + 2, y + 2);
    gfx.print(score);
}

void GateRun::drawBoostPip(gk::Gfx& gfx, const gk::Theme& t, bool on) const {
    int x = _area.right() - 28, y = _topY + 6;
    gfx.fillRect(x, y, 20, 12, on ? t.accent : t.bg);
    if (!on) return;
    gfx.setTextSize(1);
    gfx.setTextColor(t.bg);
    gfx.setTextDatum(gk::Datum::MC);
    gfx.drawString("BST", x + 10, y + 6);
    gfx.setTextDatum(gk::Datum::TL);
}

// Paint the current state in full. The host calls this after start(), and
// again any time something else has painted over the screen. It must not
// change any game state - it only reads it - and afterwards the "drawn"
// mirror has to agree with reality, or the next update() will erase the
// wrong pixels.
void GateRun::repaint(gk::Host& host, gk::Gfx& gfx) {
    const gk::Theme& t = host.theme();

    gfx.fillRect(_area.x, _area.y, _area.w, _area.h, t.bg);

    if (_phase == Phase::Over) {
        drawGameOver(host, gfx);
        return;
    }

    for (int i = 0; i < GATE_COUNT; i++) {
        drawGateColumn(gfx, t, (int)_gateX[i], _gateGapY[i], t.accent2);
        _drawnGateX[i] = _gateX[i];
        _drawnGapY[i] = _gateGapY[i];
        _gateTeleported[i] = false;
    }

    gfx.drawBitmap(_shipX - SHIP_SIZE / 2, (int)_shipY - SHIP_SIZE / 2, kShip, SHIP_SIZE, SHIP_SIZE, t.fg);
    _drawnShipY = _shipY;

    bool boosting = host.millis() < _boostUntil;
    drawBoostPip(gfx, t, boosting);
    _drawnBoost = boosting;

    drawScore(gfx, t, _score, -1);
    _drawnScore = _score;
}

void GateRun::update(gk::Host& host, gk::Gfx& gfx) {
    if (_needsRepaint) {
        _needsRepaint = false;
        repaint(host, gfx);
        return;
    }
    if (_phase != Phase::Playing) return;   // the game-over screen is static

    const uint32_t now = host.millis();

    // Fixed-step pacing. The host calls this whenever it gets round to it -
    // every 50 ms, or every 300 ms if something else was busy - so the game
    // decides for itself when a frame is due. Everything below runs exactly
    // once per due frame, never twice for a long gap: catching up by running
    // two frames back to back doubles the drawing in one pass over the bus,
    // which is the stall you were trying to avoid.
    if (now - _lastFrame < FRAME_MS) return;
    _lastFrame = now;

    const gk::Theme& t = host.theme();
    const bool boosting = now < _boostUntil;
    const float step = speed() * (boosting ? 1.8f : 1.0f);
    const int half = SHIP_SIZE / 2;

    bool crashed = false;

    for (int i = 0; i < GATE_COUNT; i++) {
        _gateX[i] -= step;

        if (_gateX[i] + GATE_W < _area.x) {
            // Off the left edge: move it back out to the right of the last
            // one. That is a teleport, not a scroll, so the renderer has to
            // erase the old column in full instead of diffing the edges.
            float rightmost = _gateX[0];
            for (int j = 1; j < GATE_COUNT; j++)
                if (_gateX[j] > rightmost) rightmost = _gateX[j];
            placeGate(host, i, rightmost + GATE_SPACING);
            _gateTeleported[i] = true;
            continue;
        }

        if (!_gateScored[i] && _gateX[i] + GATE_W < _shipX - half) {
            _gateScored[i] = true;
            _score += boosting ? 2 : 1;
            // One short blip per point, fired on the event rather than per
            // frame. Near the buzzer's loudest range so it cuts through, and
            // short enough that a fast run does not turn into a drone.
            host.playTone(boosting ? 2600 : 2200, 40);
        }

        bool overlapsX = (_shipX + half > _gateX[i]) && (_shipX - half < _gateX[i] + GATE_W);
        if (overlapsX) {
            int gapTop = _gateGapY[i] - GATE_GAP / 2;
            int gapBottom = _gateGapY[i] + GATE_GAP / 2;
            if (_shipY - half < gapTop || _shipY + half > gapBottom) crashed = true;
        }
    }

    if (_shipY - half < _topY || _shipY + half > _bottomY) crashed = true;

    if (crashed) {
        _phase = Phase::Over;
        // A falling two-note buzz. Queued in one go - the host plays them
        // back to back without the game waiting on anything.
        host.stopSound();            // whatever was blipping is now irrelevant
        host.playTone(700, 90);
        host.playTone(0,   30);
        host.playTone(420, 180);
        // Exactly one write per run, at the end of it. Writing on every point
        // scored would stutter the game and wear the flash out. The host owns
        // the stored value, so read it back rather than tracking it here.
        bool record = _score > _best && _score > 0;
        host.submitScore(_score);
        _best = host.bestScore();
        if (record) {
            // Three rising notes behind the buzz. Six notes total this frame,
            // which is inside the host's queue - fire any more and the tail
            // is silently dropped.
            host.playTone(1600, 70);
            host.playTone(2000, 70);
            host.playTone(2600, 120);
        }
        publishLegend(host);
        drawGameOver(host, gfx);
        return;
    }

    // ---- gates ----
    for (int i = 0; i < GATE_COUNT; i++) {
        if (_gateTeleported[i]) {
            drawGateColumn(gfx, t, (int)_drawnGateX[i], _drawnGapY[i], t.bg);
            drawGateColumn(gfx, t, (int)_gateX[i], _gateGapY[i], t.accent2);
            _drawnGateX[i] = _gateX[i];
            _drawnGapY[i] = _gateGapY[i];
            _gateTeleported[i] = false;
            continue;
        }

        int oldX = (int)_drawnGateX[i];
        int newX = (int)_gateX[i];
        int delta = oldX - newX;          // gates only travel left
        if (delta <= 0) continue;         // has not moved a whole pixel yet

        int gapTop = _gateGapY[i] - GATE_GAP / 2;
        int gapBottom = _gateGapY[i] + GATE_GAP / 2;

        // Two thin strips per gate instead of the whole column: the sliver the
        // gate just moved onto, and the sliver it just left behind.
        gfx.fillRect(newX, _topY, delta, gapTop - _topY, t.accent2);
        gfx.fillRect(newX, gapBottom, delta, _bottomY - gapBottom, t.accent2);
        gfx.fillRect(newX + GATE_W, _topY, delta, gapTop - _topY, t.bg);
        gfx.fillRect(newX + GATE_W, gapBottom, delta, _bottomY - gapBottom, t.bg);

        _drawnGateX[i] = _gateX[i];
    }

    // ---- ship ---- (x never changes, so only the 12x12 box moves)
    if ((int)_drawnShipY != (int)_shipY) {
        gfx.drawBitmap(_shipX - half, (int)_drawnShipY - half, kShip, SHIP_SIZE, SHIP_SIZE, t.bg);
        gfx.drawBitmap(_shipX - half, (int)_shipY - half, kShip, SHIP_SIZE, SHIP_SIZE, t.fg);
        _drawnShipY = _shipY;
    }

    // ---- HUD ----
    // Both of these are repainted LAST and UNCONDITIONALLY, every frame, even
    // when nothing about them changed. They sit inside the playfield, and the
    // gates scroll straight through them - so anything drawn "only when it
    // changes" gets eaten by the next column that passes and never comes back.
    // It costs about 500 px a frame, which is a fifth of a millisecond, and it
    // is the difference between a HUD and a HUD with holes in it.
    if (boosting != _drawnBoost) {
        _drawnBoost = boosting;
        publishLegend(host);
    }
    drawBoostPip(gfx, t, boosting);
    drawScore(gfx, t, _score, _drawnScore);
    _drawnScore = _score;
}

void GateRun::drawGameOver(gk::Host& host, gk::Gfx& gfx) {
    const gk::Theme& t = host.theme();
    int cx = _area.cx();
    int cy = _area.cy();

    gfx.fillRect(_area.x, _area.y, _area.w, _area.h, t.bg);

    gfx.setTextDatum(gk::Datum::MC);
    gfx.setTextSize(2);
    gfx.setTextColor(t.danger);
    gfx.drawString("GAME OVER", cx, cy - 46);

    gfx.setTextSize(3);
    gfx.setTextColor(t.fg);
    char buf[16];
    snprintf(buf, sizeof(buf), "%d", _score);
    gfx.drawString(buf, cx, cy - 6);

    gfx.setTextSize(1);
    bool record = _score >= _best && _score > 0;
    gfx.setTextColor(record ? t.accent2 : t.fgDim);
    snprintf(buf, sizeof(buf), record ? "NEW BEST %d" : "BEST %d", _best);
    gfx.drawString(buf, cx, cy + 26);

    gfx.setTextColor(t.fgDim);
    gfx.drawString("PRESS TO PLAY AGAIN", cx, cy + 46);
    gfx.drawString("TURN TO LEAVE", cx, cy + 58);
    gfx.setTextDatum(gk::Datum::TL);
}

void GateRun::onKnob(gk::Host& host, int dir) {
    // House rule on this device: while a game is over, turning the knob backs
    // out to the game list. The button owns restart, so the knob has to own
    // the exit - otherwise there is no way out without a hard reset.
    if (_phase == Phase::Over) { host.exitGame(); return; }

    _shipY -= dir * SHIP_STEP;   // clockwise (+1) moves up
    int half = SHIP_SIZE / 2;
    if (_shipY < _topY + half) _shipY = (float)(_topY + half);
    if (_shipY > _bottomY - half) _shipY = (float)(_bottomY - half);
    // No drawing here on purpose - update() paints the move. See Game.h.
}

void GateRun::onButton(gk::Host& host) {
    if (_phase == Phase::Over) {
        start(host);
        // start() resets state but draws nothing, and the game-over panel is
        // still on the glass - so ask the next update() for a full repaint.
        _needsRepaint = true;
        return;
    }

    if (host.millis() < _boostUntil) return;   // already burning
    _boostUntil = host.millis() + BOOST_MS;
    _score -= BOOST_COST;
    if (_score < 0) _score = 0;
    // Rising pair, so boosting sounds like the opposite of crashing.
    host.playTone(1400, 40);
    host.playTone(2100, 60);
}

} // namespace games
