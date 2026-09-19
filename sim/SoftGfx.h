#pragma once
#include "gamekit/Gfx.h"
#include <stdint.h>
#include <vector>

namespace sim {

// A software implementation of the device's drawing API over a plain
// 320x240 RGB565 framebuffer. The panel itself is a 240x320 portrait part,
// but the firmware runs it rotated into landscape - 320 across, 240 down -
// so that is what a game sees and what this reproduces. Values are stored exactly as a game sends
// them - the panel's invert-and-swap is applied only when a frame is
// converted for display (see Framebuffer::toRgb888), which is what makes
// the simulator show the same colours the hardware would.
class SoftGfx : public gk::Gfx {
public:
    static const int W = 320;
    static const int H = 240;

    SoftGfx();

    int width() const override { return W; }
    int height() const override { return H; }

    void fillRect(int x, int y, int w, int h, gk::color_t c) override;
    void drawRect(int x, int y, int w, int h, gk::color_t c) override;
    void drawPixel(int x, int y, gk::color_t c) override;
    void drawFastHLine(int x, int y, int w, gk::color_t c) override;
    void drawFastVLine(int x, int y, int h, gk::color_t c) override;
    void drawLine(int x0, int y0, int x1, int y1, gk::color_t c) override;
    void fillCircle(int cx, int cy, int r, gk::color_t c) override;
    void drawCircle(int cx, int cy, int r, gk::color_t c) override;
    void fillTriangle(int x0, int y0, int x1, int y1, int x2, int y2, gk::color_t c) override;
    void drawBitmap(int x, int y, const uint8_t* bitmap, int w, int h, gk::color_t c) override;

    void setTextSize(int size) override { _textSize = size < 1 ? 1 : size; }
    void setTextColor(gk::color_t c) override { _textColor = c; }
    void setTextDatum(gk::Datum d) override { _datum = d; }
    void setCursor(int x, int y) override { _cx = x; _cy = y; }
    void print(const char* s) override;
    void print(int v) override;
    void drawString(const char* s, int x, int y) override;
    int textWidth(const char* s) const override;
    int fontHeight() const override { return 8 * _textSize; }

    // Raw pixels, as sent (not panel-decoded).
    const uint16_t* pixels() const { return _fb.data(); }
    uint16_t pixel(int x, int y) const { return _fb[(size_t)y * W + x]; }

    // 24-bit RGB of what the glass would actually show, row-major, 3 bytes
    // per pixel. This is where the panel transform is applied.
    void toRgb888(std::vector<uint8_t>& out) const;

private:
    void drawChar(char c, int x, int y);

    std::vector<uint16_t> _fb;
    int _textSize = 1;
    gk::color_t _textColor = 0;
    gk::Datum _datum = gk::Datum::TL;
    int _cx = 0, _cy = 0;
};

} // namespace sim
