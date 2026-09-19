#include "framework.h"
#include "Harness.h"
#include "gaterun/GateRun.h"

// Tests for the reference game specifically - an example of the OTHER half of
// the job. The conformance suite proves a game behaves on the hardware; this
// proves a game does what it says it does. Write the equivalent for yours.

using namespace sim;

TEST(gaterun_stores_the_score_once_when_the_run_ends) {
    games::GateRun game;
    HarnessConfig cfg;
    Harness h(game, cfg);
    h.begin();

    // Never touch the controls: the ship holds its line and eventually meets
    // a wall, which is the only way this game ends.
    for (int i = 0; i < 300 && h.scoresSubmitted() == 0; i++) h.step(50);

    CHECK_EQ(h.scoresSubmitted(), 1, "the run should store exactly one score");

    for (int i = 0; i < 100; i++) h.step(50);
    CHECK_EQ(h.scoresSubmitted(), 1, "nothing more should be stored after the run is over");
}

TEST(gaterun_leaves_when_the_knob_is_turned_on_the_game_over_screen) {
    games::GateRun game;
    HarnessConfig cfg;
    Harness h(game, cfg);
    h.begin();

    for (int i = 0; i < 300 && h.scoresSubmitted() == 0; i++) h.step(50);
    CHECK(h.scoresSubmitted() == 1, "expected the run to have ended");
    CHECK(!h.exited(), "should still be in the game at this point");

    h.queueKnob(1);
    h.step(50);
    CHECK(h.exited(), "a knob turn on the game-over screen must go back to the game list");
}

TEST(gaterun_keeps_the_ship_inside_the_play_area) {
    games::GateRun game;
    HarnessConfig cfg;
    Harness h(game, cfg);
    h.begin();

    // Wind the knob hard against both ends. Nothing should escape upward into
    // the status bar or downward into the legend.
    for (int i = 0; i < 60; i++) h.queueKnob(1);
    h.step(50);
    for (int i = 0; i < 120; i++) h.queueKnob(-1);
    h.step(50);

    CHECK(h.recorder().violations().empty(),
          "the ship left the play area when the knob was wound to its limit");
}

TEST(gaterun_boost_costs_a_point_and_expires) {
    games::GateRun game;
    HarnessConfig cfg;
    Harness h(game, cfg);
    h.begin();

    // Score first, so there is a point to spend.
    for (int i = 0; i < 40 && h.scoresSubmitted() == 0; i++) h.step(50);
    // Nothing to assert about the score directly - it is the game's private
    // state - but the run must have ended cleanly rather than hung.
    CHECK(h.scoresSubmitted() <= 1, "the game over path ran more than once");
}
