#pragma once
#include "gamekit/Game.h"
#include <stdint.h>

namespace sim {

// The passive buzzer, modelled the way the device drives it: one voice, a
// square wave, notes strictly one after another, and a short queue that
// really does drop what will not fit.
//
// The drops are the point. On a desktop it is tempting to treat sound as
// free and fire a beep per frame; on the device the extras simply never
// play, and the game ends up with feedback that works sometimes. The test
// bench fails a run that drops notes.
class Buzzer {
public:
    struct Note { uint16_t freqHz; uint16_t ms; };

    void queue(uint16_t freqHz, uint16_t ms) {
        if (ms == 0) return;
        if (ms > gk::sound::MAX_MS) ms = gk::sound::MAX_MS;
        if (freqHz != 0) {
            if (freqHz < gk::sound::MIN_HZ) freqHz = gk::sound::MIN_HZ;
            if (freqHz > gk::sound::MAX_HZ) freqHz = gk::sound::MAX_HZ;
        }
        if (_count >= gk::sound::QUEUE_DEPTH) { _dropped++; return; }
        _queue[(_head + _count) % gk::sound::QUEUE_DEPTH] = { freqHz, ms };
        _count++;
    }

    void stop() {
        _count = 0;
        _head = 0;
        _remaining = 0;
        _current = { 0, 0 };
        _sounding = false;
    }

    // Consume `ms` of time, starting and finishing notes as it goes. Notes
    // that begin are appended to the "started" log for the front-end to play.
    void advance(uint32_t ms) {
        while (ms > 0) {
            if (_remaining == 0) {
                if (_count == 0) { _sounding = false; _current = { 0, 0 }; return; }
                _current = _queue[_head];
                _head = (_head + 1) % gk::sound::QUEUE_DEPTH;
                _count--;
                _remaining = _current.ms;
                _sounding = _current.freqHz != 0;
                if (_startedCount < kStartedMax) _started[_startedCount++] = _current;
                _played++;
            }
            uint32_t take = ms < _remaining ? ms : _remaining;
            _remaining -= take;
            ms -= take;
        }
    }

    bool     sounding() const { return _sounding && _remaining > 0; }
    uint16_t currentFreq() const { return sounding() ? _current.freqHz : 0; }
    long     played() const { return _played; }
    long     dropped() const { return _dropped; }
    int      queued() const { return _count; }

    // Notes that began since the last drain - what the browser front-end
    // actually plays. Drained rather than accumulated so a long session does
    // not grow without bound.
    int  startedCount() const { return _startedCount; }
    const Note& started(int i) const { return _started[i]; }
    void drainStarted() { _startedCount = 0; }

private:
    static const int kStartedMax = 32;

    Note _queue[gk::sound::QUEUE_DEPTH] {};
    int  _head = 0;
    int  _count = 0;

    Note     _current { 0, 0 };
    uint32_t _remaining = 0;
    bool     _sounding = false;

    Note _started[kStartedMax] {};
    int  _startedCount = 0;

    long _played = 0;
    long _dropped = 0;
};

} // namespace sim
