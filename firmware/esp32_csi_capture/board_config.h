#pragma once

// ---------------------------------------------------------------------------
// board_config.h
//
// Centralizes the only two things that actually differ between ESP32-family
// boards for this project: which GPIO drives the status LED, and whether the
// chip has a radio that can run WiFi CSI capture + BLE scanning at the same
// time. Everything else in esp32_csi_capture.ino is chip-agnostic.
//
// Supported: original ESP32, ESP32-S3, ESP32-C3, ESP32-C6 (any board built
//            on these chips - DevKitC, WROOM/WROVER modules, NodeMCU-32S,
//            TTGO/LilyGO boards, etc.)
// NOT supported: ESP32-S2 (no Bluetooth radio at all), ESP8266/ESP8285 (no
//            CSI API, no BLE), and any non-Espressif WiFi module.
// ---------------------------------------------------------------------------

#if defined(CONFIG_IDF_TARGET_ESP32S2)
  #error "ESP32-S2 has no Bluetooth radio - this project needs WiFi CSI + BLE together. Use an ESP32, ESP32-S3, ESP32-C3, or ESP32-C6 board instead."
#endif

#if !defined(CONFIG_IDF_TARGET_ESP32) && !defined(CONFIG_IDF_TARGET_ESP32S3) && \
    !defined(CONFIG_IDF_TARGET_ESP32C3) && !defined(CONFIG_IDF_TARGET_ESP32C6)
  #warning "This chip hasn't been validated with this project. It needs the esp_wifi CSI API and a working BLE stack - check your target before flashing."
#endif

// --- Status LED pin ---------------------------------------------------------
// Most ESP32 DevKit boards use GPIO 2 for the onboard LED. Boards that don't
// (or that don't expose one at all) can override this by defining LED_PIN as
// a build flag in platformio.ini, or by editing the default below.
#if defined(LED_PIN)
  // Already provided externally (e.g. -DLED_PIN=48 in platformio.ini)
#elif defined(LED_BUILTIN)
  #define LED_PIN LED_BUILTIN
#else
  #define LED_PIN 2
#endif

// --- CSI capability flag -----------------------------------------------------
// All currently-supported targets share the same esp_wifi CSI config struct,
// so no per-chip branching is needed here today. This flag exists so future
// chip-specific quirks have a single place to live.
#define BOARD_SUPPORTS_CSI 1
