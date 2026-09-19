#pragma once
// Stand-in for espressif/arduino-esp32 libraries/Preferences.
#include <Arduino.h>

class Preferences {
public:
    bool     begin(const char* name, bool readOnly = false, const char* partitionLabel = nullptr);
    void     end();
    int32_t  getInt(const char* key, int32_t defaultValue = 0);
    size_t   putInt(const char* key, int32_t value);
};
