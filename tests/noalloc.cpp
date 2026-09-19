#include "framework.h"
#include "gamekit/Game.h"
#include "SoftGfx.h"
#include "registry.h"
#include <cstdlib>
#include <cstring>
#include <new>

// The device has no memory to spare and no allocator worth trusting for
// realtime work: the heap is shared with the Wi-Fi stack and the TLS buffers,
// it fragments, and a failed allocation mid-game is a reboot. So the rule is
// simply that a game allocates nothing, ever - not a String, not a vector, not
// a `new` anywhere.
//
// This file makes that testable by counting every allocation in the process,
// then driving the games directly through a host that allocates nothing
// itself, so anything counted came from the game.

namespace {

bool   gArmed = false;
long   gAllocations = 0;

struct Armed {
    Armed()  { gAllocations = 0; gArmed = true; }
    ~Armed() { gArmed = false; }
};

// A host with no std::string, no containers, and no clock of its own.
class BareHost : public gk::Host {
public:
    explicit BareHost(gk::Rect area) : _area(area), _theme(&gk::themes::byIndex(0)) {}

    gk::Rect play() const override { return _area; }
    const gk::Theme& theme() const override { return *_theme; }
    uint32_t millis() const override { return _now; }
    uint32_t random(uint32_t n) override {
        _rng ^= _rng << 13; _rng ^= _rng >> 17; _rng ^= _rng << 5;
        return n ? _rng % n : 0;
    }
    int bestScore() const override { return _best; }
    void submitScore(int s) override { if (s > _best) _best = s; _submits++; }
    void setLegend(const char* knob, const char* button) override {
        copy(_knob, sizeof(_knob), knob);
        copy(_button, sizeof(_button), button);
    }
    void playTone(uint16_t freqHz, uint16_t ms) override { _notes++; (void)freqHz; (void)ms; }
    void stopSound() override {}
    bool soundEnabled() const override { return true; }
    void exitGame() override { _exited = true; }

    void advance(uint32_t ms) { _now += ms; }
    int submits() const { return _submits; }
    bool exited() const { return _exited; }

private:
    static void copy(char* dst, size_t n, const char* src) {
        if (!src) { dst[0] = '\0'; return; }
        strncpy(dst, src, n - 1);
        dst[n - 1] = '\0';
    }
    gk::Rect _area;
    const gk::Theme* _theme;
    uint32_t _now = 0;
    uint32_t _rng = 2463534242u;
    int _best = 0, _submits = 0, _notes = 0;
    bool _exited = false;
    char _knob[32] = {0}, _button[32] = {0};
};

} // namespace

void* operator new(size_t n) {
    if (gArmed) gAllocations++;
    void* p = malloc(n ? n : 1);
    if (!p) throw std::bad_alloc();
    return p;
}
void* operator new[](size_t n) { return operator new(n); }
void operator delete(void* p) noexcept { free(p); }
void operator delete[](void* p) noexcept { free(p); }
void operator delete(void* p, size_t) noexcept { free(p); }
void operator delete[](void* p, size_t) noexcept { free(p); }

TEST(games_allocate_nothing_while_running) {
    for (int g = 0; g < games::count(); g++) {
        gk::Game& game = *games::all()[g].game;

        // Everything that legitimately allocates is built BEFORE arming.
        sim::SoftGfx screen;
        BareHost host({ 0, 20, sim::SoftGfx::W, sim::SoftGfx::H - 50 });

        long allocations;
        {
            Armed armed;
            game.start(host);
            game.repaint(host, screen);
            uint32_t rng = 7;
            for (int i = 0; i < 500; i++) {
                rng ^= rng << 13; rng ^= rng >> 17; rng ^= rng << 5;
                if (rng % 5 == 0) game.onKnob(host, (rng % 2) ? 1 : -1);
                if (rng % 11 == 0) game.onButton(host);
                host.advance(50 + rng % 40);
                game.update(host, screen);
            }
            allocations = gAllocations;
        }

        CHECK_EQ(allocations, 0L, std::string(games::all()[g].key) +
                 " allocated memory while running - no String, no vector, no new");
    }
}
