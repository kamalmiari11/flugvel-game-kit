# Host-side check for the device target

The device firmware needs the ESP32 toolchain to build, which CI does not
have and which is a 500 MB download for a change that touches one line. These
headers stand in for the four libraries the harness uses, so
`make device-check` can compile `device/src/` on an ordinary machine and catch
the mistakes that actually happen: a method that does not exist, an argument
in the wrong place, an enum that is really a macro.

```sh
make device-check      # seconds, no toolchain
cd device && pio run   # the real thing, needs PlatformIO
```

**These are not the real libraries.** They declare the exact signatures the
harness calls and nothing else, and they have no bodies - the check is
`-fsyntax-only`. A green `device-check` means the code is consistent with the
API; only `pio run` proves it builds, and only flashing proves it runs.

Signatures were taken from, and should be re-checked against, upstream:

| stub | upstream |
| --- | --- |
| `TFT_eSPI.h` | `Bodmer/TFT_eSPI` |
| `ESP32Encoder.h` | `madhephaestus/ESP32Encoder` |
| `Arduino.h`, `Preferences.h` | `espressif/arduino-esp32`, branch `release/v2.x` |

The v2.x branch matters: `platformio.ini` pins the platform to the 6.x line
for exactly that reason, and these stubs describe that API. If the pin ever
moves to core 3.x, the LEDC declarations here are the first thing to change.
