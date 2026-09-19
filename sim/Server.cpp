#include "Server.h"
#include "Budget.h"
#include "page_html.h"

#include <arpa/inet.h>
#include <cerrno>
#include <cstdio>
#include <cstring>
#include <netinet/in.h>
#include <poll.h>
#include <string>
#include <sys/socket.h>
#include <sys/time.h>
#include <unistd.h>
#include <vector>

namespace sim {

static uint64_t nowMs() {
    struct timeval tv;
    gettimeofday(&tv, nullptr);
    return (uint64_t)tv.tv_sec * 1000ULL + (uint64_t)tv.tv_usec / 1000ULL;
}

static void sendAll(int fd, const char* data, size_t len) {
    size_t sent = 0;
    while (sent < len) {
        ssize_t n = ::send(fd, data + sent, len - sent, 0);
        if (n <= 0) return;
        sent += (size_t)n;
    }
}

static void respond(int fd, const char* status, const char* type,
                    const char* body, size_t len) {
    char head[256];
    int hl = snprintf(head, sizeof(head),
                      "HTTP/1.1 %s\r\nContent-Type: %s\r\nContent-Length: %zu\r\n"
                      "Cache-Control: no-store\r\nConnection: close\r\n\r\n",
                      status, type, len);
    sendAll(fd, head, (size_t)hl);
    if (len) sendAll(fd, body, len);
}

static std::string jsonEscape(const std::string& in) {
    std::string out;
    for (char c : in) {
        if (c == '"' || c == '\\') { out += '\\'; out += c; }
        else if (c == '\n') out += "\\n";
        else out += c;
    }
    return out;
}

int serve(Harness& h, int port, uint32_t loopMs, uint32_t jitterMs) {
    int listenFd = ::socket(AF_INET, SOCK_STREAM, 0);
    if (listenFd < 0) { perror("socket"); return 1; }
    int one = 1;
    ::setsockopt(listenFd, SOL_SOCKET, SO_REUSEADDR, &one, sizeof(one));

    sockaddr_in addr {};
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = htonl(INADDR_LOOPBACK);   // local only, never exposed
    addr.sin_port = htons((uint16_t)port);
    if (::bind(listenFd, (sockaddr*)&addr, sizeof(addr)) < 0) { perror("bind"); ::close(listenFd); return 1; }
    if (::listen(listenFd, 8) < 0) { perror("listen"); ::close(listenFd); return 1; }

    printf("simulator running: http://127.0.0.1:%d/   (ctrl-c to stop)\n", port);
    fflush(stdout);

    h.begin();

    std::vector<uint8_t> rgb;
    uint64_t start = nowMs();
    uint64_t nextStep = start;
    uint32_t stepJitter = 0;

    for (;;) {
        uint64_t t = nowMs();
        if (t >= nextStep) {
            // One iteration of the device main loop, at the device's pace,
            // with the same irregularity background work causes there.
            uint32_t advance = loopMs + stepJitter;
            h.step(advance);
            stepJitter = jitterMs ? (uint32_t)h.random(jitterMs + 1) : 0;
            nextStep = t + loopMs + stepJitter;
        }

        int wait = (int)(nextStep > nowMs() ? nextStep - nowMs() : 0);
        if (wait > 15) wait = 15;
        struct pollfd pfd { listenFd, POLLIN, 0 };
        if (::poll(&pfd, 1, wait) <= 0) continue;

        int fd = ::accept(listenFd, nullptr, nullptr);
        if (fd < 0) continue;

        char req[2048];
        ssize_t n = ::recv(fd, req, sizeof(req) - 1, 0);
        if (n <= 0) { ::close(fd); continue; }
        req[n] = '\0';

        char method[8] = {0}, path[512] = {0};
        sscanf(req, "%7s %511s", method, path);

        if (strncmp(path, "/frame.raw", 10) == 0) {
            h.screen().toRgb888(rgb);
            respond(fd, "200 OK", "application/octet-stream", (const char*)rgb.data(), rgb.size());
        } else if (strncmp(path, "/input", 6) == 0) {
            const char* k = strstr(path, "k=");
            if (k) {
                k += 2;
                if (strncmp(k, "up", 2) == 0) h.queueKnob(+1);
                else if (strncmp(k, "down", 4) == 0) h.queueKnob(-1);
                else if (strncmp(k, "press", 5) == 0) h.queueButton();
                else if (strncmp(k, "restart", 7) == 0) h.restart();
                else if (strncmp(k, "theme", 5) == 0) h.setThemeIndex((h.themeIndex() + 1) % gk::themes::count());
                else if (strncmp(k, "mute", 4) == 0) h.setSoundOn(!h.soundEnabled());
            }
            respond(fd, "200 OK", "text/plain", "ok", 2);
        } else if (strncmp(path, "/state", 6) == 0) {
            std::string viol;
            const auto& vs = h.recorder().violations();
            for (size_t i = 0; i < vs.size() && i < 20; i++) {
                if (!viol.empty()) viol += "\",\"";
                viol += jsonEscape(vs[i].op + ": " + vs[i].detail);
            }
            // Notes that started since the last poll, handed to the page to
            // play. Drained here: each one is played exactly once.
            std::string notes;
            for (int ni = 0; ni < h.buzzer().startedCount(); ni++) {
                char noteBuf[48];
                snprintf(noteBuf, sizeof(noteBuf), "%s{\"f\":%u,\"ms\":%u}",
                         notes.empty() ? "" : ",",
                         (unsigned)h.buzzer().started(ni).freqHz,
                         (unsigned)h.buzzer().started(ni).ms);
                notes += noteBuf;
            }
            h.buzzer().drainStarted();

            char body[8192];
            int bl = snprintf(body, sizeof(body),
                "{\"framePixels\":%ld,\"peakPixels\":%ld,\"frames\":%u,\"now\":%u,"
                "\"best\":%d,\"theme\":\"%s\",\"warnAt\":%ld,\"failAt\":%ld,"
                "\"exited\":%s,\"notesPlayed\":%ld,\"notesDropped\":%ld,"
                "\"soundOn\":%s,\"notes\":[%s],\"violations\":[%s%s%s]}",
                h.recorder().framePixels(), h.recorder().peakFramePixels(),
                h.frames(), h.now(), h.bestScore(), h.theme().name,
                FRAME_PIXELS_WARN, FRAME_PIXELS_FAIL,
                h.exited() ? "true" : "false",
                h.buzzer().played(), h.buzzer().dropped(),
                h.soundEnabled() ? "true" : "false", notes.c_str(),
                viol.empty() ? "" : "\"", viol.c_str(), viol.empty() ? "" : "\"");
            respond(fd, "200 OK", "application/json", body, (size_t)bl);
        } else if (strcmp(path, "/") == 0 || strncmp(path, "/index", 6) == 0) {
            respond(fd, "200 OK", "text/html; charset=utf-8", kPageHtml, strlen(kPageHtml));
        } else {
            respond(fd, "404 Not Found", "text/plain", "no", 2);
        }
        ::close(fd);

        if (h.exited()) {
            // The game asked to go back to the device's game list. There is no
            // list here, so treat it as the end of the run and start over,
            // which is what re-entering the game on the device does.
            h.restart();
        }
    }
}

} // namespace sim
