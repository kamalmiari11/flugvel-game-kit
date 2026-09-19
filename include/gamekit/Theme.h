#pragma once
#include "Colors.h"

// The device's palette, as the firmware hands it to every screen.
//
// A game must not invent its own colours. Take them from the theme the host
// gives you (Host::theme()) and use them for what they mean - the user can
// switch themes at runtime, and a game that hardcoded "green" ends up
// unreadable on the light one. Semantic roles only.
//
// All values are already panel-corrected (see Colors.h). Pass them straight
// to the drawing calls.

namespace gk {

struct Theme {
    const char* name;

    color_t bg;          // play-area background - what "erase this" means
    color_t fg;          // primary text and lines
    color_t fgDim;       // secondary text: hints, labels, anything supporting
    color_t rule;        // hairlines, borders, grid
    color_t selectBg;    // filled background of a selected/active item
    color_t selectFg;    // text drawn ON selectBg
    color_t selectMark;  // the marker bar on a selected item, drawn on selectBg
    color_t accent;      // "now / watch out" - warnings, hazards, the thing to react to
    color_t accent2;     // "good" - a hit, a point scored, a personal best
    color_t danger;      // "lost" - a miss, a life gone, game over

    int smallLineH;      // line height in px for size-1 text
};

namespace themes {
    extern const Theme Light;   // the device's default look
    extern const Theme Dark;    // the dim-room look

    const Theme& byIndex(int i);
    int count();
}

} // namespace gk
