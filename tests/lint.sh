#!/bin/sh
# Checks the game sources for things that compile fine on a desktop and then
# misbehave, stall or refuse to build on the device. Every pattern here is a
# rule from docs/RULES.md, made enforceable.
#
# Run it with: make lint

set -u
DIR="$(CDPATH= cd -- "$(dirname -- "$0")/.." && pwd)"
SRC="$DIR/games"
fails=0
warns=0

# Every line of every game source as "path:line:code", with // comments
# stripped - so prose about what the code does can talk about anything, and
# only real code is judged.
CODE=$(grep -rn '' "$SRC" --include='*.cpp' --include='*.h' 2>/dev/null | sed 's://.*::')

scan() {   # scan <severity> <pattern> <explanation>
    sev="$1"; pat="$2"; why="$3"
    hits=$(printf '%s\n' "$CODE" | grep -E "$pat" || true)
    [ -z "$hits" ] && return 0
    if [ "$sev" = "error" ]; then
        printf '\n  BANNED  %s\n' "$why"
        fails=$((fails + 1))
    else
        printf '\n  CHECK   %s\n' "$why"
        warns=$((warns + 1))
    fi
    printf '%s\n' "$hits" | sed "s|$DIR/||" | sed 's/^/          /'
}

printf 'linting %s\n' "${SRC#"$DIR"/}"

scan error '\bdelay[[:space:]]*\(|\busleep[[:space:]]*\(|\bsleep[[:space:]]*\(' \
  'blocking the loop. The whole device stops - input, screen, background work. Gate on host.millis() and return instead.'

scan error '\bwhile[[:space:]]*\([[:space:]]*(true|1)[[:space:]]*\)' \
  'an unbounded loop inside a call the host makes every iteration.'

scan error '\bnew[[:space:]]+[A-Za-z_]|\bmalloc[[:space:]]*\(|\bcalloc[[:space:]]*\(|\brealloc[[:space:]]*\(' \
  'allocating memory. The heap is shared with the network stack and fragments. Fixed-size members only.'

scan error '\bstd::(vector|string|map|set|list|deque|unique_ptr|shared_ptr)\b|\bString\b' \
  'a container or Arduino String. These allocate. Use fixed-size arrays and char buffers.'

scan error '\bSerial[[:space:]]*\.|\bWiFi\b|\bHTTPClient\b|\bEEPROM\b|\bPreferences\b|\bSPIFFS\b|\bLittleFS\b' \
  'reaching for device hardware or storage directly. Everything you may touch is on gk::Host.'

scan error '\bTFT_eSPI\b|#include[[:space:]]*<Arduino\.h>' \
  'talking to the display driver directly. Draw through gk::Gfx so the game builds on the desktop too.'

scan error '(^|[^._>[:alnum:]])(tone|noTone|ledcSetup|ledcAttach|ledcAttachPin|ledcWrite|ledcWriteTone|analogWrite|digitalWrite|pinMode)[[:space:]]*\(' \
  'driving a pin directly. The buzzer belongs to the host - use host.playTone(freq, ms), which does not block.'

scan error '\bfillScreen[[:space:]]*\(' \
  'clearing the whole panel. The status bar and the legend are not yours. Fill Host::play() instead.'

scan error '(^|[^._>[:alnum:]])millis[[:space:]]*\(|(^|[^._>[:alnum:]])micros[[:space:]]*\(' \
  'reading the clock directly. Use host.millis() - it is what makes a run reproducible.'

scan error '(^|[^._>[:alnum:]])(rand|random)[[:space:]]*\(' \
  'using the global RNG. Use host.random(n), or a recorded run will not replay.'

scan error '\bassert[[:space:]]*\(|\babort[[:space:]]*\(|\bexit[[:space:]]*\(' \
  'aborting. On the device this is a reboot, not a stack trace.'

scan error '\bfloat[[:space:]]+[A-Za-z_]+[[:space:]]*=[[:space:]]*[A-Za-z_]*sin|\bdouble\b' \
  'double-precision maths. The chip has no hardware double; use float.'

scan warn '0x[0-9A-Fa-f]{4}([^0-9A-Fa-f]|$)' \
  'a raw 16-bit constant. If it is a colour it is almost certainly wrong on this panel - take colours from host.theme(), or wrap them in PANEL_HEX(). Ignore this if it is a bitmask.'

printf '\n'
if [ "$fails" -gt 0 ]; then
    printf '%d banned pattern(s), %d to check. See docs/RULES.md.\n' "$fails" "$warns"
    exit 1
fi
printf 'lint clean'
[ "$warns" -gt 0 ] && printf ' (%d thing(s) to check by eye)' "$warns"
printf '\n'
exit 0
