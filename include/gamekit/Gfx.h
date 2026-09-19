#pragma once
#include <stdint.h>
#include "Colors.h"

// Everything a game is allowed to draw with.
//
// This is deliberately the exact subset of the display driver the firmware
// itself uses - same names, same argument order, same semantics - so the
// game you write here drops into the device build unchanged. There is no
// framebuffer and no double buffering: every call goes straight out over
// SPI to the panel, which is why what you draw and how much of it is a
// performance decision, not a style one. Read docs/RULES.md before writing
// a single draw call.
//
// Coordinates are panel coordinates: (0,0) is the top-left of the whole
// 240x320 screen, NOT of your play area. Ask the host where your play area
// is (Host::play()) and stay inside it - the harness fails the run if you
// don't.

namespace gk {

// Text anchor points: which part of the string lands on the (x,y) you give
// drawString(). Vertical letter first, so MC is middle-centre and BR is
// bottom-right.
//
// The values match the display driver's datum constants one for one, but the
// names deliberately do not: the driver defines TL_DATUM and friends as
// PREPROCESSOR MACROS, and an enumerator with the same spelling does not
// survive being macro-expanded. Scoped, short names sidestep that.
enum class Datum : uint8_t {
    TL = 0, TC, TR,
    ML,     MC, MR,
    BL,     BC, BR,
};

class Gfx {
public:
    virtual ~Gfx() = default;

    // Full panel size: 240 x 320. Your play area is smaller - see Host::play().
    virtual int width() const = 0;
    virtual int height() const = 0;

    // ---- Shapes -----------------------------------------------------------
    // fillRect is the workhorse: it is how you paint, and passing theme.bg is
    // how you erase. Cost is proportional to w*h, so erase the 16x16 box the
    // sprite was in, not the 240x270 area it was somewhere inside.
    virtual void fillRect(int x, int y, int w, int h, color_t c) = 0;
    virtual void drawRect(int x, int y, int w, int h, color_t c) = 0;
    virtual void drawPixel(int x, int y, color_t c) = 0;
    virtual void drawFastHLine(int x, int y, int w, color_t c) = 0;
    virtual void drawFastVLine(int x, int y, int h, color_t c) = 0;
    virtual void drawLine(int x0, int y0, int x1, int y1, color_t c) = 0;
    virtual void fillCircle(int cx, int cy, int r, color_t c) = 0;
    virtual void drawCircle(int cx, int cy, int r, color_t c) = 0;
    virtual void fillTriangle(int x0, int y0, int x1, int y1, int x2, int y2, color_t c) = 0;

    // 1-bit-per-pixel sprite: row-major, MSB first, each row padded to a whole
    // byte. Only the set bits are painted, so drawing the same bitmap in
    // theme.bg erases exactly the silhouette and nothing around it - that is
    // the cheapest way to move a sprite.
    virtual void drawBitmap(int x, int y, const uint8_t* bitmap, int w, int h, color_t c) = 0;

    // ---- Text -------------------------------------------------------------
    // One font only (a 5x7 cell in a 6x8 box, scaled by an integer). No other
    // font is compiled into the firmware, and adding one costs flash the
    // partition does not have. size 1 = 6x8 px per character, size 2 = 12x16,
    // and so on.
    virtual void setTextSize(int size) = 0;
    virtual void setTextColor(color_t c) = 0;
    virtual void setTextDatum(Datum d) = 0;
    virtual void setCursor(int x, int y) = 0;

    // print() draws at the cursor, top-left anchored, and advances it.
    virtual void print(const char* s) = 0;
    virtual void print(int v) = 0;

    // drawString() draws once at (x,y) using the current datum. Prefer this
    // for anything positioned; it does not touch the cursor.
    virtual void drawString(const char* s, int x, int y) = 0;

    // Width in px that s would occupy at the current text size. Use it to
    // size the rectangle you erase before redrawing changing text.
    virtual int textWidth(const char* s) const = 0;
    virtual int fontHeight() const = 0;
};

} // namespace gk
