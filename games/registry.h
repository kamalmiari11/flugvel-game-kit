#pragma once
#include "gamekit/Game.h"

// Every game the simulator and the device harness can run. Add yours here
// and it shows up in both: ./build/sim --list
namespace games {

struct Entry {
    const char* key;      // what you pass to --game
    gk::Game*   game;
};

const Entry* all();
int count();
gk::Game* byKey(const char* key);

} // namespace games
