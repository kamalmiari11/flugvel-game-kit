#include "framework.h"

namespace t {
std::vector<Case>& cases() { static std::vector<Case> c; return c; }
int failures = 0;
const char* current = "";
}

int main() {
    int before = 0;
    printf("running %zu tests\n\n", t::cases().size());
    for (auto& c : t::cases()) {
        t::current = c.name;
        before = t::failures;
        printf("  %s\n", c.name);
        c.fn();
        if (t::failures == before) printf("    ok\n");
    }
    printf("\n%s: %d failure(s)\n", t::failures ? "FAILED" : "PASSED", t::failures);
    return t::failures ? 1 : 0;
}
