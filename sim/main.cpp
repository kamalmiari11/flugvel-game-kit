#include "Harness.h"
#include "Server.h"
#include "Script.h"
#include "Bmp.h"
#include "Budget.h"
#include "registry.h"

#include <algorithm>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <fstream>
#include <string>
#include <vector>

using namespace sim;

static void usage() {
    printf(
"simulator - runs a game the way the device runs it\n"
"\n"
"  sim [--game KEY] [options]\n"
"\n"
"  --list                 show the games this build knows about\n"
"  --game KEY             which one to run (default: the first)\n"
"\n"
"interactive (the default):\n"
"  --port N               serve the panel at http://127.0.0.1:N/  (default 8080)\n"
"\n"
"replay, for tests and bug reports:\n"
"  --script FILE          play a recorded input script (see sim/Script.h)\n"
"  --duration MS          how long to run, on the device clock (default 10000)\n"
"  --record FILE.html     write a filmstrip of the run you can open in a browser\n"
"  --every N              with --record, keep every Nth frame (default 4)\n"
"  --shot FILE.bmp        write the final frame\n"
"  --strict               exit non-zero on any rule break or blown frame budget\n"
"\n"
"shared:\n"
"  --seed N               random seed (default 12345) - same seed, same run\n"
"  --theme N              0 light, 1 dark\n"
"  --best N               pretend the stored best score is N\n"
"  --mute                 run with the buzzer off, as a player may well do\n"
"  --loop-ms N            device loop period (default 50 - do not raise it to\n"
"                         make your game feel better, the device will not)\n"
"  --jitter N             extra irregular delay per iteration (default 40)\n");
}

int main(int argc, char** argv) {
    const char* gameKey = nullptr;
    const char* script = nullptr;
    const char* record = nullptr;
    const char* shot = nullptr;
    int port = 8080, every = 4;
    uint32_t duration = 10000;
    bool strict = false, interactive = true;
    HarnessConfig cfg;

    for (int i = 1; i < argc; i++) {
        auto next = [&](const char* what) -> const char* {
            if (i + 1 >= argc) { fprintf(stderr, "%s needs a value\n", what); exit(2); }
            return argv[++i];
        };
        if (!strcmp(argv[i], "--help") || !strcmp(argv[i], "-h")) { usage(); return 0; }
        else if (!strcmp(argv[i], "--list")) {
            for (int g = 0; g < games::count(); g++)
                printf("%-12s %s\n", games::all()[g].key, games::all()[g].game->name());
            return 0;
        }
        else if (!strcmp(argv[i], "--game")) gameKey = next("--game");
        else if (!strcmp(argv[i], "--port")) port = atoi(next("--port"));
        else if (!strcmp(argv[i], "--script")) { script = next("--script"); interactive = false; }
        else if (!strcmp(argv[i], "--duration")) { duration = (uint32_t)atoi(next("--duration")); interactive = false; }
        else if (!strcmp(argv[i], "--record")) { record = next("--record"); interactive = false; }
        else if (!strcmp(argv[i], "--every")) every = atoi(next("--every"));
        else if (!strcmp(argv[i], "--shot")) { shot = next("--shot"); interactive = false; }
        else if (!strcmp(argv[i], "--strict")) strict = true;
        else if (!strcmp(argv[i], "--seed")) cfg.seed = (uint32_t)atoi(next("--seed"));
        else if (!strcmp(argv[i], "--theme")) cfg.themeIndex = atoi(next("--theme"));
        else if (!strcmp(argv[i], "--best")) cfg.startingBest = atoi(next("--best"));
        else if (!strcmp(argv[i], "--mute")) cfg.soundOn = false;
        else if (!strcmp(argv[i], "--loop-ms")) cfg.loopMs = (uint32_t)atoi(next("--loop-ms"));
        else if (!strcmp(argv[i], "--jitter")) cfg.jitterMs = (uint32_t)atoi(next("--jitter"));
        else { fprintf(stderr, "unknown option: %s\n\n", argv[i]); usage(); return 2; }
    }

    gk::Game* game = gameKey ? games::byKey(gameKey) : (games::count() ? games::all()[0].game : nullptr);
    if (!game) {
        fprintf(stderr, "no such game: %s (try --list)\n", gameKey ? gameKey : "(none)");
        return 2;
    }
    if (every < 1) every = 1;

    Harness h(*game, cfg);

    if (interactive) return serve(h, port, cfg.loopMs, cfg.jitterMs);

    // ---- replay ----
    std::vector<Event> events;
    if (script) {
        std::string err;
        if (!loadScript(script, events, err)) { fprintf(stderr, "%s\n", err.c_str()); return 2; }
    }

    struct Shot { std::string label; std::vector<uint8_t> bmp; };
    std::vector<Shot> shots;
    std::vector<uint8_t> rgb;
    auto capture = [&](const std::string& label) {
        h.screen().toRgb888(rgb);
        shots.push_back({ label, encodeBmp(rgb, SoftGfx::W, SoftGfx::H) });
    };

    h.begin();
    if (record) capture("start");

    size_t next = 0;
    long overBudget = 0;
    uint32_t frame = 0;
    bool ended = false;
    std::vector<long> frameCost;

    while (h.now() < duration && !ended) {
        uint32_t advance = cfg.loopMs + (cfg.jitterMs ? h.random(cfg.jitterMs + 1) : 0);
        uint32_t target = h.now() + advance;

        while (next < events.size() && events[next].atMs <= target) {
            const Event& e = events[next++];
            switch (e.kind) {
                case Event::Knob:
                    for (int k = 0; k < (e.arg < 0 ? -e.arg : e.arg); k++) h.queueKnob(e.arg > 0 ? 1 : -1);
                    break;
                case Event::Press: h.queueButton(); break;
                case Event::Theme: h.setThemeIndex(e.arg); break;
                case Event::Shot:  capture(e.label); break;
                case Event::End:   ended = true; break;
            }
        }
        if (ended) break;

        h.step(advance);
        frameCost.push_back(h.recorder().framePixels());
        if (h.recorder().framePixels() > FRAME_PIXELS_FAIL) overBudget++;
        if (record && (frame % (uint32_t)every) == 0) capture(std::to_string(h.now()) + " ms");
        frame++;

        if (h.exited()) break;
    }

    if (record) capture("end");

    // ---- report ----
    // A full repaint of the play area is 64,800 px - about 26 ms - and is
    // perfectly fine now and then: entering the game, restarting, a theme
    // change. What must not happen is a heavy frame during ordinary play, so
    // the numbers that matter are the typical one and the 95th percentile,
    // not the single worst.
    std::sort(frameCost.begin(), frameCost.end());
    auto pct = [&](double p) -> long {
        if (frameCost.empty()) return 0;
        size_t i = (size_t)(p * (double)(frameCost.size() - 1));
        return frameCost[i];
    };
    // Full repaints are legitimate but rare: entering the game, restarting,
    // a theme change. One frame in twenty leaves room for a short, hard game
    // being restarted repeatedly and still catches anything that clears the
    // play area as a matter of course.
    const long heavyAllowance = (long)frameCost.size() / 20 + 2;

    const auto& viol = h.recorder().violations();
    printf("game            %s\n", game->name());
    printf("device clock    %u ms over %u iterations\n", h.now(), h.frames());
    printf("typical frame   %ld px  (%.1f ms of bus time)\n", pct(0.5), (double)pct(0.5) / PIXELS_PER_MS);
    printf("95th pct frame  %ld px  (%.1f ms)\n", pct(0.95), (double)pct(0.95) / PIXELS_PER_MS);
    printf("worst frame     %ld px  (%.1f ms)\n",
           h.recorder().peakFramePixels(), (double)h.recorder().peakFramePixels() / PIXELS_PER_MS);
    printf("heavy frames    %ld over %ld px (allowed %ld - full repaints)\n",
           overBudget, FRAME_PIXELS_FAIL, heavyAllowance);
    printf("scores stored   %d (last %d, best %d)\n", h.scoresSubmitted(), h.lastScore(), h.bestScore());
    printf("buzzer          %ld note(s) played, %ld dropped%s\n",
           h.buzzer().played(), h.buzzer().dropped(),
           h.soundEnabled() ? "" : "  (muted)");
    printf("left the game   %s\n", h.exited() ? "yes" : "no");
    printf("rule breaks     %zu\n", viol.size());
    for (size_t i = 0; i < viol.size() && i < 12; i++)
        printf("  - %s: %s\n", viol[i].op.c_str(), viol[i].detail.c_str());
    if (viol.size() > 12) printf("  ... and %zu more\n", viol.size() - 12);

    if (shot) {
        h.screen().toRgb888(rgb);
        auto bmp = encodeBmp(rgb, SoftGfx::W, SoftGfx::H);
        std::ofstream f(shot, std::ios::binary);
        f.write((const char*)bmp.data(), (std::streamsize)bmp.size());
        printf("wrote           %s\n", shot);
    }

    if (record) {
        std::ofstream f(record);
        f << "<!doctype html><meta charset=utf-8><title>run</title>"
             "<style>body{background:#14161a;color:#e7eaf0;font:13px ui-monospace,monospace;margin:0;padding:20px}"
             "#s{display:flex;gap:10px;align-items:center;margin-bottom:14px}"
             "img{image-rendering:pixelated;width:320px;height:240px;background:#000;border-radius:8px}"
             "figure{margin:0}figcaption{color:#98a1b0;padding-top:6px;text-align:center}"
             "#strip{display:flex;flex-wrap:wrap;gap:14px}</style>"
             "<div id=s><button id=p>play</button><input id=r type=range min=0 value=0><span id=l></span></div>"
             "<figure><img id=big><figcaption id=cap></figcaption></figure><hr style='border-color:#2c313a;margin:18px 0'>"
             "<div id=strip></div><script>const F=[";
        for (size_t i = 0; i < shots.size(); i++) {
            f << (i ? "," : "") << "{l:\"" << shots[i].label << "\",d:\"data:image/bmp;base64,"
              << base64(shots[i].bmp) << "\"}";
        }
        f << "];const big=document.getElementById('big'),cap=document.getElementById('cap'),"
             "r=document.getElementById('r'),l=document.getElementById('l'),p=document.getElementById('p');"
             "r.max=F.length-1;function show(i){big.src=F[i].d;cap.textContent=F[i].l;l.textContent=(+i+1)+'/'+F.length;r.value=i}"
             "r.oninput=e=>show(+e.target.value);let t=null;p.onclick=()=>{if(t){clearInterval(t);t=null;p.textContent='play';return}"
             "p.textContent='pause';t=setInterval(()=>{let i=(+r.value+1)%F.length;show(i)},120)};show(0);"
             "F.forEach((f,i)=>{const d=document.createElement('figure');d.innerHTML='<img src=\"'+f.d+'\" style=\"width:160px;height:120px\">"
             "<figcaption>'+f.l+'</figcaption>';d.onclick=()=>show(i);document.getElementById('strip').appendChild(d)});</script>";
        printf("wrote           %s (%zu frames)\n", record, shots.size());
    }

    if (strict && (!viol.empty() || overBudget > heavyAllowance || h.buzzer().dropped() > 0)) {
        printf("\nFAILED: %zu rule break(s), %ld heavy frame(s) (allowance %ld), "
               "%ld dropped note(s)\n",
               viol.size(), overBudget, heavyAllowance, h.buzzer().dropped());
        return 1;
    }
    return 0;
}
