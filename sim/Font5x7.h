#pragma once
#include <stdint.h>

// The 5x7 glyph set the simulator draws text with, in a 6x8 cell - the same
// metrics as the single font compiled into the device firmware, so a string
// occupies the same number of pixels here as it does on the panel.
//
// The shapes are close but not pixel-identical to the device's font, and
// lowercase is drawn with the uppercase glyph (the device has real lowercase).
// Layout is what this is for; if a screen is tight enough that the exact
// glyph shape matters, check it on hardware.
namespace sim {

// Returns 7 rows of 5 bits (bit 4 = leftmost column), or the "unknown glyph"
// box for anything not in the table.
const uint8_t* glyphRows(char c);

} // namespace sim
