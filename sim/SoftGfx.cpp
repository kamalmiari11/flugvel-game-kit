#include "SoftGfx.h"
#include "Font5x7.h"
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <algorithm>

namespace sim {

SoftGfx::SoftGfx() : _fb((size_t)W * H, 0) {}

void SoftGfx::drawPixel(int x, int y, gk::color_t c) {
    if (x < 0 || y < 0 || x >= W || y >= H) return;   // the panel clips too
    _fb[(size_t)y * W + x] = c;
}

void SoftGfx::fillRect(int x, int y, int w, int h, gk::color_t c) {
    if (w <= 0 || h <= 0) return;
    int x0 = std::max(0, x), y0 = std::max(0, y);
    int x1 = std::min(W, x + w), y1 = std::min(H, y + h);
    for (int yy = y0; yy < y1; yy++)
        for (int xx = x0; xx < x1; xx++)
            _fb[(size_t)yy * W + xx] = c;
}

void SoftGfx::drawRect(int x, int y, int w, int h, gk::color_t c) {
    if (w <= 0 || h <= 0) return;
    drawFastHLine(x, y, w, c);
    drawFastHLine(x, y + h - 1, w, c);
    drawFastVLine(x, y, h, c);
    drawFastVLine(x + w - 1, y, h, c);
}

void SoftGfx::drawFastHLine(int x, int y, int w, gk::color_t c) { fillRect(x, y, w, 1, c); }
void SoftGfx::drawFastVLine(int x, int y, int h, gk::color_t c) { fillRect(x, y, 1, h, c); }

void SoftGfx::drawLine(int x0, int y0, int x1, int y1, gk::color_t c) {
    int dx = abs(x1 - x0), sx = x0 < x1 ? 1 : -1;
    int dy = -abs(y1 - y0), sy = y0 < y1 ? 1 : -1;
    int err = dx + dy;
    for (;;) {
        drawPixel(x0, y0, c);
        if (x0 == x1 && y0 == y1) break;
        int e2 = 2 * err;
        if (e2 >= dy) { err += dy; x0 += sx; }
        if (e2 <= dx) { err += dx; y0 += sy; }
    }
}

void SoftGfx::fillCircle(int cx, int cy, int r, gk::color_t c) {
    if (r < 0) return;
    for (int dy = -r; dy <= r; dy++) {
        int span = (int)(__builtin_sqrt((double)(r * r - dy * dy)));
        drawFastHLine(cx - span, cy + dy, 2 * span + 1, c);
    }
}

void SoftGfx::drawCircle(int cx, int cy, int r, gk::color_t c) {
    int x = r, y = 0, err = 1 - r;
    while (x >= y) {
        drawPixel(cx + x, cy + y, c); drawPixel(cx + y, cy + x, c);
        drawPixel(cx - y, cy + x, c); drawPixel(cx - x, cy + y, c);
        drawPixel(cx - x, cy - y, c); drawPixel(cx - y, cy - x, c);
        drawPixel(cx + y, cy - x, c); drawPixel(cx + x, cy - y, c);
        y++;
        if (err < 0) err += 2 * y + 1;
        else { x--; err += 2 * (y - x) + 1; }
    }
}

void SoftGfx::fillTriangle(int x0, int y0, int x1, int y1, int x2, int y2, gk::color_t c) {
    // Same approach as the driver: sort by y, then walk the scanlines.
    if (y0 > y1) { std::swap(y0, y1); std::swap(x0, x1); }
    if (y1 > y2) { std::swap(y1, y2); std::swap(x1, x2); }
    if (y0 > y1) { std::swap(y0, y1); std::swap(x0, x1); }
    if (y2 == y0) {
        drawFastHLine(std::min(x0, std::min(x1, x2)), y0,
                      std::max(x0, std::max(x1, x2)) - std::min(x0, std::min(x1, x2)) + 1, c);
        return;
    }
    auto edge = [](int ya, int xa, int yb, int xb, int y) {
        if (yb == ya) return xa;
        return xa + (int)((long)(xb - xa) * (y - ya) / (yb - ya));
    };
    for (int y = y0; y <= y2; y++) {
        int xa = edge(y0, x0, y2, x2, y);
        int xb = (y < y1) ? edge(y0, x0, y1, x1, y) : edge(y1, x1, y2, x2, y);
        if (xa > xb) std::swap(xa, xb);
        drawFastHLine(xa, y, xb - xa + 1, c);
    }
}

void SoftGfx::drawBitmap(int x, int y, const uint8_t* bitmap, int w, int h, gk::color_t c) {
    // Row-padded to a byte, MSB first - matching the device driver exactly.
    int byteWidth = (w + 7) / 8;
    for (int j = 0; j < h; j++) {
        uint8_t byte = 0;
        for (int i = 0; i < w; i++) {
            if (i & 7) byte <<= 1;
            else byte = bitmap[j * byteWidth + i / 8];
            if (byte & 0x80) drawPixel(x + i, y + j, c);
        }
    }
}

int SoftGfx::textWidth(const char* s) const {
    if (!s) return 0;
    return (int)strlen(s) * 6 * _textSize;
}

void SoftGfx::drawChar(char c, int x, int y) {
    const uint8_t* rows = glyphRows(c);
    for (int r = 0; r < 7; r++) {
        for (int col = 0; col < 5; col++) {
            if (!(rows[r] & (1 << (4 - col)))) continue;
            if (_textSize == 1) drawPixel(x + col, y + r, _textColor);
            else fillRect(x + col * _textSize, y + r * _textSize, _textSize, _textSize, _textColor);
        }
    }
}

void SoftGfx::print(const char* s) {
    if (!s) return;
    for (const char* p = s; *p; p++) {
        if (*p == '\n') { _cx = 0; _cy += fontHeight(); continue; }
        drawChar(*p, _cx, _cy);
        _cx += 6 * _textSize;
    }
}

void SoftGfx::print(int v) {
    char buf[16];
    snprintf(buf, sizeof(buf), "%d", v);
    print(buf);
}

void SoftGfx::drawString(const char* s, int x, int y) {
    if (!s) return;
    int w = textWidth(s);
    int h = 8 * _textSize;
    int ax = x, ay = y;
    switch (_datum) {
        case gk::Datum::TL: break;
        case gk::Datum::TC: ax = x - w / 2; break;
        case gk::Datum::TR: ax = x - w; break;
        case gk::Datum::ML: ay = y - h / 2; break;
        case gk::Datum::MC: ax = x - w / 2; ay = y - h / 2; break;
        case gk::Datum::MR: ax = x - w;     ay = y - h / 2; break;
        case gk::Datum::BL: ay = y - h; break;
        case gk::Datum::BC: ax = x - w / 2; ay = y - h; break;
        case gk::Datum::BR: ax = x - w;     ay = y - h; break;
    }
    int cx = ax;
    for (const char* p = s; *p; p++) {
        drawChar(*p, cx, ay);
        cx += 6 * _textSize;
    }
}

void SoftGfx::toRgb888(std::vector<uint8_t>& out) const {
    out.resize((size_t)W * H * 3);
    for (size_t i = 0; i < _fb.size(); i++) {
        uint16_t shown = gk::panelDecode(_fb[i]);   // what the glass does to it
        uint8_t r = (uint8_t)((shown >> 11) & 0x1F);
        uint8_t g = (uint8_t)((shown >> 5) & 0x3F);
        uint8_t b = (uint8_t)(shown & 0x1F);
        out[i * 3 + 0] = (uint8_t)((r << 3) | (r >> 2));
        out[i * 3 + 1] = (uint8_t)((g << 2) | (g >> 4));
        out[i * 3 + 2] = (uint8_t)((b << 3) | (b >> 2));
    }
}

} // namespace sim
