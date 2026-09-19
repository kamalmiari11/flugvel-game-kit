#pragma once
#include <stdint.h>

// RGB565 colour, and the one non-obvious thing about this panel.
//
// THE PANEL LIES. It is a no-name ST7789 clone that displays
// swapRB(~value): it inverts every channel and swaps red with blue. Send it
// "red" and you see cyan. Nothing in the driver fixes this, so every colour
// in the firmware is pre-distorted on the way out, and you must do the same.
//
// Never write a raw 16-bit literal in game code. Write the colour you want
// to SEE and let PANEL_HEX do the transform:
//
//     gfx.fillRect(x, y, w, h, gk::PANEL_HEX(0xff0000));   // looks red
//     gfx.fillRect(x, y, w, h, 0xf800);                    // looks cyan (wrong)
//
// In practice you should not be picking colours at all - take them from the
// theme the host hands you (see Theme.h), which is already correct. This
// header is here for the rare case where a game needs a colour of its own.

namespace gk {

using color_t = uint16_t;

// Plain RGB565 packing - what a normal display would take.
constexpr color_t rgb565(uint8_t r, uint8_t g, uint8_t b) {
    return (color_t)(((r & 0xF8) << 8) | ((g & 0xFC) << 3) | ((b & 0xF8) >> 3));
}

// The value to SEND so the panel SHOWS rgb(r,g,b): invert, and swap R/B.
constexpr color_t panelRgb(uint8_t r, uint8_t g, uint8_t b) {
    return (color_t)(~rgb565(b, g, r) & 0xFFFF);
}

constexpr color_t panelHex(uint32_t hex) {
    return panelRgb((uint8_t)((hex >> 16) & 0xFF), (uint8_t)((hex >> 8) & 0xFF), (uint8_t)(hex & 0xFF));
}

// The inverse, used by the simulator to work out what a sent value actually
// looks like on the glass. Game code has no reason to call this.
constexpr color_t panelDecode(color_t sent) {
    return (color_t)((((~sent) & 0x001F) << 11) | (((~sent) & 0x07E0)) | ((((~sent) & 0xF800) >> 11)));
}

} // namespace gk

// The firmware this game ends up in may already define PANEL_HEX with the
// same meaning, so do not fight it.
#ifndef PANEL_HEX
#define PANEL_HEX(h) ::gk::panelHex(h)
#endif

// Pin the rule to values measured on the real panel. If one of these ever
// fails, the hardware changed and every theme colour needs re-deriving -
// it is not something to "fix" by editing the expected value.
static_assert(gk::panelHex(0x000000) == 0xFFFF, "panel rule: black");
static_assert(gk::panelHex(0xFFFFFF) == 0x0000, "panel rule: white");
static_assert(gk::panelHex(0xFF0000) == 0xFFE0, "panel rule: red");
static_assert(gk::panelHex(0x0000FF) == 0x07FF, "panel rule: blue");
static_assert(gk::panelHex(0x00FFFF) == 0x001F, "panel rule: cyan");
static_assert(gk::panelHex(0xFFFF00) == 0xF800, "panel rule: yellow");
static_assert(gk::panelDecode(gk::panelHex(0xFF0000)) == gk::rgb565(0xFF, 0, 0), "decode round-trips");
