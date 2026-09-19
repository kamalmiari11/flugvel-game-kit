#pragma once
#include "gamekit/Gfx.h"
#include "gamekit/Game.h"
#include <cstdio>
#include <string>
#include <vector>
#include <cstring>

namespace sim {

// Wraps the real renderer and watches what the game does with it: what it
// painted outside the area it owns, and how much it painted per frame.
//
// This exists because both mistakes are invisible on a desktop and obvious
// on the device. Painting over the chrome looks fine until the host repaints
// it a frame later and your sprite is sliced in half; painting too much per
// frame looks fine at 60 fps on a laptop and turns into visible tearing plus
// a knob that answers a third of a second late on a 40 MHz SPI bus.
struct Violation {
    std::string op;      // which call did it
    std::string detail;
    int x, y, w, h;
};

class Recorder : public gk::Gfx {
public:
    Recorder(gk::Gfx& inner, gk::Rect allowed) : _in(inner), _allowed(allowed) {}

    // Pixels written since the last resetFrame(), as an upper bound (a call's
    // bounding box). At 40 MHz and 16 bits per pixel the panel eats roughly
    // 2,500 pixels per millisecond, so this number divided by 2500 is the
    // milliseconds the frame cost the main loop.
    long framePixels() const { return _framePixels; }
    void resetFrame() { _framePixels = 0; }

    const std::vector<Violation>& violations() const { return _violations; }
    void clearViolations() { _violations.clear(); }
    long totalPixels() const { return _totalPixels; }
    long peakFramePixels() const { return _peakFrame; }
    void notePeak() { if (_framePixels > _peakFrame) _peakFrame = _framePixels; }

    int width() const override { return _in.width(); }
    int height() const override { return _in.height(); }

    void fillRect(int x, int y, int w, int h, gk::color_t c) override {
        note("fillRect", x, y, w, h);
        if (x <= 0 && y <= 0 && w >= _in.width() && h >= _in.height())
            _violations.push_back({"fillRect", "full-screen fill - the chrome is not yours to erase", x, y, w, h});
        _in.fillRect(x, y, w, h, c);
    }
    void drawRect(int x, int y, int w, int h, gk::color_t c) override {
        note("drawRect", x, y, w, h); _in.drawRect(x, y, w, h, c);
    }
    void drawPixel(int x, int y, gk::color_t c) override {
        note("drawPixel", x, y, 1, 1); _in.drawPixel(x, y, c);
    }
    void drawFastHLine(int x, int y, int w, gk::color_t c) override {
        note("drawFastHLine", x, y, w, 1); _in.drawFastHLine(x, y, w, c);
    }
    void drawFastVLine(int x, int y, int h, gk::color_t c) override {
        note("drawFastVLine", x, y, 1, h); _in.drawFastVLine(x, y, h, c);
    }
    void drawLine(int x0, int y0, int x1, int y1, gk::color_t c) override {
        int x = x0 < x1 ? x0 : x1, y = y0 < y1 ? y0 : y1;
        note("drawLine", x, y, (x0 > x1 ? x0 - x1 : x1 - x0) + 1, (y0 > y1 ? y0 - y1 : y1 - y0) + 1);
        _in.drawLine(x0, y0, x1, y1, c);
    }
    void fillCircle(int cx, int cy, int r, gk::color_t c) override {
        note("fillCircle", cx - r, cy - r, 2 * r + 1, 2 * r + 1); _in.fillCircle(cx, cy, r, c);
    }
    void drawCircle(int cx, int cy, int r, gk::color_t c) override {
        note("drawCircle", cx - r, cy - r, 2 * r + 1, 2 * r + 1); _in.drawCircle(cx, cy, r, c);
    }
    void fillTriangle(int x0, int y0, int x1, int y1, int x2, int y2, gk::color_t c) override {
        int minx = x0 < x1 ? (x0 < x2 ? x0 : x2) : (x1 < x2 ? x1 : x2);
        int maxx = x0 > x1 ? (x0 > x2 ? x0 : x2) : (x1 > x2 ? x1 : x2);
        int miny = y0 < y1 ? (y0 < y2 ? y0 : y2) : (y1 < y2 ? y1 : y2);
        int maxy = y0 > y1 ? (y0 > y2 ? y0 : y2) : (y1 > y2 ? y1 : y2);
        note("fillTriangle", minx, miny, maxx - minx + 1, maxy - miny + 1);
        _in.fillTriangle(x0, y0, x1, y1, x2, y2, c);
    }
    void drawBitmap(int x, int y, const uint8_t* bmp, int w, int h, gk::color_t c) override {
        note("drawBitmap", x, y, w, h); _in.drawBitmap(x, y, bmp, w, h, c);
    }

    void setTextSize(int s) override { _size = s < 1 ? 1 : s; _in.setTextSize(s); }
    void setTextColor(gk::color_t c) override { _in.setTextColor(c); }
    void setTextDatum(gk::Datum d) override { _datum = d; _in.setTextDatum(d); }
    void setCursor(int x, int y) override { _cx = x; _cy = y; _in.setCursor(x, y); }

    void print(const char* s) override {
        int w = _in.textWidth(s);
        note("print", _cx, _cy, w, 8 * _size);
        _cx += w;
        _in.print(s);
    }
    void print(int v) override {
        char buf[16]; snprintf(buf, sizeof(buf), "%d", v);
        print((const char*)buf);
    }
    void drawString(const char* s, int x, int y) override {
        int w = _in.textWidth(s), h = 8 * _size;
        int ax = x, ay = y;
        int dh = (int)_datum % 3, dv = (int)_datum / 3;
        if (dh == 1) ax = x - w / 2; else if (dh == 2) ax = x - w;
        if (dv == 1) ay = y - h / 2; else if (dv == 2) ay = y - h;
        note("drawString", ax, ay, w, h);
        _in.drawString(s, x, y);
    }
    int textWidth(const char* s) const override { return _in.textWidth(s); }
    int fontHeight() const override { return _in.fontHeight(); }

private:
    void note(const char* op, int x, int y, int w, int h) {
        if (w <= 0 || h <= 0) return;

        // Clip to the panel first, because the driver does. Sending a sprite
        // that is half off the right-hand edge is normal and costs nothing for
        // the part that misses - objects are routinely spawned off-screen and
        // walked in. Only what lands ON the glass is counted or judged.
        int x0 = x < 0 ? 0 : x;
        int y0 = y < 0 ? 0 : y;
        int x1 = x + w, y1 = y + h;
        if (x1 > _in.width()) x1 = _in.width();
        if (y1 > _in.height()) y1 = _in.height();
        if (x1 <= x0 || y1 <= y0) return;          // entirely off-panel

        _framePixels += (long)(x1 - x0) * (y1 - y0);
        _totalPixels += (long)(x1 - x0) * (y1 - y0);
        if (!_watch) return;

        // What is left is on the glass. If any of it is outside the play area
        // it is in the chrome, which the host repaints over you.
        bool outside = x0 < _allowed.x || y0 < _allowed.y ||
                       x1 > _allowed.x + _allowed.w || y1 > _allowed.y + _allowed.h;
        if (outside) {
            char msg[192];
            snprintf(msg, sizeof(msg),
                     "painted (%d,%d)-(%d,%d) on screen, outside the play area (%d,%d) %dx%d",
                     x0, y0, x1, y1, _allowed.x, _allowed.y, _allowed.w, _allowed.h);
            _violations.push_back({op, msg, x0, y0, x1 - x0, y1 - y0});
        }
    }

public:
    // The host draws its own chrome through this same object; that drawing is
    // legitimately outside the play area, so bounds checking is switched off
    // around it. Pixel counting stays on - the chrome costs the bus too.
    void setWatching(bool on) { _watch = on; }

private:
    gk::Gfx& _in;
    gk::Rect _allowed;
    std::vector<Violation> _violations;
    long _framePixels = 0;
    long _totalPixels = 0;
    long _peakFrame = 0;
    bool _watch = true;
    int _size = 1, _cx = 0, _cy = 0;
    gk::Datum _datum = gk::Datum::TL;
};

} // namespace sim
