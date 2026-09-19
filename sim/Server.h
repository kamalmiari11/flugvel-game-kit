#pragma once
#include "Harness.h"

namespace sim {

// Runs the game in real time and puts the panel in a browser tab. Returns a
// process exit code.
int serve(Harness& h, int port, uint32_t loopMs, uint32_t jitterMs);

} // namespace sim
