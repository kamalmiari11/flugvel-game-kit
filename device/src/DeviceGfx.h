#pragma once
#include <TFT_eSPI.h>
#include "gamekit/Gfx.h"

// gk::Gfx on top of the real display driver. Every call is a straight
// pass-through: the API was chosen to be the driver's own subset precisely so
// this file could stay this dull, and so a game cannot accidentally depend on
// something the device does not have.
class DeviceGfx : public gk::Gfx {
public:
    explicit DeviceGfx(TFT_eSPI* tft) : _tft(tft) {}

    int width() const override { return _tft->width(); }
    int height() const override { return _tft->height(); }

    void fillRect(int x, int y, int w, int h, gk::color_t c) override { _tft->fillRect(x, y, w, h, c); }
    void drawRect(int x, int y, int w, int h, gk::color_t c) override { _tft->drawRect(x, y, w, h, c); }
    void drawPixel(int x, int y, gk::color_t c) override { _tft->drawPixel(x, y, c); }
    void drawFastHLine(int x, int y, int w, gk::color_t c) override { _tft->drawFastHLine(x, y, w, c); }
    void drawFastVLine(int x, int y, int h, gk::color_t c) override { _tft->drawFastVLine(x, y, h, c); }
    void drawLine(int x0, int y0, int x1, int y1, gk::color_t c) override { _tft->drawLine(x0, y0, x1, y1, c); }
    void fillCircle(int x, int y, int r, gk::color_t c) override { _tft->fillCircle(x, y, r, c); }
    void drawCircle(int x, int y, int r, gk::color_t c) override { _tft->drawCircle(x, y, r, c); }
    void fillTriangle(int x0, int y0, int x1, int y1, int x2, int y2, gk::color_t c) override {
        _tft->fillTriangle(x0, y0, x1, y1, x2, y2, c);
    }
    void drawBitmap(int x, int y, const uint8_t* bmp, int w, int h, gk::color_t c) override {
        _tft->drawBitmap(x, y, bmp, w, h, c);
    }

    void setTextSize(int s) override { _tft->setTextSize(s); }
    void setTextColor(gk::color_t c) override { _tft->setTextColor(c); }
    void setTextDatum(gk::Datum d) override { _tft->setTextDatum((uint8_t)d); }
    void setCursor(int x, int y) override { _tft->setCursor(x, y); }
    void print(const char* s) override { _tft->print(s); }
    void print(int v) override { _tft->print(v); }
    void drawString(const char* s, int x, int y) override { _tft->drawString(s, x, y); }
    int textWidth(const char* s) const override { return _tft->textWidth(s); }
    int fontHeight() const override { return _tft->fontHeight(); }

private:
    TFT_eSPI* _tft;
};
