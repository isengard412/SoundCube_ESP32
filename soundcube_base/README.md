# 🔊 Soundcube

**A Bluetooth A2DP speaker firmware for the ESP32 with a 10-band equalizer, NVS settings persistence, and a live serial console.**

Built on [ESP-IDF v5.5](https://github.com/espressif/esp-idf) using the Bluedroid Classic Bluetooth stack.

---

## Features

| | |
|---|---|
| 🎵 **Bluetooth A2DP Sink** | Pairs as a standard Bluetooth speaker — works with any phone, tablet or computer |
| 🎛️ **10-Band Parametric EQ** | Software biquad equalizer covering 31 Hz – 16 kHz, adjustable at runtime via serial |
| 💾 **NVS Persistence** | EQ settings survive reboots — stored in flash, loaded automatically on startup |
| 🔈 **Mono Mix Mode** | Compile-time option to sum L+R into a mono signal for single-speaker builds |
| 🔁 **Pairing Button** | GPIO button (short or long press) triggers discoverable pairing mode |
| 🖥️ **Serial Console** | Live command-line interface over USB-UART for EQ tuning and device control |
| 🔌 **ES8388 Codec** | I²C initialisation for the ES8388 audio codec with I2S output |

---

## Hardware

### Required

| Component | Notes |
|---|---|
| **ESP32** | Any module with Bluetooth Classic support |
| **ES8388 audio codec** | Connected via I²C (config & init) and I2S (audio data) |
| **Speaker / amplifier** | Connected to the ES8388 line/headphone output |

### Pin Mapping

| ESP32 GPIO | Signal | Notes |
|---|---|---|
| GPIO 18 | I²C SDA | ES8388 control |
| GPIO 23 | I²C SCL | ES8388 control |
| GPIO 25 | I2S DATA | Audio out |
| GPIO 26 | I2S BCK | Bit clock |
| GPIO 27 | I2S LRCK | Word select |
| GPIO 39 | Button | Pairing trigger (active low) |

> Pins can be changed in `sdkconfig` / `menuconfig`.

---

## Getting Started

### Prerequisites

- [ESP-IDF v5.5](https://docs.espressif.com/projects/esp-idf/en/v5.5/esp32/get-started/index.html) installed and sourced
- ESP32 board connected via USB

### Build & Flash

```bash
# Source the ESP-IDF environment
source $IDF_PATH/export.sh

# Configure (optional — set device name, codec selection, pin mapping)
idf.py menuconfig

# Build
idf.py build

# Flash and open monitor
idf.py flash monitor
```

The device will appear as **`ESP_SPEAKER`** (configurable in `menuconfig → A2DP Example Configuration`).

---

## Serial Console

Connect at **115200 baud**. A prompt appears after boot:

```
soundcube>
```

### Commands

| Command | Description |
|---|---|
| `eq <0–9> <gain>` | Set EQ band gain in dB (–12 to +12). Band 0 = 31 Hz, band 9 = 16 kHz |
| `eq show` | Display all 10 bands and their current gain |
| `eq reset` | Reset all bands to 0 dB (flat) |
| `save` | Persist current EQ settings to flash |
| `pair` | Enter Bluetooth pairing / discoverable mode |
| `stfu` | Mute all background log output until next reboot |
| `yolo` | Soft-reset the device |
| `help` | Show command reference |

### Example session

```
soundcube> eq show
 10-Band Equalizer:
 -----------------------
  31Hz:   0.0 dB
  62Hz:   0.0 dB
 125Hz:  +3.0 dB
 250Hz:  +3.0 dB
 500Hz:   0.0 dB
  1kHz:  -2.0 dB
  2kHz:   0.0 dB
  4kHz:   0.0 dB
  8kHz:  +1.5 dB
 16kHz:  +1.5 dB
 -----------------------
soundcube> eq 0 6
Band 0 (31Hz) set to 6.0 dB
soundcube> save
EQ settings saved to flash.
```

---

## EQ Band Reference

| Band | Frequency | Suggested use |
|---|---|---|
| 0 | 31 Hz | Sub-bass |
| 1 | 62 Hz | Bass body |
| 2 | 125 Hz | Upper bass / warmth |
| 3 | 250 Hz | Low-mids |
| 4 | 500 Hz | Mids |
| 5 | 1 kHz | Upper-mids / presence |
| 6 | 2 kHz | Presence / attack |
| 7 | 4 kHz | Clarity |
| 8 | 8 kHz | Air / sibilance |
| 9 | 16 kHz | Brilliance / sparkle |

---

## Mono Mix Mode

For single-speaker builds, enable mono mixing in `main/equalizer.h`:

```c
#define EQUALIZER_MONO_MIX
```

When active, both stereo input channels are summed into one mono signal **before** EQ processing, and the result is written to both output channels. This also halves EQ CPU load since only one filter chain runs. Comment the line out to restore independent stereo processing.

---

## Project Structure

```
main/
├── main.c           — App entry point, BT stack setup, A2DP callbacks
├── equalizer.c/h    — 10-band biquad EQ engine
├── storage.c/h      — NVS load/save for EQ settings
├── console.c/h      — UART command console
├── es8388_init.c/h  — ES8388 codec I²C initialisation
├── button.c/h       — GPIO button with debounce and short/long-press detection
└── Kconfig.projbuild — menuconfig options (device name, codec selection)
```

---

## Configuration Reference

All options are available via `idf.py menuconfig → A2DP Example Configuration`:

| Option | Default | Description |
|---|---|---|
| `CONFIG_EXAMPLE_LOCAL_DEVICE_NAME` | `ESP_SPEAKER` | Bluetooth device name visible during pairing |
| `CONFIG_EXAMPLE_A2DP_SINK_USE_EXTERNAL_CODEC` | `n` | Use external (undecoded) audio data path |
| I2S pin options | see above | BCK, LRCK, DATA GPIOs |

---

## License

`SPDX-License-Identifier: GPL-3.0-or-later`

This project is licensed under the **GNU General Public License v3.0 or later**.
See the [LICENSE](LICENSE) file for the full text.

> **Dependency note:** ESP-IDF is licensed under Apache 2.0, which is compatible with GPL v3.
> If you fork this project, be aware that GPL v2 (without the "or later" clause) is **not** compatible with Apache 2.0.


## VS CODE

Action	Shortcut	What happens
Build	Ctrl+Shift+B	Runs idf.py build (default task)
Flash	Ctrl+Shift+P → Run Task → ESP-IDF: Flash	Flashes to the connected device
Flash & Monitor	Ctrl+Shift+P → Run Task → ESP-IDF: Flash & Monitor	Flashes + opens serial monitor