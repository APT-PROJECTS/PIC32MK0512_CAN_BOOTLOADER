#include "config_handler.h"
#include "eeprom_handler.h"
#include "wifi_mqtt_handler.h"
#include "tcu_handler.h"
#include "mac_handler.h"
#include "mcp23017_handler.h"

// --------------------------
// INTERNAL HELPERS
// --------------------------
static void macToString(const uint8_t *mac, char *out) {
  // Format: AA:AA:AA:AA:AA:AA
  sprintf(out,
          "%02X:%02X:%02X:%02X:%02X:%02X",
          mac[0], mac[1], mac[2],
          mac[3], mac[4], mac[5]);
}

// --------------------------
// MAIN HANDLER
// --------------------------
bool Config_Handler_Run(const uint8_t *mac, uint8_t *scu_id) {
  static ConfigState_t state = CFG_STATE_IDLE;
  static ConfigSubState_t subState = CFG_SUB_SCU_ID;

  static bool updateNeeded = false;
  static bool configFetched = false;
  static DynamicJsonDocument cfg(512);

  if (WiFi.status() != WL_CONNECTED)
    return false;

  char macStr[18];
  char url[128];
  HTTPClient http;

  macToString(mac, macStr);

  switch (state) {

    // =================================================
    case CFG_STATE_IDLE:
      state = CFG_STATE_CHECK_UPDATE;
      break;

    // =================================================
    case CFG_STATE_CHECK_UPDATE:
      snprintf(url, sizeof(url),
               "%s/check-update/%s",
               API_BASE_URL, macStr);

      http.begin(url);
      if (http.GET() != HTTP_CODE_OK) {
        http.end();
        break;  // 🔁 retry
      }

      {
        DynamicJsonDocument doc(256);
        if (deserializeJson(doc, http.getString())) {
          http.end();
          break;  // 🔁 retry
        }

        updateNeeded = doc["updateNeeded"] | false;
      }

      http.end();

      if (updateNeeded) {
        configFetched = false;
        subState = CFG_SUB_SCU_ID;
        state = CFG_STATE_GET_CONFIG;
      } else {
        state = CFG_STATE_DONE;
      }
      break;

    // =================================================
    case CFG_STATE_GET_CONFIG:

      // -------- FETCH CONFIG ONCE --------
      if (!configFetched) {

        snprintf(url, sizeof(url),
                 "%s/get-configs/%s",
                 API_BASE_URL, macStr);

        http.begin(url);
        if (http.GET() != HTTP_CODE_OK) {
          http.end();
          break;  // 🔁 retry
        }

        if (deserializeJson(cfg, http.getString())) {
          http.end();
          break;  // 🔁 retry
        }

        http.end();
        configFetched = true;
      }

      // -------- SUB STATE MACHINE --------
      switch (subState) {

        // ---------------- SCU ID ----------------
        case CFG_SUB_SCU_ID:
          {
            const char *scuStr = cfg["device_vcu"];
            if (!scuStr || strlen(scuStr) != 8)
              break;  // 🔁 retry

            for (int i = 0; i < 4; i++) {
              char b[3] = { scuStr[i * 2], scuStr[i * 2 + 1], 0 };
              scu_id[i] = strtoul(b, NULL, 16);
            }

            EEPROM_Write(EEPROM_SCU_ADDR, scu_id, 4);
            Serial.println("CFG: SCU ID updated");

            subState = CFG_SUB_WIFI;
            break;
          }

        // ---------------- WIFI CONFIG ----------------
        case CFG_SUB_WIFI:
          {
            const char *ssid = cfg["device_config_ssid"];
            const char *pass = cfg["device_config_password"];

            if (ssid && pass) {
              memset(WIFI_SSID, 0, sizeof(WIFI_SSID));
              memset(WIFI_PASS, 0, sizeof(WIFI_PASS));

              strncpy(WIFI_SSID, ssid, sizeof(WIFI_SSID) - 1);
              strncpy(WIFI_PASS, pass, sizeof(WIFI_PASS) - 1);

              EEPROM_Write(EEPROM_WIFI_SSID,
                           (uint8_t *)WIFI_SSID,
                           sizeof(WIFI_SSID));

              EEPROM_Write(EEPROM_WIFI_PASS,
                           (uint8_t *)WIFI_PASS,
                           sizeof(WIFI_PASS));

              Serial.printf("CFG: WiFi updated (SSID=%s)\n", WIFI_SSID);
            }

            subState = CFG_SUB_LIVE_MODE;
            break;
          }

        // ---------------- LIVE MODE ----------------
        case CFG_SUB_LIVE_MODE:
          {
            bool deviceLive = cfg["device_live"] | false;

            if (deviceLive) {
              sdReadIndex = sdWriteIndex;
              Serial.println("CFG: DEVICE LIVE MODE → SD sync");
            }

            subState = CFG_SUB_FINISH;
            break;
          }

        // ---------------- FINISH ----------------
        case CFG_SUB_FINISH:
          state = CFG_STATE_DONE_UPDATE;
          break;
      }
      break;

    // =================================================
    case CFG_STATE_DONE_UPDATE:
      snprintf(url, sizeof(url),
               "%s/done-update/%s",
               API_BASE_URL, macStr);

      http.begin(url);
      http.sendRequest("DELETE");
      http.end();

      Serial.println("CFG: Update completed");
      WiFi.disconnect(true, true);
      state = CFG_STATE_DONE;
      break;

    // =================================================
    case CFG_STATE_DONE:
      state = CFG_STATE_IDLE;
      return true;
  }

  return false;
}

void CORE0_CONFIG_Task(void *p) {
  while (1) {

    if (WiFi.status() == WL_CONNECTED) {
      Config_Handler_Run(mac, SCU_ID);
    }

    // Slow API polling (server-friendly)
    vTaskDelay(3000 / portTICK_PERIOD_MS);
  }
}
