#include "registry.h"
#include "gaterun/GateRun.h"
#include "template/NewGame.h"
#include <cstring>

namespace games {

// Statically allocated, one instance each, alive for the life of the program -
// the same way the device holds them. Nothing here is ever new'd.
static GateRun sGateRun;
static NewGame sNewGame;

static const Entry kEntries[] = {
    { "gaterun",  &sGateRun },
    { "newgame",  &sNewGame },
};

const Entry* all() { return kEntries; }
int count() { return (int)(sizeof(kEntries) / sizeof(kEntries[0])); }

gk::Game* byKey(const char* key) {
    if (!key) return nullptr;
    for (int i = 0; i < count(); i++)
        if (strcmp(kEntries[i].key, key) == 0) return kEntries[i].game;
    return nullptr;
}

} // namespace games
