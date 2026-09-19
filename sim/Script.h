#pragma once
#include <stdint.h>
#include <string>
#include <vector>

namespace sim {

// A recorded run: what the player did, and when, on the device's clock.
// Replaying one is deterministic - same seed, same script, same pixels - so
// "it crashes after about a minute" becomes a file anyone can re-run.
//
//   # lines starting with # are comments
//   400   knob +1      one detent clockwise (a number > 1 means several)
//   450   knob -2
//   900   press        the button
//   1200  theme 1      switch palette mid-run
//   2000  shot over    save a frame named "over"
//   5000  end          stop here
struct Event {
    uint32_t atMs;
    enum Kind { Knob, Press, Theme, Shot, End } kind;
    int arg;
    std::string label;
};

// Returns false and fills `error` if the file cannot be read or a line is
// malformed. A script with a bad line is never silently half-run.
bool loadScript(const char* path, std::vector<Event>& out, std::string& error);

} // namespace sim
