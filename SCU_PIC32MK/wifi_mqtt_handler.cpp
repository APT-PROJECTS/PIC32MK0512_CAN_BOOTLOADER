#include "wifi_mqtt_handler.h"
#include "eeprom_handler.h"
#include "tcu_handler.h"
#include "mac_handler.h"

// Default values (used if EEPROM empty)
char WIFI_SSID[32] = "Mangesh";
char WIFI_PASS[32] = "Sundarsmruti@2002";

// ----------------------------------------------------
// AWS IoT SETTINGS
// ----------------------------------------------------
const char* AWS_ENDPOINT = "a1xpgudobmtdbi-ats.iot.ap-south-1.amazonaws.com";
const int AWS_PORT = 8883;
const char* MQTT_TOPIC_NAME = "ashok/function";

// ----------------------------------------------------
// FLAGS
// ----------------------------------------------------
bool wifiWasConnected = false;
bool mqttWasConnected = false;

// ----------------------------------------------------
// CERTIFICATES (EXACTLY YOURS)
// ----------------------------------------------------
// Amazon Root CA
const char* AWS_ROOT_CA = R"EOF(
-----BEGIN CERTIFICATE-----
MIIDQTCCAimgAwIBAgITBmyfz5m/jAo54vB4ikPmljZbyjANBgkqhkiG9w0BAQsF
ADA5MQswCQYDVQQGEwJVUzEPMA0GA1UEChMGQW1hem9uMRkwFwYDVQQDExBBbWF6
b24gUm9vdCBDQSAxMB4XDTE1MDUyNjAwMDAwMFoXDTM4MDExNzAwMDAwMFowOTEL
MAkGA1UEBhMCVVMxDzANBgNVBAoTBkFtYXpvbjEZMBcGA1UEAxMQQW1hem9uIFJv
b3QgQ0EgMTCCASIwDQYJKoZIhvcNAQEBBQADggEPADCCAQoCggEBALJ4gHHKeNXj
ca9HgFB0fW7Y14h29Jlo91ghYPl0hAEvrAIthtOgQ3pOsqTQNroBvo3bSMgHFzZM
9O6II8c+6zf1tRn4SWiw3te5djgdYZ6k/oI2peVKVuRF4fn9tBb6dNqcmzU5L/qw
IFAGbHrQgLKm+a/sRxmPUDgH3KKHOVj4utWp+UhnMJbulHheb4mjUcAwhmahRWa6
VOujw5H5SNz/0egwLX0tdHA114gk957EWW67c4cX8jJGKLhD+rcdqsq08p8kDi1L
93FcXmn/6pUCyziKrlA4b9v7LWIbxcceVOF34GfID5yHI9Y/QCB/IIDEgEw+OyQm
jgSubJrIqg0CAwEAAaNCMEAwDwYDVR0TAQH/BAUwAwEB/zAOBgNVHQ8BAf8EBAMC
AYYwHQYDVR0OBBYEFIQYzIU07LwMlJQuCFmcx7IQTgoIMA0GCSqGSIb3DQEBCwUA
A4IBAQCY8jdaQZChGsV2USggNiMOruYou6r4lK5IpDB/G/wkjUu0yKGX9rbxenDI
U5PMCCjjmCXPI6T53iHTfIUJrU6adTrCC2qJeHZERxhlbI1Bjjt/msv0tadQ1wUs
N+gDS63pYaACbvXy8MWy7Vu33PqUXHeeE6V/Uq2V8viTO96LXFvKWlJbYK8U90vv
o/ufQJVtMVT8QtPHRh8jrdkPSHCa2XV4cdFyQzR1bldZwgJcJmApzyMZFo6IQ6XU
5MsI+yMRQ+hDKXJioaldXgjUkK642M4UwtBV8ob2xJNDd2ZhwLnoQdeXeGADbkpy
rqXRfboQnoZsG4q5WTP468SQvvG5
-----END CERTIFICATE-----
)EOF";

// Device Certificate
const char* AWS_CERTIFICATE = R"EOF(
-----BEGIN CERTIFICATE-----
MIIDWTCCAkGgAwIBAgIUF8826VxhB/UjiBvvOC7xYbJmeFYwDQYJKoZIhvcNAQEL
BQAwTTFLMEkGA1UECwxCQW1hem9uIFdlYiBTZXJ2aWNlcyBPPUFtYXpvbi5jb20g
SW5jLiBMPVNlYXR0bGUgU1Q9V2FzaGluZ3RvbiBDPVVTMB4XDTI0MDcyNDA2NDQ0
N1oXDTQ5MTIzMTIzNTk1OVowHjEcMBoGA1UEAwwTQVdTIElvVCBDZXJ0aWZpY2F0
ZTCCASIwDQYJKoZIhvcNAQEBBQADggEPADCCAQoCggEBAK7zqFALvzBv+Njf52W8
25TIf69Qv2IDfnbIOLGCihNWnEpLFwCJHdybh+UaufnTmMT7KtlSbHLlJcqtaQqm
UI5TqyIpwpu/feYNwgBCM1G5E6ySnj/SYQJIgM/T0qrWCU0XwlvqHZvNFuiPjGKb
VGq79AJewEATVcCKgZp58PrG63mBkVDXa7+3FCKV9fM1fFxfLZYe7UBCig0ntyZt
bJs8Mv9e9pacqk1s1aPlOWysJM399EYt6LnJG7b2f9oucQgO7Bg1Du1LW/9//Gfc
V9kluq47QyxUmvGVgFL7GB7J7ihKmsua0qfRWRnh4xi0EZJ1MVf2v5ZJeLsfOWbX
pv8CAwEAAaNgMF4wHwYDVR0jBBgwFoAULD1T1oJru2hW3tycpZrG5rUWGhEwHQYD
VR0OBBYEFCKgbBfCdis/yxdh4sFwwU6CktH3MAwGA1UdEwEB/wQCMAAwDgYDVR0P
AQH/BAQDAgeAMA0GCSqGSIb3DQEBCwUAA4IBAQCve2AT1GYy6bw6qZnlvTW9slhF
DCrFDQSs2W3h3e++M+V59/PiRe5ZyqQWSDPpmjd+THSvcUGQXg89tAtDAbcHFIrn
NwFf7pYsWVfQOdECKMnxhd4TZe0/mgbCouZ0n1lIX/VzsNcZn/7d9bJJlwunD4+p
T8qAa4ysme0+W5mlEkqhU/6G+bBxt5ApL1IPB7We3koVZcEMYDd0V4mwBzVkNfKn
hawkNKHD8/KR+Catzj6QuYiKDzqb1Ni/ZUgjEQpwHcRYmlIAZ7m9YGAsWf2vZl9x
BaRMZKtUdal9B+fMPwq+icsme+wnjapPvPc3/jVW7WOqrz7WBcWW07u/2zRc
-----END CERTIFICATE-----
)EOF";

// Device Private Key
const char* AWS_PRIVATE_KEY = R"EOF(
-----BEGIN RSA PRIVATE KEY-----
MIIEowIBAAKCAQEArvOoUAu/MG/42N/nZbzblMh/r1C/YgN+dsg4sYKKE1acSksX
AIkd3JuH5Rq5+dOYxPsq2VJscuUlyq1pCqZQjlOrIinCm7995g3CAEIzUbkTrJKe
P9JhAkiAz9PSqtYJTRfCW+odm80W6I+MYptUarv0Al7AQBNVwIqBmnnw+sbreYGR
UNdrv7cUIpX18zV8XF8tlh7tQEKKDSe3Jm1smzwy/172lpyqTWzVo+U5bKwkzf30
Ri3ouckbtvZ/2i5xCA7sGDUO7Utb/3/8Z9xX2SW6rjtDLFSa8ZWAUvsYHsnuKEqa
y5rSp9FZGeHjGLQRknUxV/a/lkl4ux85Ztem/wIDAQABAoIBAByb8bEtSpWEKgBl
FPT5HjtFUkP8A1/mzF5mPYchnqys5B/uCZPn2D9EG8PmS/VDmd7UZ9xj8yISwMxU
8TFxowizO+/5Pj6z4MWnCEt8h+HcTxBQZZpwL7bs3kueCvwuxhSaRwk6zp60ugNL
vhw95F8T30In9m5D+yjiPXFicXrAmVv0qvDz8t6axXxH4mCERxH/RP07GDfDUlrJ
//GDI++WkH+eQy3zy3vKmjG7oS7ZugjWtM8J8/t4Xpy6KX/5eqqWe94lNKMsjjxd
UG25MBtjSRN/XCvLijLZYxZpoCJL1G1QRn/krFkg8zO2dtYs31watSCmvpfFcgui
p0ph57kCgYEA3+Ex9fFWV+GunIbLaVXszI0TD8mDjUoknPf4gfMDA+tXqo3F69Aw
uHiQekx5OM6exkIyLsvKSnqJlNLpBxR3QVPBnW2d7Dd7wfXQwkPvrbGZeCwdMaju
b3tp0GwGYcZzRSHgMBxEoR0uCWWTjj/tHG83h+IUSp3lrksV1SfbKksCgYEAyA1n
4P1an5IgvAtcIGU6jit+frjNaHHdMFKjTm/95vY0dS+stMaW6E9ywGBufRs/ZFxv
Bod8LHxX6fL2s6a123PHYmHaEqw6P6Z3q5SDryIXm2wC5IGE11hehVDhBB+5/Xyw
UL1CRAj4fEk6Lmu2/PMI47BpV7IvJcfagKvJxZ0CgYBKcJdrHn7GI80qUIuMD7z3
2qSO6EMslI2N/8mKc+H5yKPSmudHXGcGv1fSwmDAKbuTUmlbM/mkQBBFAsfmycBJ
YW8MSGltJJmqyXBLV8l26ImN8qUWYQQ+3zs0yOZqVlD3mKbxeES9iXVIPJUOhGTz
wmslbF+w28WF6YNt552w9wKBgFmlaD9ySQHowbKChK4frrW21+CBf8ql+7CD15p2
pRiyi59X1dTMiSNhNtLuY5S8eRPIOBh87jAhKGQ19EddFXFZ2KdFoitr8k5DkqzY
yInahSmopUMwpw65EuI69Bqc/QOStI/5v5AtVOydA+15Z27CmeEuVIZv2qDbhhbS
5IAhAoGBAKhw7CnLUb8axke0Bl7kHvRF9HkvFOkOKt9eCmLSoM8NmMpWxRipd0+R
l+LB6XBm+ffavOPOjwDrtcqh0TJLwPGzNkEozC8aiKN0+WYGqeUChsY9y7DU/VqZ
4HvwPfLpBFfazsfSxksfuvEWB2t7bCNx/BEKi0vHCccupDRlF8Ow
-----END RSA PRIVATE KEY-----
)EOF";

// ----------------------------------------------------
// GLOBAL WiFi + MQTT OBJECTS
// ----------------------------------------------------
WiFiClientSecure wifiClient;
PubSubClient mqttClient(wifiClient);

// ----------------------------------------------------
// Connect to WiFi (non-blocking, retries through list)
// ----------------------------------------------------
void connectToWiFi()
{
  if (WiFi.status() == WL_CONNECTED)
    return;

  uint8_t ssidBuf[32];
  uint8_t passBuf[32];

  // ==================================================
  // 1️⃣ TRY DEFAULT CREDENTIALS FIRST (5 ATTEMPTS)
  // ==================================================
  Serial.printf("WiFi: Trying DEFAULT credentials (SSID=%s)\n", WIFI_SSID);
  wifiWasConnected = false;

  for (int attempt = 0; attempt < DEFAULT_WIFI_CONN_ATTEMPT; attempt++) {

    WiFi.mode(WIFI_STA);
    WiFi.setAutoReconnect(false);
    WiFi.setSleep(false);
    WiFi.disconnect(true, true);
    delay(500);

    WiFi.begin(WIFI_SSID, WIFI_PASS);

    unsigned long start = millis();
    while (millis() - start < 10000) {
      if (WiFi.status() == WL_CONNECTED) {
        Serial.printf(
          "CORE0 → WiFi CONNECTED (DEFAULT): %s\n",
          WiFi.localIP().toString().c_str());
          wifiWasConnected = true;
        return;
      }
      vTaskDelay(200 / portTICK_PERIOD_MS);
    }

    Serial.printf("WiFi: DEFAULT attempt %d failed\n", attempt + 1);
    wifiWasConnected = false;
  }

  // ==================================================
  // 2️⃣ FALLBACK TO EEPROM CREDENTIALS
  // ==================================================
  EEPROM_Read(EEPROM_WIFI_SSID, ssidBuf, sizeof(ssidBuf));
  EEPROM_Read(EEPROM_WIFI_PASS, passBuf, sizeof(passBuf));

  if (!isEEPROMEmpty(ssidBuf, sizeof(ssidBuf)) &&
      !isEEPROMEmpty(passBuf, sizeof(passBuf)))
  {
    memset(WIFI_SSID, 0, sizeof(WIFI_SSID));
    memset(WIFI_PASS, 0, sizeof(WIFI_PASS));

    memcpy(WIFI_SSID, ssidBuf, sizeof(WIFI_SSID) - 1);
    memcpy(WIFI_PASS, passBuf, sizeof(WIFI_PASS) - 1);

    Serial.printf("WiFi: Trying EEPROM credentials (SSID=%s)\n", WIFI_SSID);
    wifiWasConnected = false;

    WiFi.mode(WIFI_STA);
    WiFi.setAutoReconnect(true);
    WiFi.setSleep(false);
    WiFi.disconnect(true, true);
    delay(500);

    WiFi.begin(WIFI_SSID, WIFI_PASS);

    unsigned long start = millis();
    while (millis() - start < 10000) {
      if (WiFi.status() == WL_CONNECTED) {
        Serial.printf(
          "CORE0 → WiFi CONNECTED (EEPROM): %s\n",
          WiFi.localIP().toString().c_str());
          wifiWasConnected = true;
        return;
      }
      vTaskDelay(200 / portTICK_PERIOD_MS);
    }

    Serial.println("WiFi: EEPROM credentials failed");
    wifiWasConnected = false;
  }
  else {
    Serial.println("WiFi: EEPROM credentials not valid");
    wifiWasConnected = false;
  }

  // ==================================================
  // 3️⃣ FINAL FAIL
  // ==================================================
  Serial.println("CORE0 → WiFi CONNECT FAILED (DEFAULT + EEPROM)");
  wifiWasConnected = false;
}

// ----------------------------------------------------
// Connect to MQTT (AWS IoT Core)
// ----------------------------------------------------
void connectToMQTT() {

  if (mqttClient.connected()) {
    mqttWasConnected = true;
    return;
  }

  // If we were previously connected and now not connected
  if (mqttWasConnected && !mqttClient.connected()) {
    Serial.println("MQTT disconnected! Rolling back SD index by 30...");

    if (sdReadIndex >= 30)
      sdReadIndex -= 30;
    else
      sdReadIndex = 0;

    mqttWasConnected = false;   // prevent repeated rollback
  }

  Serial.println("CORE0 → Connecting MQTT...");

  wifiClient.setCACert(AWS_ROOT_CA);
  wifiClient.setCertificate(AWS_CERTIFICATE);
  wifiClient.setPrivateKey(AWS_PRIVATE_KEY);

  mqttClient.setServer(AWS_ENDPOINT, AWS_PORT);
  mqttClient.setBufferSize(MQTT_BUFFER_SIZE);

  // ==================================================
  // 🔥 Create UNIQUE Client ID using MAC
  // ==================================================
  char clientId[32];

  sprintf(clientId, "SCU_%02X%02X%02X%02X%02X%02X",
          mac[0], mac[1], mac[2],
          mac[3], mac[4], mac[5]);

  Serial.printf("MQTT Client ID: %s\n", clientId);

  // ==================================================
  // Connect
  // ==================================================
  int retries = 0;

  while (!mqttClient.connected() && retries < 5) {

    if (mqttClient.connect(clientId)) {
      Serial.println("CORE0 → MQTT Connected!");
      mqttWasConnected = true;
      return;
    }

    retries++;
    vTaskDelay(500 / portTICK_PERIOD_MS);
  }

  Serial.println("CORE0 → MQTT connect FAILED!");
  mqttWasConnected = false;
}

// ----------------------------------------------------
// Initialize WiFi + MQTT (RUN ON CORE 0 ONLY)
// ----------------------------------------------------
void WIFI_MQTT_InitCore0() {
  WiFi.mode(WIFI_STA);

  connectToWiFi();
  connectToMQTT();

  // Serial.println("CORE0 → WiFi + MQTT INIT DONE");
}

// ----------------------------------------------------
// Check MQTT Connected
// ----------------------------------------------------
bool MQTT_IsConnected() {
  return mqttClient.connected();
}

// ----------------------------------------------------
// Publish 812 bytes to MQTT as JSON
// Format: {"D":"<HEX_STRING>"}
// ----------------------------------------------------
void MQTT_Publish812(uint8_t* buf) {
  // Ensure MQTT is connected
  if (!mqttClient.connected())
    WIFI_MQTT_InitCore0();

  mqttClient.loop();

  // ---- Convert 812 bytes to HEX string ----
  // buf = [SCU_ID(4) + RTC(8) + DATA(800)]
  String hexString;
  hexString.reserve(812 * 2);  // avoid heap fragmentation

  for (int i = 0; i < 812; i++) {
    if (buf[i] < 16) hexString += "0";
    hexString += String(buf[i], HEX);
  }

  hexString.toUpperCase();

  // ---- Wrap in JSON ----
  String jsonPayload = "{\"D\":\"" + hexString + "\"}";

  // ---- Debug print ----
  Serial.println("==== JSON Payload to Publish ====");
  // Serial.println(jsonPayload);   // uncomment only for debugging
  Serial.println("=================================");

  // ---- Publish ----
  bool ok = mqttClient.publish(
    MQTT_TOPIC_NAME,
    jsonPayload.c_str());

  if (ok) {
    Serial.println("CORE0 → MQTT Publish OK (JSON 812 bytes)");
    dataPublished = true;  // allow CORE1 to read next block
    dataPrepared = false;
  } else {
    Serial.println("CORE0 → MQTT Publish FAIL!");
    dataPublished = false;
    dataPrepared = true;
  }
}