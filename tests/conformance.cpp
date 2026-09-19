#include "framework.h"
#include "Harness.h"
#include "Budget.h"
#include "registry.h"
#include <algorithm>
#include <vector>

// The house rules, applied to every game in the registry. A game that passes
// these behaves correctly on the device; one that does not, does not - the
// checks are here because each of them corresponds to something that is
// invisible on a desktop and obvious on hardware.
//
// These run against YOUR game automatically as soon as it is in the registry.
// Do not edit them to make them pass.

using namespace sim;

namespace {

// A repeatable "player": nonsense input, but the same nonsense every run.
struct Monkey {
    uint32_t s = 99991;
    uint32_t next() { s ^= s << 13; s ^= s >> 17; s ^= s << 5; return s; }
    void drive(Harness& h, int iterations) {
        for (int i = 0; i < iterations; i++) {
            uint32_t r = next() % 100;
            if (r < 30) h.queueKnob(1);
            else if (r < 55) h.queueKnob(-1);
            else if (r < 70) h.queueButton();
            h.step(50 + next() % 40);
        }
    }
};

std::vector<uint16_t> playArea(Harness& h) {
    gk::Rect p = h.play();
    std::vector<uint16_t> out;
    out.reserve((size_t)p.w * p.h);
    for (int y = p.y; y < p.bottom(); y++)
        for (int x = p.x; x < p.right(); x++)
            out.push_back(h.screen().pixel(x, y));
    return out;
}

} // namespace

TEST(games_stay_inside_their_play_area) {
    for (int g = 0; g < games::count(); g++) {
        HarnessConfig cfg;
        Harness h(*games::all()[g].game, cfg);
        h.begin();
        Monkey().drive(h, 400);

        const auto& v = h.recorder().violations();
        if (!v.empty()) {
            printf("    %s: %zu violation(s), first: %s %s\n",
                   games::all()[g].key, v.size(), v[0].op.c_str(), v[0].detail.c_str());
        }
        CHECK(v.empty(), std::string(games::all()[g].key) +
              " drew outside the area the host gave it - the chrome will cut it in half");
    }
}

TEST(games_read_the_play_area_instead_of_hardcoding_it) {
    // Same games, a firmware with a taller status bar and a taller legend.
    // Anything that baked in 20 and 30 fails here, which is the point.
    for (int g = 0; g < games::count(); g++) {
        HarnessConfig cfg;
        cfg.headerH = 28;
        cfg.legendH = 44;
        Harness h(*games::all()[g].game, cfg);
        h.begin();
        Monkey().drive(h, 300);

        CHECK(h.recorder().violations().empty(), std::string(games::all()[g].key) +
              " hardcodes the screen layout instead of reading Host::play()");
    }
}

TEST(repaint_rebuilds_exactly_what_was_on_screen) {
    // The contract says repaint() paints the current state in full. Prove it:
    // wipe the play area, ask for a repaint, and the pixels must come back
    // identical. A game that fails this loses half its screen the first time
    // the device draws an overlay over it.
    for (int g = 0; g < games::count(); g++) {
        HarnessConfig cfg;
        Harness h(*games::all()[g].game, cfg);
        h.begin();
        Monkey().drive(h, 120);

        // Let it settle before snapshotting. A game paces its own frames, so
        // between one frame and the next there is legitimately state that has
        // not been painted yet - a knob turn that arrives mid-frame is drawn
        // on the next one, not instantly. Half a second of quiet calls closes
        // that window for any sane frame interval, and the check below stays
        // exactly as strict about what it is actually testing: that repaint()
        // can rebuild the screen from state.
        for (int i = 0; i < 8; i++) h.step(60);

        std::vector<uint16_t> before = playArea(h);

        gk::Rect p = h.play();
        h.screen().fillRect(p.x, p.y, p.w, p.h, 0x1234);   // garbage, not a theme colour
        h.forceRepaint();

        std::vector<uint16_t> after = playArea(h);
        size_t diff = 0;
        for (size_t i = 0; i < before.size() && i < after.size(); i++)
            if (before[i] != after[i]) diff++;

        CHECK(diff == 0, std::string(games::all()[g].key) +
              " could not rebuild its screen from state: " + std::to_string(diff) +
              " pixel(s) differ after repaint()");
    }
}

TEST(frames_stay_within_the_bus_budget) {
    for (int g = 0; g < games::count(); g++) {
        HarnessConfig cfg;
        Harness h(*games::all()[g].game, cfg);
        h.begin();

        std::vector<long> cost;
        Monkey m;
        for (int i = 0; i < 400; i++) {
            uint32_t r = m.next() % 100;
            if (r < 30) h.queueKnob(1);
            else if (r < 55) h.queueKnob(-1);
            else if (r < 70) h.queueButton();
            h.step(50 + m.next() % 40);
            cost.push_back(h.recorder().framePixels());
        }
        std::sort(cost.begin(), cost.end());

        long median = cost[cost.size() / 2];
        long p95 = cost[(size_t)(cost.size() * 0.95)];
        long heavy = 0;
        for (long c : cost) if (c > FRAME_PIXELS_FAIL) heavy++;

        CHECK_LE(median, FRAME_PIXELS_WARN, std::string(games::all()[g].key) +
                 " redraws too much in a typical frame");
        CHECK_LE(p95, FRAME_PIXELS_FAIL, std::string(games::all()[g].key) +
                 " has too many expensive frames - the knob will feel late");
        // A full repaint is legitimate - entering the game, restarting after a
        // loss, a theme change - and a hard game played badly restarts a lot,
        // so the allowance is a rate rather than a count: one in twenty frames,
        // about one every one and a half seconds. A game that repaints every
        // frame comes in at twenty times that and still fails outright.
        CHECK_LE(heavy, (long)cost.size() / 20 + 2, std::string(games::all()[g].key) +
                 " paints the whole play area far too often");
    }
}

TEST(a_run_is_reproducible_from_its_seed) {
    // Same seed, same input, same pixels - otherwise a recorded bug report is
    // worthless and none of the tests above mean anything.
    for (int g = 0; g < games::count(); g++) {
        HarnessConfig cfg;
        Harness a(*games::all()[g].game, cfg);
        Harness b(*games::all()[g].game, cfg);
        a.begin();
        Monkey().drive(a, 150);
        std::vector<uint16_t> pa = playArea(a);

        b.begin();
        Monkey().drive(b, 150);
        std::vector<uint16_t> pb = playArea(b);

        CHECK(pa == pb, std::string(games::all()[g].key) +
              " is not deterministic - it is using a clock or an RNG the host does not control");
    }
}

TEST(games_do_not_overflow_the_buzzer_queue) {
    // The buzzer plays one note at a time and the host holds only a handful
    // behind it. A game firing sound per frame instead of per event loses the
    // tail silently, which is how you end up with feedback that works
    // sometimes and cannot be debugged by ear.
    for (int g = 0; g < games::count(); g++) {
        HarnessConfig cfg;
        Harness h(*games::all()[g].game, cfg);
        h.begin();
        Monkey().drive(h, 400);

        CHECK_EQ(h.buzzer().dropped(), 0L, std::string(games::all()[g].key) +
                 " queued more notes than the buzzer can hold - fire sound on events, not on frames");
    }
}

TEST(games_play_identically_with_the_sound_off) {
    // The player can mute the device, and plenty do. Audio is feedback for
    // something the screen is already saying - never the only place the
    // information appears - so muting must change nothing but the noise.
    //
    // Same seed, same input, one run with sound and one without: the pixels
    // must match. A game that fails this has put something in the sound that
    // is not anywhere else, or has let the sound feed back into its state.
    for (int g = 0; g < games::count(); g++) {
        HarnessConfig loud;
        HarnessConfig quiet;
        quiet.soundOn = false;

        Harness a(*games::all()[g].game, loud);
        a.begin();
        Monkey().drive(a, 200);
        std::vector<uint16_t> pa = playArea(a);

        Harness b(*games::all()[g].game, quiet);
        b.begin();
        Monkey().drive(b, 200);
        std::vector<uint16_t> pb = playArea(b);

        CHECK(pa == pb, std::string(games::all()[g].key) +
              " plays differently with the sound off");
    }
}

TEST(input_never_arrives_faster_than_the_hardware_allows) {
    // The button is debounced at 200 ms in the firmware. Queue a press on
    // every single iteration and the game must still only see them at that
    // rate - a game designed around faster tapping is unplayable on the real
    // device.
    struct Counter : gk::Game {
        int presses = 0;
        const char* name() const override { return "Counter"; }
        void start(gk::Host&) override { presses = 0; }
        void repaint(gk::Host&, gk::Gfx&) override {}
        void update(gk::Host&, gk::Gfx&) override {}
        void onKnob(gk::Host&, int) override {}
        void onButton(gk::Host&) override { presses++; }
    } counter;

    HarnessConfig cfg;
    Harness h(counter, cfg);
    h.begin();
    for (int i = 0; i < 100; i++) { h.queueButton(); h.step(50); }

    // 100 iterations x 50 ms = 5 s. At one press per 200 ms that is 25-ish.
    CHECK_LE(counter.presses, 30, "the harness is not debouncing like the hardware");
    CHECK(counter.presses >= 15, "the harness is swallowing presses it should deliver");
}
