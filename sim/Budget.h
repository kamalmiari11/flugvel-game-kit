#pragma once

namespace sim {

// What the panel costs. The bus runs at 40 MHz, 16 bits per pixel, so it
// moves roughly 2,500 pixels per millisecond - and while it is moving them
// the main loop is stopped, which means input is not being read either.
// Every budget below is derived from that one number.
static const long PIXELS_PER_MS = 2500;

// A frame at or under this is comfortable: ~5 ms of bus time, leaving the
// loop free to poll the knob and the button several times before the next one.
static const long FRAME_PIXELS_WARN = 12000;

// Past this a frame costs ~10 ms and you will feel it: the knob lags, the
// panel tears, and the whole device gets less responsive - not just the game.
static const long FRAME_PIXELS_FAIL = 25000;

// A full clear of the play area (320 x 190) is 60,800 pixels - 24 ms - which
// is why no game may do one per frame. It is fine once, in repaint().
static const long FULL_PLAY_AREA_PIXELS = 320L * 190L;

} // namespace sim
