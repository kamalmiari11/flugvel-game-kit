#pragma once
#include <stdint.h>
#include <string>
#include <vector>

namespace sim {

// 24-bit BMP, which browsers display and every image tool opens, and which
// needs no compression library to write.
std::vector<uint8_t> encodeBmp(const std::vector<uint8_t>& rgb, int w, int h);
std::string base64(const std::vector<uint8_t>& data);

} // namespace sim
