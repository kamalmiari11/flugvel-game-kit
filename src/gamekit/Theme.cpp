#include "gamekit/Theme.h"

namespace gk {
namespace themes {

// These are the real device palettes, panel-compensated. The numbers passed
// to panelHex() are the colours as the eye sees them on the glass; contrast
// ratios were measured against the background, which is why they are not
// round numbers. Do not "tidy" them.

const Theme Light = {
    "Light",
    panelHex(0x9cbf6a),  // bg
    panelHex(0x1a2010),  // fg          8.0:1 on bg
    panelHex(0x3a4a22),  // fgDim       4.6:1
    panelHex(0x6f8c45),  // rule
    panelHex(0x23271d),  // selectBg
    panelHex(0xe6f0d0),  // selectFg   12.9:1 on selectBg
    panelHex(0xd2692a),  // selectMark  4.2:1 on selectBg
    panelHex(0x9a3f0a),  // accent      3.3:1
    panelHex(0x1c4410),  // accent2     5.3:1
    panelHex(0x7a1a08),  // danger      5.1:1
    12,
};

const Theme Dark = {
    "Dark",
    panelHex(0x0d0f10),  // bg
    panelHex(0xe9f7f9),  // fg         17.5:1
    panelHex(0x8fb4ba),  // fgDim       8.6:1
    panelHex(0x2b3a3d),  // rule
    panelHex(0x9edbe6),  // selectBg
    panelHex(0x0d1214),  // selectFg
    panelHex(0x0d1214),  // selectMark
    panelHex(0xff9a4d),  // accent      6.0:1
    panelHex(0x8fe38f),  // accent2     9.1:1
    panelHex(0xff8a80),  // danger      7.4:1
    12,
};

static const Theme* kAll[] = { &Light, &Dark };

const Theme& byIndex(int i) {
    if (i < 0 || i >= (int)(sizeof(kAll) / sizeof(kAll[0]))) return Light;
    return *kAll[i];
}

int count() { return (int)(sizeof(kAll) / sizeof(kAll[0])); }

} // namespace themes
} // namespace gk
