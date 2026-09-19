#pragma once
// Stand-in for Bodmer/TFT_eSPI - only what the harness calls, with the
// upstream signatures.
#include <Arduino.h>

// These really are preprocessor macros upstream, not enumerators. That is why
// gk::Datum uses short scoped names: an enum called TL_DATUM does not survive
// being macro-expanded. The values below are the ones gk::Datum maps onto.
#define TL_DATUM 0
#define TC_DATUM 1
#define TR_DATUM 2
#define ML_DATUM 3
#define MC_DATUM 4
#define MR_DATUM 5
#define BL_DATUM 6
#define BC_DATUM 7
#define BR_DATUM 8

#ifndef TFT_BL
#define TFT_BL 27
#endif

class TFT_eSPI {
public:
    void init();
    void setRotation(uint8_t r);
    void setTextFont(uint8_t font);
    int16_t width();
    int16_t height();

    void fillScreen(uint32_t color);
    void fillRect(int32_t x, int32_t y, int32_t w, int32_t h, uint32_t color);
    void drawRect(int32_t x, int32_t y, int32_t w, int32_t h, uint32_t color);
    void drawPixel(int32_t x, int32_t y, uint32_t color);
    void drawFastHLine(int32_t x, int32_t y, int32_t w, uint32_t color);
    void drawFastVLine(int32_t x, int32_t y, int32_t h, uint32_t color);
    void drawLine(int32_t xs, int32_t ys, int32_t xe, int32_t ye, uint32_t color);
    void fillCircle(int32_t x, int32_t y, int32_t r, uint32_t color);
    void drawCircle(int32_t x, int32_t y, int32_t r, uint32_t color);
    void fillTriangle(int32_t x1, int32_t y1, int32_t x2, int32_t y2,
                      int32_t x3, int32_t y3, uint32_t color);
    void drawBitmap(int16_t x, int16_t y, const uint8_t* bitmap,
                    int16_t w, int16_t h, uint16_t color);

    void setTextSize(uint8_t size);
    void setTextColor(uint16_t color);
    void setTextDatum(uint8_t datum);
    void setCursor(int16_t x, int16_t y);
    void print(const char* s);
    void print(int v);

    int16_t drawString(const char* string, int32_t x, int32_t y);
    int16_t drawString(const String& string, int32_t x, int32_t y);
    int16_t textWidth(const char* string);
    int16_t fontHeight();
};
