# API reference

Four headers, and nothing else is available to a game.

| | |
| --- | --- |
| [`gk::Game`](#gkgame) | the class you implement |
| [`gk::Host`](#gkhost) | the device, as far as you are concerned |
| [`gk::Gfx`](#gkgfx) | drawing |
| [`gk::Theme`](#gktheme) | colours |

---

## `gk::Game`

```cpp
class Game {
    virtual const char* name() const = 0;
    virtual void start(Host&) = 0;
    virtual void repaint(Host&, Gfx&) = 0;
    virtual void update(Host&, Gfx&) = 0;
    virtual void onKnob(Host&, int dir) = 0;
    virtual void onButton(Host&) = 0;
};
```

**`name()`** - shown in the device's game list. Sixteen characters or fewer.

**`start(host)`** - reset to a fresh run. Called once per run. Read
`host.play()` here and keep it. **Do not draw** - `repaint()` is called
immediately afterwards and puts the first frame up. Your object is not
reconstructed between runs, so anything that must be fresh has to be reset
here explicitly.

**`repaint(host, gfx)`** - paint the current state, in full, assuming nothing
on screen is yours. Called after `start()`, on a theme change, and whenever
something painted over you. Must not change state, and must leave the screen
exactly as `update()` would have. See rule 6.

**`update(host, gfx)`** - advance the game and paint what changed. Called
every iteration of the device's main loop, which is every 50-90 ms and
irregular. Gate your own frame on `host.millis()` and return immediately when
it is not due. Must never block.

**`onKnob(host, dir)`** - one detent. `dir` is `+1` clockwise or `-1`. Can be
called several times in one pass if the knob was spun. No drawing.

**`onButton(host)`** - the button went down, at most once per 200 ms. No
drawing.

---

## `gk::Host`

```cpp
Rect        play() const;
const Theme& theme() const;
uint32_t    millis() const;
uint32_t    random(uint32_t n);
int         bestScore() const;
void        submitScore(int score);
void        setLegend(const char* knob, const char* button);
void        playTone(uint16_t freqHz, uint16_t ms);
void        stopSound();
bool        soundEnabled() const;
void        exitGame();
```

**`play()`** - the rectangle you may draw in, in panel coordinates.
Currently `{0, 20, 320, 190}`, but read it, never assume it.

```cpp
struct Rect {
    int x, y, w, h;
    int right() const;    // x + w
    int bottom() const;   // y + h
    int cx() const;       // centre x
    int cy() const;       // centre y
    bool contains(int px, int py) const;
};
```

**`theme()`** - the current palette. See below.

**`millis()`** - milliseconds since the device booted. Your only clock.
Jumps irregularly between calls; treat every interval as unknown.

**`random(n)`** - a random integer in `[0, n)`. Seeded and reproducible.
`random(0)` returns 0.

**`bestScore()`** / **`submitScore(n)`** - the player's stored best for this
game. Submit once, when a run ends; it writes to flash. `bestScore()` reflects
the write immediately afterwards.

**`setLegend(knob, button)`** - what the bottom strip says the controls do
right now. Pass `""` for the knob if it genuinely does nothing in this state.
Keep each under ~22 characters. The strings are copied, so a local buffer is
fine.

**`playTone(freqHz, ms)`** - queue one note on the buzzer and return
immediately. A square wave, one voice, notes strictly one after another.
`freqHz` of `0` is a rest. Both arguments are clamped to the limits in
`gk::sound`:

| | |
| --- | --- |
| `gk::sound::MIN_HZ` / `MAX_HZ` | 100 / 5000 |
| `gk::sound::SWEET_SPOT_HZ` | 2700 - where a buzzer this size is loudest |
| `gk::sound::MAX_MS` | 2000 per note |
| `gk::sound::QUEUE_DEPTH` | 8 notes held; the rest are dropped |

Fire these on events, never per frame. See rule 13 in
[RULES.md](RULES.md).

**`stopSound()`** - cut the sounding note and everything queued behind it.

**`soundEnabled()`** - whether the player has sound on. Notes are dropped
silently when they do not, so you rarely need to ask; your game must be
completely playable muted either way.

**`exitGame()`** - leave the game and go back to the device's game list. The
host repaints everything afterwards; do not draw once you have called it.

---

## `gk::Gfx`

Coordinates are panel coordinates - `(0,0)` is the top-left of the whole
screen, not of your play area. Drawing off the edge of the panel is clipped
and costs nothing, so spawning objects off-screen and walking them in is
normal and fine. Drawing *on* the panel but outside your play area is a rule
break.

### Shapes

```cpp
void fillRect(int x, int y, int w, int h, color_t c);
void drawRect(int x, int y, int w, int h, color_t c);
void drawPixel(int x, int y, color_t c);
void drawFastHLine(int x, int y, int w, color_t c);
void drawFastVLine(int x, int y, int h, color_t c);
void drawLine(int x0, int y0, int x1, int y1, color_t c);
void fillCircle(int cx, int cy, int r, color_t c);
void drawCircle(int cx, int cy, int r, color_t c);
void fillTriangle(int x0,int y0, int x1,int y1, int x2,int y2, color_t c);
```

`fillRect` is the one that matters: it is how you paint, and passing
`theme.bg` is how you erase. Cost is `w * h` pixels - see the budget in
[HARDWARE.md](HARDWARE.md).

### Sprites

```cpp
void drawBitmap(int x, int y, const uint8_t* bmp, int w, int h, color_t c);
```

One bit per pixel, row-major, MSB first, each row padded to a whole byte.
Only the set bits are painted, which means drawing the same bitmap in
`theme.bg` erases exactly the silhouette and nothing around it - the cheapest
way to move a sprite. `(x, y)` is the top-left of the box.

```cpp
// 12x12 - two bytes per row, the low four bits of the second byte unused
static const uint8_t kShip[] = {
    0x0C, 0x00,   // ....##......
    0x0E, 0x00,   // ....###.....
    // ...
};
```

Keeping the picture in a comment beside the bytes is worth the space; nobody
can read hex.

### Text

```cpp
void setTextSize(int size);          // 1 = 6x8 px per char, 2 = 12x16, ...
void setTextColor(color_t c);
void setTextDatum(Datum d);          // where (x,y) sits on the string
void setCursor(int x, int y);
void print(const char* s);           // draws at the cursor, advances it
void print(int v);
void drawString(const char* s, int x, int y);   // draws at (x,y) by datum
int  textWidth(const char* s) const;
int  fontHeight() const;
```

`Datum` is `gk::Datum::TL`, `TC`, `TR`, `ML`, `MC`, `MR`, `BL`, `BC`, `BR` -
vertical letter first, so `MC` centres the string on the point. It is sticky:
it stays set until you change it, so set it back to `TL` when you are done
if other code expects that.

Use `textWidth()` to size the rectangle you erase before redrawing changing
text. For numbers, note that the *width changes* when the digit count does -
erase the old width, not the new one, or you leave a digit behind.

There is no `fillScreen()`. That is deliberate.

---

## `gk::Theme`

```cpp
struct Theme {
    const char* name;
    color_t bg;          // play-area background - what "erase" means
    color_t fg;          // primary text and lines
    color_t fgDim;       // secondary: hints, labels, supporting text
    color_t rule;        // hairlines, borders, grid
    color_t selectBg;    // filled background of a selected item
    color_t selectFg;    // text drawn on selectBg
    color_t selectMark;  // marker bar on a selected item
    color_t accent;      // "now / watch out" - hazards, the thing to react to
    color_t accent2;     // "good" - a hit, a point, a personal best
    color_t danger;      // "lost" - a miss, a life gone, game over
    int smallLineH;      // line height in px for size-1 text
};
```

Pick by meaning. The contrast ratios were measured on the real panel, which
is why a theme's colours are not the ones you would choose on a monitor.

`gk::themes::Light` and `gk::themes::Dark` are what ship; the user switches
between them at will, including mid-run.

### Making your own colour

```cpp
#include "gamekit/Colors.h"
gfx.fillRect(x, y, w, h, PANEL_HEX(0xff8800));   // shows orange on the glass
```

Never a raw 16-bit literal - see the panel note in
[HARDWARE.md](HARDWARE.md).
