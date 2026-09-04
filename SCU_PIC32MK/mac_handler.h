#ifndef MAC_HANDLER_H
#define MAC_HANDLER_H

#include <Arduino.h>
#include <esp_system.h>
#include "esp_mac.h"

// Global MAC buffer (STA)
extern uint8_t mac[6];

// Read ESP32 WiFi STA MAC into mac[6]
void MAC_Init(void);

#endif
