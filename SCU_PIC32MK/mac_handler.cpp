#include "mac_handler.h"

// Define global MAC buffer
uint8_t mac[6];

void MAC_Init(void) {
  esp_read_mac(mac, ESP_MAC_WIFI_STA);

  Serial.printf(
    "ESP32 WiFi STA MAC: %02X:%02X:%02X:%02X:%02X:%02X\n",
    mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);
}
