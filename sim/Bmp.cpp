#include "Bmp.h"

namespace sim {

static void put16(std::vector<uint8_t>& v, uint16_t x) { v.push_back(x & 0xFF); v.push_back(x >> 8); }
static void put32(std::vector<uint8_t>& v, uint32_t x) {
    v.push_back(x & 0xFF); v.push_back((x >> 8) & 0xFF);
    v.push_back((x >> 16) & 0xFF); v.push_back((x >> 24) & 0xFF);
}

std::vector<uint8_t> encodeBmp(const std::vector<uint8_t>& rgb, int w, int h) {
    const int rowBytes = ((w * 3) + 3) & ~3;          // rows are 4-byte aligned
    const uint32_t pixelBytes = (uint32_t)rowBytes * h;
    std::vector<uint8_t> out;
    out.reserve(54 + pixelBytes);

    out.push_back('B'); out.push_back('M');
    put32(out, 54 + pixelBytes);
    put16(out, 0); put16(out, 0);
    put32(out, 54);
    put32(out, 40);
    put32(out, (uint32_t)w);
    put32(out, (uint32_t)h);
    put16(out, 1); put16(out, 24);
    put32(out, 0); put32(out, pixelBytes);
    put32(out, 2835); put32(out, 2835);
    put32(out, 0); put32(out, 0);

    // BMP rows run bottom-up, and the channel order is BGR.
    for (int y = h - 1; y >= 0; y--) {
        for (int x = 0; x < w; x++) {
            size_t i = ((size_t)y * w + x) * 3;
            out.push_back(rgb[i + 2]);
            out.push_back(rgb[i + 1]);
            out.push_back(rgb[i + 0]);
        }
        for (int pad = w * 3; pad < rowBytes; pad++) out.push_back(0);
    }
    return out;
}

std::string base64(const std::vector<uint8_t>& data) {
    static const char* T = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";
    std::string out;
    out.reserve(((data.size() + 2) / 3) * 4);
    size_t i = 0;
    for (; i + 2 < data.size(); i += 3) {
        uint32_t n = (data[i] << 16) | (data[i + 1] << 8) | data[i + 2];
        out += T[(n >> 18) & 63]; out += T[(n >> 12) & 63];
        out += T[(n >> 6) & 63];  out += T[n & 63];
    }
    if (i + 1 == data.size()) {
        uint32_t n = data[i] << 16;
        out += T[(n >> 18) & 63]; out += T[(n >> 12) & 63]; out += "==";
    } else if (i + 2 == data.size()) {
        uint32_t n = (data[i] << 16) | (data[i + 1] << 8);
        out += T[(n >> 18) & 63]; out += T[(n >> 12) & 63]; out += T[(n >> 6) & 63]; out += '=';
    }
    return out;
}

} // namespace sim
