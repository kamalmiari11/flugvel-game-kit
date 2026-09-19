#pragma once
#include <stdint.h>
#include "Gfx.h"
#include "Theme.h"

// The contract. A game is one class implementing gk::Game; the host owns
// everything else - the screen, the input, the score storage, the menu you
// were launched from and the strip at the bottom that says what the controls
// do right now.
//
// The host calls you. You never call back into anything that is not on the
// Host below: no display driver, no Wi-Fi, no filesystem, no delay(), no
// millis() of your own. See docs/RULES.md.

namespace gk {

struct Rect {
    int x, y, w, h;
    int right()  const { return x + w; }
    int bottom() const { return y + h; }
    int cx()     const { return x + w / 2; }
    int cy()     const { return y + h / 2; }
    bool contains(int px, int py) const {
        return px >= x && px < x + w && py >= y && py < y + h;
    }
};

// ---- Sound -----------------------------------------------------------
// The device has a passive buzzer: one pin, a square wave, no volume control
// and one note at a time. It is a beeper, not an instrument.
namespace sound {
    // Anything outside this is either inaudible on a part this small or
    // unpleasant on a desk. The host clamps rather than refusing.
    static const uint16_t MIN_HZ = 100;
    static const uint16_t MAX_HZ = 5000;

    // A small piezo is loudest around here - useful to know when picking the
    // note that has to cut through a room.
    static const uint16_t SWEET_SPOT_HZ = 2700;

    // One note may not run longer than this. A game holding a tone for
    // seconds is a game nobody keeps switched on.
    static const uint16_t MAX_MS = 2000;

    // How many notes the host will hold for you. Enough for a short jingle
    // fired in one go; queue more and the extras are dropped, which the test
    // bench treats as a bug.
    static const int QUEUE_DEPTH = 8;
}

class Host {
public:
    virtual ~Host() = default;

    // Where to draw. Everything outside this rectangle belongs to the device
    // chrome (the status bar at the top, the control legend at the bottom) and
    // is repainted over you if you touch it. Read it every time rather than
    // caching it in a constant - it is not the same on every firmware build.
    virtual Rect play() const = 0;

    // Colours. Never hardcode; see Theme.h.
    virtual const Theme& theme() const = 0;

    // Milliseconds since boot. This is the ONLY clock you get, and the only
    // way to pace anything. It jumps: the host's loop is shared with
    // background work, so consecutive update() calls can be 15 ms or 300 ms
    // apart. Never assume a fixed step.
    virtual uint32_t millis() const = 0;

    // Random integer in [0, n). Seeded by the host, and deterministic under
    // test - which is what makes a replay reproduce a bug. Do not use rand().
    virtual uint32_t random(uint32_t n) = 0;

    // The player's stored best for this game, and how you update it. Call
    // submitScore exactly once per run, when the run ENDS. It writes to flash,
    // which stalls the device for milliseconds - calling it per point would
    // both stutter the game and wear the part out.
    virtual int  bestScore() const = 0;
    virtual void submitScore(int score) = 0;

    // The two-line strip at the bottom of the screen: what turning the knob
    // does, and what the button does, RIGHT NOW. Update it whenever the answer
    // changes (playing vs. game over vs. paused). Pass "" for the knob line if
    // the knob genuinely does nothing in this state. Keep each under ~22
    // characters or it is clipped.
    virtual void setLegend(const char* knob, const char* button) = 0;

    // ---- The buzzer ----
    // Queue one note: a square wave at freqHz for ms milliseconds. Returns
    // immediately - the host plays it from its own loop, so this never
    // blocks and never costs you a frame.
    //
    // A frequency of 0 is a rest, which is how you put a gap between notes
    // in a jingle:
    //
    //     host.playTone(1200, 60);      // blip
    //     host.playTone(0,    40);      // gap
    //     host.playTone(1800, 80);      // higher blip
    //
    // Frequency and duration are clamped to the limits in gk::sound. There is
    // no volume: a passive buzzer is on or off. There is one voice: notes
    // play strictly one after another, so a beep fired while another is
    // sounding waits its turn rather than blending.
    //
    // Fire these on events - a point scored, a life lost - never every frame.
    // The queue holds gk::sound::QUEUE_DEPTH notes and drops the rest.
    virtual void playTone(uint16_t freqHz, uint16_t ms) = 0;

    // Cut everything short: the sounding note and anything queued behind it.
    // For a run ending mid-jingle, or leaving the game.
    virtual void stopSound() = 0;

    // Whether the player has sound switched on. Notes are dropped silently
    // when they do not, so you do not have to check this - it is here for the
    // rare case where a game wants to show something instead of playing it.
    //
    // A game must remain completely playable with the sound off. Audio is
    // feedback for something the screen is already telling the player, never
    // the only place information appears.
    virtual bool soundEnabled() const = 0;

    // Leave the game and go back to the device's game list. The host repaints
    // everything after this; do not draw anything once you have called it.
    virtual void exitGame() = 0;
};

// Your game is constructed once at boot and lives for the whole life of the
// device - it is a static object, not something allocated per run. Put all
// state in members, size every array at compile time, and do no allocation
// anywhere (see docs/RULES.md: the test bench fails a run that allocates).
class Game {
public:
    virtual ~Game() = default;

    // Shown in the device's game list. Keep it to ~16 characters.
    virtual const char* name() const = 0;

    // Reset to a fresh run. Called once per run, before any drawing. Do not
    // draw here - initialise state only; repaint() is called immediately
    // after and paints the first frame.
    virtual void start(Host& host) = 0;

    // Paint the CURRENT state, in full, assuming nothing on screen is yours.
    // The host calls this after start(), and again any time something painted
    // over you (a system overlay, a theme change, the screen being re-entered).
    // It must be safe to call at any moment and must leave the screen showing
    // exactly what update() would have left. Everything you rely on having
    // drawn goes here, not in start().
    virtual void repaint(Host& host, Gfx& gfx) = 0;

    // Advance the game and paint what changed. Called as fast as the host's
    // loop runs, which is not a frame rate - gate your own simulation on
    // host.millis() and return immediately when it is not time yet. Must
    // never block, sleep, or spin.
    virtual void update(Host& host, Gfx& gfx) = 0;

    // One detent of the knob: dir is +1 (clockwise) or -1. Never called more
    // than once per detent, but can be called several times in one host loop
    // if the knob was spun hard.
    //
    // Note there is no Gfx here, on purpose: input handlers record what
    // happened and let the next update() paint it. Drawing from an input
    // handler means a fast spin paints the same object five times in one
    // pass over the bus, which is exactly the stall that makes the knob feel
    // laggy - the thing you were trying to make responsive.
    virtual void onKnob(Host& host, int dir) = 0;

    // The button went down. The host debounces it for you: presses arrive at
    // most once per 200 ms, so do not design a game that needs faster
    // tapping than that. Same no-drawing rule as onKnob().
    virtual void onButton(Host& host) = 0;
};

} // namespace gk
