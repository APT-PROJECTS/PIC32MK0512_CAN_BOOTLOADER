#ifndef WIFI_MQTT_HANDLER_H
#define WIFI_MQTT_HANDLER_H

#include <WiFi.h>
#include <WiFiClientSecure.h>
#include <PubSubClient.h>
#include <HTTPClient.h>
#include <ArduinoJson.h>

#define MQTT_BUFFER_SIZE 4096
#define MQTT_TOPIC "ashok/function"

#define DEFAULT_WIFI_CONN_ATTEMPT 2

// ----------------------------------------------------
// WiFi credentials (WRITABLE)
// ----------------------------------------------------
extern char WIFI_SSID[32];
extern char WIFI_PASS[32];

extern bool wifiWasConnected;
extern bool mqttWasConnected;

// ------------------------
// FUNCTIONS
// ------------------------
void WIFI_MQTT_InitCore0();  // Must be called INSIDE CORE0 task
void connectToWiFi();        // Internal WiFi connect
void connectToMQTT();        // Internal MQTT connect
void MQTT_Publish812(uint8_t* buf);
bool MQTT_IsConnected();

#endif