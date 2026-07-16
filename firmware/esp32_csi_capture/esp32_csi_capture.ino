#include <WiFi.h>
#include <string.h>
#include <Preferences.h>
#include <BLEDevice.h>
#include <BLEScan.h>
#include <BLEAdvertisedDevice.h>
extern "C" {
  #include "esp_wifi.h"
}
#include "board_config.h"

const uint32_t CSI_PING_INTERVAL_MS = 300;
const uint32_t LED_TIMEOUT_MS = 2000;
const uint32_t BLE_SCAN_INTERVAL_MS = 1000;
const uint32_t BLE_SCAN_DURATION_SEC = 1;
const uint32_t RECONNECT_RETRY_MS = 3000;

enum Mode { MODE_IDLE, MODE_CONNECTED, MODE_SNIFFING };
Mode currentMode = MODE_IDLE;

WiFiClient tcpClient;
IPAddress gatewayIP;
uint32_t lastPing = 0;
uint32_t lastMotionCmd = 0;
uint32_t pingCount = 0;
uint32_t lastPingReport = 0;
uint32_t lastBleScan = 0;
uint32_t lastReconnectAttempt = 0;

String currentSsid = "";
String currentPassword = "";
uint8_t targetBssid[6] = {0, 0, 0, 0, 0, 0};
bool bssidFilterEnabled = false;

Preferences prefs;

BLEScan *pBLEScan = nullptr;
long bleRssiSum = 0;
int bleDeviceCount = 0;

class BleScanCallbacks : public BLEAdvertisedDeviceCallbacks {
  void onResult(BLEAdvertisedDevice advertisedDevice) override {
    bleRssiSum += advertisedDevice.getRSSI();
    bleDeviceCount++;
  }
};

String cmdBuffer = "";

uint8_t csiPayload[513];

void sendBinaryFrame(uint8_t type, const uint8_t *payload, uint16_t len) {
  uint8_t header[5];
  header[0] = 0xAA;
  header[1] = 0x55;
  header[2] = type;
  header[3] = (uint8_t)(len & 0xFF);
  header[4] = (uint8_t)((len >> 8) & 0xFF);
  Serial.write(header, 5);
  if (len > 0) {
    Serial.write(payload, len);
  }
  uint8_t checksum = 0;
  for (uint16_t i = 0; i < len; i++) {
    checksum ^= payload[i];
  }
  Serial.write(&checksum, 1);
}

void wifi_promiscuous_rx_cb(void *buf, wifi_promiscuous_pkt_type_t type) {
}

void wifi_csi_rx_cb(void *ctx, wifi_csi_info_t *info) {
  if (!info || !info->buf) return;

  if (bssidFilterEnabled) {
    if (memcmp(info->mac, targetBssid, 6) != 0) {
      return;
    }
  }

  wifi_csi_info_t d = *info;
  int8_t *csi_data = d.buf;
  int len = d.len;
  if (len > 512) len = 512;

  csiPayload[0] = (uint8_t)d.rx_ctrl.rssi;
  memcpy(csiPayload + 1, csi_data, len);
  sendBinaryFrame(0x01, csiPayload, len + 1);
}

void enableCsiAndPromiscuous() {
  wifi_csi_config_t csi_config;
  memset(&csi_config, 0, sizeof(csi_config));
  csi_config.lltf_en = true;
  csi_config.htltf_en = true;
  csi_config.stbc_htltf2_en = true;
  csi_config.ltf_merge_en = true;

  ESP_ERROR_CHECK(esp_wifi_set_csi_config(&csi_config));
  ESP_ERROR_CHECK(esp_wifi_set_csi_rx_cb(&wifi_csi_rx_cb, NULL));
  ESP_ERROR_CHECK(esp_wifi_set_csi(true));

  wifi_promiscuous_filter_t filter = {
    .filter_mask = WIFI_PROMIS_FILTER_MASK_MGMT | WIFI_PROMIS_FILTER_MASK_DATA | WIFI_PROMIS_FILTER_MASK_CTRL
  };
  ESP_ERROR_CHECK(esp_wifi_set_promiscuous_filter(&filter));
  ESP_ERROR_CHECK(esp_wifi_set_promiscuous_rx_cb(&wifi_promiscuous_rx_cb));
  ESP_ERROR_CHECK(esp_wifi_set_promiscuous(true));
}

void doScan() {
  WiFi.mode(WIFI_STA);
  WiFi.disconnect();
  delay(100);
  int n = WiFi.scanNetworks();
  for (int i = 0; i < n; i++) {
    Serial.print("NET|");
    Serial.print(i);
    Serial.print("|");
    Serial.print(WiFi.SSID(i));
    Serial.print("|");
    Serial.print(WiFi.channel(i));
    Serial.print("|");
    Serial.println(WiFi.RSSI(i));
  }
  Serial.println("SCANDONE");
}

bool attemptConnect(const String &ssid, const String &password, uint32_t timeoutMs) {
  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid.c_str(), password.c_str());

  uint32_t start = millis();
  while (WiFi.status() != WL_CONNECTED) {
    delay(300);
    if (millis() - start > timeoutMs) {
      return false;
    }
  }
  return true;
}

void finalizeConnected() {
  esp_wifi_set_ps(WIFI_PS_NONE);
  esp_wifi_config_80211_tx_rate(WIFI_IF_STA, WIFI_PHY_RATE_MCS0_LGI);
  gatewayIP = WiFi.gatewayIP();

  uint8_t *bssid = WiFi.BSSID();
  if (bssid) {
    memcpy(targetBssid, bssid, 6);
    bssidFilterEnabled = true;
  }

  Serial.print("Connected. IP: ");
  Serial.println(WiFi.localIP());
  Serial.print("Gateway IP: ");
  Serial.println(gatewayIP);
  Serial.print("WiFi channel: ");
  Serial.println(WiFi.channel());
  Serial.print("RSSI to AP: ");
  Serial.println(WiFi.RSSI());
  Serial.print("BSSID filter locked to: ");
  Serial.println(WiFi.BSSIDstr());

  enableCsiAndPromiscuous();
  currentMode = MODE_CONNECTED;
  pingCount = 0;
  lastPing = millis();
  lastPingReport = millis();
  lastReconnectAttempt = millis();
  Serial.println("READY,CONNECTED");
}

void doConnect(const String &ssid, const String &password) {
  bssidFilterEnabled = false;
  Serial.print("Connecting to ");
  Serial.println(ssid);

  if (!attemptConnect(ssid, password, 20000)) {
    Serial.println("CONNECT_FAILED,timeout");
    return;
  }

  currentSsid = ssid;
  currentPassword = password;
  prefs.begin("csicfg", false);
  prefs.putString("ssid", ssid);
  prefs.putString("pass", password);
  prefs.end();

  finalizeConnected();
}

void doReconnectStored() {
  bssidFilterEnabled = false;
  prefs.begin("csicfg", true);
  String ssid = prefs.getString("ssid", "");
  String password = prefs.getString("pass", "");
  prefs.end();

  if (ssid.length() == 0) {
    Serial.println("RECONNECT_FAILED,no_stored_credentials");
    return;
  }

  Serial.print("Reconnecting to stored network ");
  Serial.println(ssid);

  if (!attemptConnect(ssid, password, 20000)) {
    Serial.println("CONNECT_FAILED,timeout");
    return;
  }

  currentSsid = ssid;
  currentPassword = password;
  finalizeConnected();
}

void doSniff(int channel) {
  WiFi.mode(WIFI_STA);
  WiFi.disconnect();
  delay(100);
  bssidFilterEnabled = false;

  esp_err_t chErr = esp_wifi_set_channel(channel, WIFI_SECOND_CHAN_NONE);
  if (chErr != ESP_OK) {
    Serial.print("SNIFF_FAILED,channel_error,");
    Serial.println(chErr);
    return;
  }
  Serial.print("Locked to channel: ");
  Serial.println(channel);

  enableCsiAndPromiscuous();
  currentMode = MODE_SNIFFING;
  Serial.println("READY,SNIFFING");
}

void handleCommand(String line) {
  line.trim();
  if (line.length() == 0) return;

  if (line == "SCAN") {
    doScan();
  } else if (line.startsWith("CONNECT|")) {
    int firstBar = line.indexOf('|');
    int secondBar = line.indexOf('|', firstBar + 1);
    if (secondBar > 0) {
      String ssid = line.substring(firstBar + 1, secondBar);
      String password = line.substring(secondBar + 1);
      doConnect(ssid, password);
    }
  } else if (line.startsWith("SNIFF|")) {
    int bar = line.indexOf('|');
    int channel = line.substring(bar + 1).toInt();
    if (channel >= 1 && channel <= 14) {
      doSniff(channel);
    }
  } else if (line == "RECONNECT") {
    doReconnectStored();
  } else if (line == "M") {
    digitalWrite(LED_PIN, HIGH);
    lastMotionCmd = millis();
  } else if (line == "N") {
    digitalWrite(LED_PIN, LOW);
    lastMotionCmd = millis();
  }
}

void setup() {
  Serial.begin(921600);
  delay(1000);

  pinMode(LED_PIN, OUTPUT);
  digitalWrite(LED_PIN, LOW);

  BLEDevice::init("");
  pBLEScan = BLEDevice::getScan();
  pBLEScan->setAdvertisedDeviceCallbacks(new BleScanCallbacks(), true);
  pBLEScan->setActiveScan(false);
  pBLEScan->setInterval(100);
  pBLEScan->setWindow(99);

  Serial.println("BOOT,waiting for command (SCAN / CONNECT|ssid|pass / SNIFF|channel / RECONNECT)");
}

void loop() {
  while (Serial.available() > 0) {
    char c = (char)Serial.read();
    if (c == '\n') {
      handleCommand(cmdBuffer);
      cmdBuffer = "";
    } else if (c != '\r') {
      cmdBuffer += c;
    }
  }

  if (currentMode == MODE_CONNECTED) {
    if (WiFi.status() != WL_CONNECTED) {
      if (millis() - lastReconnectAttempt >= RECONNECT_RETRY_MS) {
        lastReconnectAttempt = millis();
        Serial.println("RECONNECTING");
        WiFi.begin(currentSsid.c_str(), currentPassword.c_str());
      }
    } else {
      if (millis() - lastPing >= CSI_PING_INTERVAL_MS) {
        lastPing = millis();
        if (tcpClient.connect(gatewayIP, 80, 200)) {
          tcpClient.print("GET / HTTP/1.1\r\nHost: router\r\nConnection: close\r\n\r\n");
          uint32_t drainStart = millis();
          while (tcpClient.connected() && millis() - drainStart < 100) {
            while (tcpClient.available()) {
              tcpClient.read();
            }
          }
          tcpClient.stop();
          pingCount++;
        }
      }

      if (millis() - lastPingReport >= 1000) {
        lastPingReport = millis();
        Serial.print("PINGRATE,");
        Serial.println(pingCount);
        pingCount = 0;
      }
    }
  }

  if (millis() - lastMotionCmd > LED_TIMEOUT_MS) {
    digitalWrite(LED_PIN, LOW);
  }

  if (currentMode != MODE_IDLE && millis() - lastBleScan >= BLE_SCAN_INTERVAL_MS) {
    lastBleScan = millis();
    bleRssiSum = 0;
    bleDeviceCount = 0;
    pBLEScan->start(BLE_SCAN_DURATION_SEC, false);

    if (bleDeviceCount > 0) {
      uint8_t blePayload[3];
      int16_t avgRssi = (int16_t)(bleRssiSum / bleDeviceCount);
      blePayload[0] = (uint8_t)(avgRssi & 0xFF);
      blePayload[1] = (uint8_t)((avgRssi >> 8) & 0xFF);
      blePayload[2] = (uint8_t)bleDeviceCount;
      sendBinaryFrame(0x02, blePayload, 3);
    }
    pBLEScan->clearResults();
  }
}
