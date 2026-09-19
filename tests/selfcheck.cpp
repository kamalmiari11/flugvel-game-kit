#include "framework.h"
#include "Harness.h"
#include "Budget.h"

// A test suite that cannot fail is decoration. This file keeps a deliberately
// broken game around and asserts that each check catches it - so if someone
// later softens a rule, or a refactor quietly stops the recorder from
// watching, these go red instead of everything going green.

using namespace sim;

namespace {

// Does three forbidden things: paints over the status bar, clears the whole
// play area every single frame, and cannot rebuild its screen from state.
class BadGame : public gk::Game {
public:
    const char* name() const override { return "Bad"; }
    void start(gk::Host& host) override { _area = host.play(); _tick = 0; }
    void repaint(gk::Host& host, gk::Gfx& gfx) override {
        // Wrong on purpose: paints a fixed picture rather than current state.
        gfx.fillRect(_area.x, _area.y, _area.w, _area.h, host.theme().bg);
    }
    void update(gk::Host& host, gk::Gfx& gfx) override {
        const gk::Theme& t = host.theme();
        gfx.fillRect(_area.x, _area.y, _area.w, _area.h, t.bg);       // whole area, every frame
        gfx.fillRect(10, 2, 40, 12, t.accent);                         // into the status bar
        gfx.fillRect(_area.x + 20, _area.y + 20 + (_tick % 40), 16, 16, t.fg);
        host.playTone(1000, 200);                                      // a beep every frame
        _tick++;
    }
    void onKnob(gk::Host&, int) override {}
    void onButton(gk::Host&) override {}
private:
    gk::Rect _area {0, 0, 0, 0};
    int _tick = 0;
};

} // namespace

TEST(the_bench_catches_drawing_in_the_chrome) {
    BadGame bad;
    HarnessConfig cfg;
    Harness h(bad, cfg);
    h.begin();
    for (int i = 0; i < 20; i++) h.step(50);

    CHECK(!h.recorder().violations().empty(),
          "a game painting over the status bar went unnoticed - the recorder is not watching");
}

TEST(the_bench_catches_a_blown_frame_budget) {
    BadGame bad;
    HarnessConfig cfg;
    Harness h(bad, cfg);
    h.begin();

    long heavy = 0;
    for (int i = 0; i < 40; i++) {
        h.step(50);
        if (h.recorder().framePixels() > FRAME_PIXELS_FAIL) heavy++;
    }
    CHECK(heavy > 30, "clearing the whole play area every frame should blow the budget every frame");
}

TEST(the_bench_catches_a_game_that_floods_the_buzzer) {
    BadGame bad;
    HarnessConfig cfg;
    Harness h(bad, cfg);
    h.begin();
    for (int i = 0; i < 40; i++) h.step(50);

    CHECK(h.buzzer().dropped() > 0,
          "a beep on every frame should overrun the queue - the buzzer model is not counting drops");
}

TEST(the_bench_catches_a_repaint_that_does_not_rebuild_the_screen) {
    BadGame bad;
    HarnessConfig cfg;
    Harness h(bad, cfg);
    h.begin();
    for (int i = 0; i < 10; i++) h.step(50);

    gk::Rect p = h.play();
    std::vector<uint16_t> before;
    for (int y = p.y; y < p.bottom(); y++)
        for (int x = p.x; x < p.right(); x++) before.push_back(h.screen().pixel(x, y));

    h.screen().fillRect(p.x, p.y, p.w, p.h, 0x1234);
    h.forceRepaint();

    size_t diff = 0, i = 0;
    for (int y = p.y; y < p.bottom(); y++)
        for (int x = p.x; x < p.right(); x++, i++)
            if (h.screen().pixel(x, y) != before[i]) diff++;

    CHECK(diff > 0, "a repaint() that throws the game's state away should not match what was on screen");
}
