# CSI Motion Detector

WiFi CSI (Channel State Information) + BLE-based motion/presence detection.
An ESP32-family board sniffs WiFi channel state and BLE RSSI variance and
streams both over serial to a Qt desktop app, which fuses the two signals
into a debounced motion state.

```
┌─────────────────┐   serial (921600 baud)   ┌──────────────────────┐
│  ESP32 firmware  │ ───────────────────────► │   Qt6 desktop app     │
│  (WiFi CSI + BLE)│ ◄─────────────────────── │  (calibration, graph, │
└─────────────────┘      motion state (M/N)   │   motion fusion)       │
                                               └──────────────────────┘
```

## Hardware compatibility

CSI capture depends on Espressif's `esp_wifi` CSI API, which only exists on
certain chips. **This is not "any WiFi board" — it needs both a supported
Espressif chip and BLE.**

| Chip | WiFi CSI | Bluetooth | Works with this project |
|---|---|---|---|
| ESP32 (original) | ✅ | ✅ (BT Classic + BLE) | ✅ |
| ESP32-S3 | ✅ | ✅ (BLE) | ✅ |
| ESP32-C3 | ✅ | ✅ (BLE) | ✅ |
| ESP32-C6 | ✅ | ✅ (BLE) | ✅ (untested, should work) |
| ESP32-S2 | ✅ | ❌ no radio | ❌ no BLE, so no fusion signal |
| ESP8266 / ESP8285 | ❌ | ❌ | ❌ not an option |
| Non-Espressif WiFi modules | ❌ | — | ❌ not an option |

Any dev board built on a supported chip works (DevKitC, WROOM/WROVER
breakouts, NodeMCU-32S, TTGO/LilyGO boards, etc.) — the firmware only cares
about the chip, not the board layout. `firmware/esp32_csi_capture/board_config.h`
auto-detects the target and picks a sane default LED pin; override `LED_PIN`
as a build flag if your board's LED is on a nonstandard pin, and the sketch
will fail to compile with a clear error on an unsupported chip (e.g. S2)
rather than silently misbehaving.

## Repo layout

```
firmware/esp32_csi_capture/   Arduino sketch flashed to the ESP32
desktop-app/                  Qt6 + CMake desktop application
  src/                        Application source
  third_party/qcustomplot/    Vendored plotting library (see LICENSE note)
  scripts/build_windows.bat   One-shot MinGW/CMake build + launch for Windows
```

## Building the firmware

1. Install the [Arduino IDE](https://www.arduino.cc/en/software) (or
   PlatformIO) and add the ESP32 board package
   (`https://raw.githubusercontent.com/espressif/arduino-esp32/gh-pages/package_esp32_index.json`
   in Boards Manager URLs).
2. Open `firmware/esp32_csi_capture/esp32_csi_capture.ino`.
3. Select your board (Tools → Board → ESP32 Arduino → your board).
4. Flash it. Serial baud is `921600`.

## Building the desktop app

Requires Qt 6 (Widgets, SerialPort, PrintSupport) and CMake 3.16+.

```bash
cd desktop-app
cmake -B build -DCMAKE_PREFIX_PATH=/path/to/Qt/6.x/your_kit
cmake --build build
```

On Windows with the MinGW Qt kit, `scripts\build_windows.bat` does the above
plus `windeployqt` and launches the app — edit `QT_DIR` / `MINGW_DIR` at the
top of the script to match your install paths first.

## Serial protocol

Binary frames: `0xAA 0x55 <type:1> <len:2 LE> <payload> <xor checksum:1>`
- `type 0x01` — CSI packet (`rssi:1` + I/Q byte pairs)
- `type 0x02` — BLE packet (`avgRssi:2 LE` + `deviceCount:1`)

Plain-text commands (newline-terminated) sent host → device:
`SCAN`, `CONNECT|ssid|password`, `SNIFF|channel`, `RECONNECT`, `M`/`N` (motion
state, drives the status LED).

## License

MIT for this project's own code — see [LICENSE](LICENSE). QCustomPlot in
`desktop-app/third_party/qcustomplot/` ships under its own GPL/commercial
terms; check that license before redistributing binaries.
