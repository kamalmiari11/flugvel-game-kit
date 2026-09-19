#pragma once
// Stand-in for madhephaestus/ESP32Encoder.
#include <Arduino.h>

// A scoped enum upstream, so puType::up is the correct spelling.
enum class puType { up, down, none };

class ESP32Encoder {
public:
    static puType useInternalWeakPullResistors;
    void    attachFullQuad(int aPinNumber, int bPinNumber);
    void    attachHalfQuad(int aPinNumber, int bPinNumber);
    int64_t getCount();          // note: 64-bit
    int64_t clearCount();
    int64_t setCount(int64_t value);
};
