#pragma once
#include <cstdio>
#include <string>
#include <vector>

// A test framework in forty lines, because pulling in a real one would be the
// only dependency in this kit. Write a test with TEST(name) { ... } and assert
// with CHECK(cond, why) / CHECK_EQ(a, b, why).

namespace t {

struct Case { const char* name; void (*fn)(); };
std::vector<Case>& cases();
extern int failures;
extern const char* current;

inline void fail(const char* file, int line, const std::string& msg) {
    failures++;
    printf("    FAIL %s:%d\n         %s\n", file, line, msg.c_str());
}

struct Register {
    Register(const char* name, void (*fn)()) { cases().push_back({name, fn}); }
};

} // namespace t

#define TEST(name)                                                    \
    static void name();                                               \
    static ::t::Register reg_##name(#name, name);                     \
    static void name()

#define CHECK(cond, why)                                              \
    do { if (!(cond)) ::t::fail(__FILE__, __LINE__,                   \
        std::string(why) + "  [" #cond "]"); } while (0)

#define CHECK_EQ(a, b, why)                                           \
    do { auto _a = (a); auto _b = (b);                                \
         if (!(_a == _b)) ::t::fail(__FILE__, __LINE__,               \
            std::string(why) + "  (got " + std::to_string(_a) +       \
            ", expected " + std::to_string(_b) + ")"); } while (0)

#define CHECK_LE(a, b, why)                                           \
    do { auto _a = (a); auto _b = (b);                                \
         if (!(_a <= _b)) ::t::fail(__FILE__, __LINE__,               \
            std::string(why) + "  (" + std::to_string(_a) +           \
            " > " + std::to_string(_b) + ")"); } while (0)
