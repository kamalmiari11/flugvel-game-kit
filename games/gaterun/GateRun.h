#pragma once
#include "gamekit/Game.h"

// GATE RUN - the reference game.
//
// It is not the game you are being paid to build. It is here because it does
// every awkward thing correctly, and it is the shortest way to see what
// "correctly" means on this device:
//
//   * it paces itself off host.millis() and survives an irregular call rate
//   * it repaints only the pixels that changed, and can rebuild the whole
//     screen from state on demand (repaint())
//   * it keeps every byte of state in fixed-size members - no allocation,
//     anywhere, ever
//   * its input handlers only set variables; all drawing happens in update()
//   * it takes all colour from the theme and all randomness from the host
//   * it tells the host what the controls do in each state, and stores the
//     best score exactly once, when the run ends
//
// Read GateRun.cpp alongside docs/RULES.md. Anything it does that the rules
// forbid is a bug in one of the two - say so rather than copying it.
//
// Gameplay: the ship holds station while gates scroll in from the right. The
// knob moves it up and down; the button burns a short boost that doubles the
// speed and the points. Touch a wall and the run is over.

namespace games {

class GateRun : public gk::Game {
public:
    const char* name() const override { return "Gate Run"; }

    void start(gk::Host& host) override;
    void repaint(gk::Host& host, gk::Gfx& gfx) override;
    void update(gk::Host& host, gk::Gfx& gfx) override;
    void onKnob(gk::Host& host, int dir) override;
    void onButton(gk::Host& host) override;

private:
    enum class Phase : uint8_t { Playing, Over };

    // Tuned for the real play area: 320 across and only 190 down. The
    // vertical numbers are the tight ones - a gap of 60 in a 190px column
    // leaves the player about two ship-heights of room either side of centre.
    static const int GATE_COUNT   = 3;
    static const int GATE_W       = 22;
    static const int GATE_GAP     = 60;    // vertical opening, in px
    static const int GATE_SPACING = 106;   // horizontal distance between gates
    static const int SHIP_X_OFF   = 52;    // from the left edge of the play area
    static const int SHIP_SIZE    = 12;    // drawn size, and the collision box
    static const int SHIP_STEP    = 9;     // px per knob detent
    static const uint32_t FRAME_MS = 45;   // the pace the game WANTS to run at
    static const uint32_t BOOST_MS = 1200;
    static const int BOOST_COST   = 1;

    void layoutFrom(gk::Host& host);
    void placeGate(gk::Host& host, int i, float x);
    void drawGateColumn(gk::Gfx& gfx, const gk::Theme& t, int x, int gapCenter, gk::color_t c) const;
    void drawScore(gk::Gfx& gfx, const gk::Theme& t, int score, int previous) const;
    void drawBoostPip(gk::Gfx& gfx, const gk::Theme& t, bool on) const;
    void drawGameOver(gk::Host& host, gk::Gfx& gfx);
    void publishLegend(gk::Host& host) const;
    float speed() const;

    // Geometry, re-read from the host on every start() rather than baked in.
    gk::Rect _area {0, 0, 0, 0};
    int _shipX = 0;
    int _topY = 0, _bottomY = 0;

    Phase _phase = Phase::Playing;
    float _shipY = 0;
    float _gateX[GATE_COUNT] = {0, 0, 0};
    int   _gateGapY[GATE_COUNT] = {0, 0, 0};
    bool  _gateScored[GATE_COUNT] = {false, false, false};
    int   _score = 0;
    int   _best = 0;
    uint32_t _lastFrame = 0;
    uint32_t _boostUntil = 0;

    // What is currently ON THE SCREEN, which is not the same thing as what is
    // in the state above. Every dirty-rect renderer needs this second copy:
    // it is the only way to know which pixels to take back.
    float _drawnShipY = 0;
    float _drawnGateX[GATE_COUNT] = {0, 0, 0};
    int   _drawnGapY[GATE_COUNT] = {0, 0, 0};
    bool  _gateTeleported[GATE_COUNT] = {false, false, false};
    int   _drawnScore = -1;
    bool  _drawnBoost = false;

    // Set when the screen no longer shows what this game thinks it shows -
    // after a restart, for instance, where the game-over panel is still up.
    // update() clears it by repainting. An input handler cannot draw, so a
    // flag like this is how it asks for one.
    bool  _needsRepaint = false;
};

} // namespace games
