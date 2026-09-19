#pragma once

// How the panel and the controls are wired.
//
// This is the only file in the kit that describes the physical device. If you
// were not sent a unit, you do not need it - the simulator is the full
// development environment and every check runs there.
//
// The display flags live in platformio.ini rather than here, because the
// display driver reads them at compile time.

#define PIN_ENCODER_A    32   // rotary encoder, quadrature A
#define PIN_ENCODER_B    33   // rotary encoder, quadrature B
#define PIN_ENCODER_BTN  25   // pushing the knob in
#define PIN_GAME_BUTTON  26   // the separate button - "the button" in the docs
#define PIN_BUZZER       17   // passive buzzer - a square wave, nothing else

// Which LEDC channel drives the buzzer.
//
// This is not a free choice. On this chip the eight high-speed channels share
// four timers in pairs (channel / 2), and two channels on one timer are stuck
// with the same frequency. Channels 0 and 1 belong to the Arduino tone()
// function, and the firmware's display backlight sits on channel 4 (timer 2)
// at 20 kHz. Channel 2 puts the buzzer on timer 1, alone - so changing the
// note cannot change the screen brightness.
#define BUZZER_LEDC_CHANNEL 2

// Quadrature counts per physical click of the knob. The encoder reports four
// counts per detent in full-quad mode; one detent must produce exactly one
// onKnob() call or the game will feel like it is skipping.
#define ENCODER_COUNTS_PER_DETENT 4

// The button is noisy for a few milliseconds after it closes. Anything faster
// than this is contact bounce, not a player.
#define BUTTON_DEBOUNCE_MS 200
