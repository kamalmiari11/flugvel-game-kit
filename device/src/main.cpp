#include <Arduino.h>
#include <ESP32Encoder.h>
#include <Preferences.h>
#include <TFT_eSPI.h>

#include "DeviceGfx.h"
#include "gamekit/Game.h"
#include "pins.h"
#include "registry.h"

// The device-side harness: the smallest possible stand-in for the firmware.
// It owns the screen, reads the knob and the button, keeps the best scores,
// and runs one game from games/registry.cpp at a time.
//
// It is NOT the product firmware, and nothing here needs to end up in it -
// the real device already has all of this. Its job is to let you hold the
// thing in your hands and check that what you built in the simulator feels
// the same on the glass.
//
//   knob press        ->  next game in the registry
//   knob turn         ->  the game's onKnob()
//   button            ->  the game's onButton()
//   both together     ->  sound on / off

static const int HEADER_H = 20;
static const int LEGEND_H = 30;

static TFT_eSPI tft;
static DeviceGfx gfx(&tft);
static ESP32Encoder encoder;
static Preferences prefs;

class DeviceHost : public gk::Host {
public:
    gk::Rect play() const override {
        return { 0, HEADER_H, tft.width(), tft.height() - HEADER_H - LEGEND_H };
    }
    const gk::Theme& theme() const override { return gk::themes::byIndex(_theme); }
    uint32_t millis() const override { return ::millis(); }

    uint32_t random(uint32_t n) override {
        _rng ^= _rng << 13; _rng ^= _rng >> 17; _rng ^= _rng << 5;
        return n ? (_rng % n) : 0;
    }

    int bestScore() const override { return _best; }

    void submitScore(int score) override {
        if (score <= _best) return;
        _best = score;
        // One flash write per run, and only when it is actually a new best.
        prefs.putInt(_key, _best);
    }

    void setLegend(const char* knob, const char* button) override {
        copyInto(_knob, sizeof(_knob), knob);
        copyInto(_button, sizeof(_button), button);
    }

    // ---- the buzzer ----
    // Queued, never blocking. The note is started here and stopped by
    // serviceSound() from the main loop when its time is up, so a game that
    // fires a three-note jingle carries on with its frame while the jingle
    // plays.
    void playTone(uint16_t freqHz, uint16_t ms) override {
        if (!_soundOn || ms == 0) return;
        if (ms > gk::sound::MAX_MS) ms = gk::sound::MAX_MS;
        if (freqHz != 0) {
            if (freqHz < gk::sound::MIN_HZ) freqHz = gk::sound::MIN_HZ;
            if (freqHz > gk::sound::MAX_HZ) freqHz = gk::sound::MAX_HZ;
        }
        if (_noteCount >= gk::sound::QUEUE_DEPTH) return;   // dropped, as documented
        _notes[(_noteHead + _noteCount) % gk::sound::QUEUE_DEPTH] = { freqHz, ms };
        _noteCount++;
    }

    void stopSound() override {
        _noteCount = 0;
        _noteHead = 0;
        _noteEndsAt = 0;
        ledcWriteTone(BUZZER_LEDC_CHANNEL, 0);
    }

    bool soundEnabled() const override { return _soundOn; }

    // Called every iteration of loop(). Finishes the note that is due and
    // starts the next one.
    void serviceSound() {
        const uint32_t now = ::millis();
        if (_noteEndsAt != 0) {
            if ((int32_t)(now - _noteEndsAt) < 0) return;   // still sounding
            ledcWriteTone(BUZZER_LEDC_CHANNEL, 0);
            _noteEndsAt = 0;
        }
        if (_noteCount == 0) return;

        Note n = _notes[_noteHead];
        _noteHead = (_noteHead + 1) % gk::sound::QUEUE_DEPTH;
        _noteCount--;
        // Frequency 0 is a rest: the pin stays quiet, the clock still runs.
        ledcWriteTone(BUZZER_LEDC_CHANNEL, n.freqHz);
        _noteEndsAt = now + n.ms;
        if (_noteEndsAt == 0) _noteEndsAt = 1;   // 0 means "nothing sounding"
    }

    void toggleSound() { _soundOn = !_soundOn; if (!_soundOn) stopSound(); }

    void exitGame() override { _exitRequested = true; }

    // ---- harness side ----
    void bindGame(const char* key) {
        snprintf(_key, sizeof(_key), "b_%.10s", key);
        _best = prefs.getInt(_key, 0);
        _knob[0] = _button[0] = '\0';
        _legendDrawn = false;
        _exitRequested = false;
    }
    bool takeExitRequest() { bool e = _exitRequested; _exitRequested = false; return e; }
    void seed(uint32_t s) { _rng = s ? s : 1; }
    void cycleTheme() { _theme = (_theme + 1) % gk::themes::count(); }
    int themeIndex() const { return _theme; }

    const char* legendKnob() const { return _knob; }
    const char* legendButton() const { return _button; }
    bool legendChanged() const { return !_legendDrawn || strcmp(_knob, _drawnKnob) || strcmp(_button, _drawnButton); }
    void legendPainted() {
        copyInto(_drawnKnob, sizeof(_drawnKnob), _knob);
        copyInto(_drawnButton, sizeof(_drawnButton), _button);
        _legendDrawn = true;
    }

private:
    static void copyInto(char* dst, size_t n, const char* src) {
        if (!src) { dst[0] = '\0'; return; }
        strncpy(dst, src, n - 1);
        dst[n - 1] = '\0';
    }

    struct Note { uint16_t freqHz; uint16_t ms; };
    Note _notes[gk::sound::QUEUE_DEPTH] {};
    int _noteHead = 0, _noteCount = 0;
    uint32_t _noteEndsAt = 0;
    bool _soundOn = true;

    int _theme = 0;
    int _best = 0;
    uint32_t _rng = 2463534242u;
    bool _exitRequested = false;
    char _key[16] = "b_none";
    char _knob[24] = {0}, _button[24] = {0};
    char _drawnKnob[24] = {0}, _drawnButton[24] = {0};
    bool _legendDrawn = false;
};

static DeviceHost host;
static int gameIndex = 0;

static void drawChrome(bool full) {
    const gk::Theme& t = host.theme();
    gk::Game* game = games::all()[gameIndex].game;

    if (full) {
        tft.fillRect(0, 0, tft.width(), HEADER_H, t.selectBg);
        tft.setTextSize(1);
        tft.setTextColor(t.selectFg);
        tft.setTextDatum(ML_DATUM);
        tft.drawString(game->name(), 6, HEADER_H / 2);
        tft.setTextDatum(MR_DATUM);
        tft.drawString(String(t.name) + (host.soundEnabled() ? "" : "  MUTE"),
                       tft.width() - 6, HEADER_H / 2);
        tft.setTextDatum(TL_DATUM);
    }

    if (!full && !host.legendChanged()) return;

    const int ly = tft.height() - LEGEND_H;
    tft.fillRect(0, ly, tft.width(), LEGEND_H, t.bg);
    tft.drawFastHLine(0, ly, tft.width(), t.rule);
    tft.setTextSize(1);
    tft.setTextDatum(TL_DATUM);
    tft.setTextColor(t.fgDim);
    tft.drawString(String("KNOB  ") + (host.legendKnob()[0] ? host.legendKnob() : "-"), 6, ly + 5);
    tft.setTextColor(t.fg);
    tft.drawString(String("PRESS ") + (host.legendButton()[0] ? host.legendButton() : "-"), 6, ly + 16);
    host.legendPainted();
}

static void enterGame(int index) {
    gameIndex = ((index % games::count()) + games::count()) % games::count();
    gk::Game* game = games::all()[gameIndex].game;

    host.stopSound();
    host.bindGame(games::all()[gameIndex].key);
    host.seed((uint32_t)micros());

    tft.fillScreen(host.theme().bg);
    game->start(host);
    game->repaint(host, gfx);
    drawChrome(true);

    Serial.printf("[kit] running %s (best %d)\n", game->name(), host.bestScore());
}

void setup() {
    Serial.begin(115200);
    delay(200);

    tft.init();
    // Rotation 1 is the firmware's normal landscape: 320 across, 240 down.
    // A left-handed unit uses 3 instead (the board is physically turned
    // around), which is the same geometry upside down - so a game that reads
    // Host::play() needs no changes for it.
    tft.setRotation(1);
    tft.setTextFont(1);

    pinMode(TFT_BL, OUTPUT);
    digitalWrite(TFT_BL, HIGH);

    pinMode(PIN_GAME_BUTTON, INPUT_PULLUP);
    pinMode(PIN_ENCODER_BTN, INPUT_PULLUP);

    ESP32Encoder::useInternalWeakPullResistors = puType::up;
    encoder.attachFullQuad(PIN_ENCODER_A, PIN_ENCODER_B);
    encoder.clearCount();

    // The buzzer. 2 kHz and 8-bit resolution are just starting values -
    // ledcWriteTone() resets both per note.
    ledcSetup(BUZZER_LEDC_CHANNEL, 2000, 8);
    ledcAttachPin(PIN_BUZZER, BUZZER_LEDC_CHANNEL);
    ledcWriteTone(BUZZER_LEDC_CHANNEL, 0);

    prefs.begin("gamekit", false);

    enterGame(0);
}

void loop() {
    static long lastCount = 0;
    static long accumulated = 0;
    static uint32_t lastButtonAt = 0;
    static int lastButtonState = HIGH;
    static int lastKnobState = HIGH;
    static uint32_t lastKnobAt = 0;

    gk::Game* game = games::all()[gameIndex].game;

    // The buzzer is serviced first and every iteration, so a note's length is
    // governed by the clock rather than by how busy the game is.
    host.serviceSound();

    // ---- knob rotation ----
    long count = encoder.getCount();
    accumulated += count - lastCount;
    lastCount = count;
    while (accumulated >= ENCODER_COUNTS_PER_DETENT) {
        accumulated -= ENCODER_COUNTS_PER_DETENT;
        game->onKnob(host, +1);
    }
    while (accumulated <= -ENCODER_COUNTS_PER_DETENT) {
        accumulated += ENCODER_COUNTS_PER_DETENT;
        game->onKnob(host, -1);
    }

    // ---- the button ----
    const uint32_t now = millis();
    int buttonState = digitalRead(PIN_GAME_BUTTON);
    if (buttonState != lastButtonState && now - lastButtonAt > BUTTON_DEBOUNCE_MS) {
        lastButtonAt = now;
        lastButtonState = buttonState;
        if (buttonState == LOW) game->onButton(host);
    }

    // ---- knob press: next game, or both buttons: mute ----
    int knobState = digitalRead(PIN_ENCODER_BTN);
    if (knobState != lastKnobState && now - lastKnobAt > BUTTON_DEBOUNCE_MS) {
        lastKnobAt = now;
        lastKnobState = knobState;
        if (knobState == LOW) {
            if (buttonState == LOW) {
                host.toggleSound();
                Serial.printf("[kit] sound %s\n", host.soundEnabled() ? "on" : "off");
                drawChrome(true);
            } else {
                enterGame(gameIndex + 1);
            }
            return;
        }
    }

    game->update(host, gfx);
    drawChrome(false);

    if (host.takeExitRequest()) {
        // On the device this returns to the game list. There is no list here,
        // so start the same game again.
        enterGame(gameIndex);
        return;
    }

    // The firmware's main loop ends exactly like this. It is why a game gets
    // called roughly 15-20 times a second and not 60.
    delay(50);
}
