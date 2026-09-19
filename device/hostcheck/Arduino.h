#pragma once
// Stand-in for the Arduino-ESP32 core (release/v2.x). Declarations only.
#include <cstdarg>
#include <cstdint>
#include <cstdio>
#include <cstring>
#include <string>

#define HIGH 1
#define LOW 0
#define OUTPUT 1
#define INPUT 0
#define INPUT_PULLUP 2

unsigned long millis();
unsigned long micros();
void delay(uint32_t ms);
void pinMode(uint8_t pin, uint8_t mode);
void digitalWrite(uint8_t pin, uint8_t val);
int  digitalRead(uint8_t pin);

// esp32-hal-ledc.h - core 2.x signatures. Core 3.x replaced these with
// ledcAttach()/ledcWrite(pin, ...), which is why the platform is pinned.
uint32_t ledcSetup(uint8_t channel, uint32_t freq, uint8_t resolutionBits);
void     ledcWrite(uint8_t channel, uint32_t duty);
uint32_t ledcWriteTone(uint8_t channel, uint32_t freq);
void     ledcAttachPin(uint8_t pin, uint8_t channel);

class String {
public:
    String() {}
    String(const char* s) : _s(s ? s : "") {}
    String operator+(const char* o) const { String r; r._s = _s + (o ? o : ""); return r; }
    const char* c_str() const { return _s.c_str(); }
    operator const char*() const { return _s.c_str(); }
private:
    std::string _s;
};

class HardwareSerial {
public:
    void begin(unsigned long baud);
    void printf(const char* fmt, ...);
    void println(const char* s);
};
extern HardwareSerial Serial;
