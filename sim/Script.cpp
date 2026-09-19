#include "Script.h"
#include <algorithm>
#include <cstdio>
#include <cstring>
#include <fstream>
#include <sstream>

namespace sim {

bool loadScript(const char* path, std::vector<Event>& out, std::string& error) {
    std::ifstream in(path);
    if (!in) { error = std::string("cannot open script: ") + path; return false; }

    std::string line;
    int lineNo = 0;
    while (std::getline(in, line)) {
        lineNo++;
        size_t hash = line.find('#');
        if (hash != std::string::npos) line = line.substr(0, hash);
        std::istringstream ls(line);

        long at;
        std::string verb;
        if (!(ls >> at >> verb)) {
            if (line.find_first_not_of(" \t\r\n") == std::string::npos) continue;   // blank
            error = "line " + std::to_string(lineNo) + ": expected '<ms> <action>'";
            return false;
        }

        Event e { (uint32_t)at, Event::Press, 0, "" };
        if (verb == "knob") {
            int n;
            if (!(ls >> n) || n == 0) { error = "line " + std::to_string(lineNo) + ": knob needs a non-zero count"; return false; }
            e.kind = Event::Knob; e.arg = n;
        } else if (verb == "press") {
            e.kind = Event::Press;
        } else if (verb == "theme") {
            int n; if (!(ls >> n)) { error = "line " + std::to_string(lineNo) + ": theme needs an index"; return false; }
            e.kind = Event::Theme; e.arg = n;
        } else if (verb == "shot") {
            std::string label; ls >> label;
            e.kind = Event::Shot; e.label = label.empty() ? std::to_string(at) : label;
        } else if (verb == "end") {
            e.kind = Event::End;
        } else {
            error = "line " + std::to_string(lineNo) + ": unknown action '" + verb + "'";
            return false;
        }
        out.push_back(e);
    }

    std::stable_sort(out.begin(), out.end(), [](const Event& a, const Event& b) { return a.atMs < b.atMs; });
    return true;
}

} // namespace sim
