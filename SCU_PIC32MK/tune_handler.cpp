#include "tune_handler.h"
#include "mac_handler.h"
#include "can_handler.h"

// ---- Globals ----
uint8_t tuner_read = 0;
uint8_t tuner_write = 0;
uint8_t zeroAngleEstimate = 0;

float tuner_params[44];

static uint8_t received_count = 0;
static bool received_flags[44] = { 0 };

static bool readSuccessMCU = false;
static bool readSuccessServer = false;
static bool writeSuccessMCU = false;
static bool mcuOffline = false;
static bool zeroAngleEstimateReadMCU = false;

void CORE0_TUNER_Task(void* pvParameters) {
  CORE0_TUNNER_STATE state = CORE0_TUNNER_CHECK_STATUS;

  char url[128];
  char macStr[32];

  while (1) {
    switch (state) {
      // --------------------------------------------------
      case CORE0_TUNNER_CHECK_STATUS:
        {
          if (WiFi.status() == WL_CONNECTED) {
            sprintf(macStr, "%02X:%02X:%02X:%02X:%02X:%02X",
                    mac[0], mac[1], mac[2],
                    mac[3], mac[4], mac[5]);

            sprintf(url,
                    "http://vehicle.aproposdrive.com/api/status/%s",
                    macStr);

            HTTPClient http;
            http.begin(url);

            int httpCode = http.GET();

            if (httpCode > 0) {
              String payload = http.getString();

              StaticJsonDocument<256> doc;
              if (!deserializeJson(doc, payload)) {
                tuner_read = atoi(doc["read"]);
                tuner_write = atoi(doc["write"]);
                zeroAngleEstimate = atoi(doc["zeroAngleEstimate"]);

                // 🔥 Decision making
                if (tuner_read == 1) {
                  state = CORE0_TUNNER_READ;
                } else if (tuner_write == 1) {
                  state = CORE0_TUNNER_WRITE;
                } else if (zeroAngleEstimate == 1) {
                  state = CORE0_TUNNER_ZERO_ANGLE_ESTIMATE;
                } else {
                  state = CORE0_TUNNER_DONE;
                }
              } else {
                state = CORE0_TUNNER_ERROR;
              }
            } else {
              state = CORE0_TUNNER_ERROR;
            }

            http.end();
          } else {
            state = CORE0_TUNNER_ERROR;
          }
          break;
        }

      // --------------------------------------------------
      case CORE0_TUNNER_READ:
        {
          if (readSuccessMCU) {
            if (WiFi.status() == WL_CONNECTED) {
              char url[128];
              char macStr[32];

              sprintf(macStr, "%02X:%02X:%02X:%02X:%02X:%02X",
                      mac[0], mac[1], mac[2],
                      mac[3], mac[4], mac[5]);

              sprintf(url,
                      "http://vehicle.aproposdrive.com/api/parameters/%s",
                      macStr);

              HTTPClient http;
              http.begin(url);
              http.addHeader("Content-Type", "application/json");

              // ---- Build JSON ----
              StaticJsonDocument<2048> doc;

              doc["ProjectID"] = "P001";
              doc["FirmwareID"] = "FW_1.0";
              doc["PIC_ID_High"] = "ABCD";
              doc["PIC_ID_Low"] = "1234";

              // ---- Add T0 → T43 ----
              for (int i = 0; i < 44; i++) {
                char key[6];  // "T43\0"
                sprintf(key, "T%d", i);

                doc[key] = tuner_params[i];
              }

              // ---- Serialize JSON ----
              String body;
              serializeJson(doc, body);

              Serial.println("POST Body:");
              Serial.println(body);

              // ---- Send POST ----
              int httpCode = http.POST(body);

              if (httpCode > 0) {
                String response = http.getString();
                Serial.println("POST Success:");
                Serial.println(response);
              } else {
                Serial.println("POST Failed");
              }

              http.end();
            } else {
              Serial.println("WiFi not connected");
            }

            // ---- Reset flag ----
            readSuccessMCU = false;

            state = CORE0_TUNNER_DONE;
          }

          break;
        }

      // --------------------------------------------------
      case CORE0_TUNNER_WRITE:
        {
          if (WiFi.status() == WL_CONNECTED) {
            char url[128];
            char macStr[32];

            sprintf(macStr, "%02X:%02X:%02X:%02X:%02X:%02X",
                    mac[0], mac[1], mac[2],
                    mac[3], mac[4], mac[5]);

            sprintf(url,
                    "http://vehicle.aproposdrive.com/api/parameters/%s",
                    macStr);

            HTTPClient http;
            http.begin(url);

            int httpCode = http.GET();

            if (httpCode > 0) {
              String payload = http.getString();

              Serial.println("WRITE API Response:");
              Serial.println(payload);

              StaticJsonDocument<3072> doc;

              if (!deserializeJson(doc, payload)) {
                // ---- Extract T0 → T43 ----
                for (int i = 0; i < 44; i++) {
                  char key[6];
                  sprintf(key, "T%d", i);

                  // JSON gives string → convert to float
                  const char* valStr = doc[key];

                  if (valStr != NULL) {
                    tuner_params[i] = atof(valStr);
                  } else {
                    tuner_params[i] = 0.0f;
                  }
                }

                // ---- Set flag ----
                readSuccessServer = true;

                Serial.println("Server params loaded successfully");

                state = CORE0_TUNNER_CHECK_WRITE_SUCCESS;
              } else {
                Serial.println("JSON Parse Error");
                state = CORE0_TUNNER_ERROR;
              }
            } else {
              Serial.println("HTTP GET Failed");
              state = CORE0_TUNNER_ERROR;
            }

            http.end();
          } else {
            Serial.println("WiFi not connected");
            state = CORE0_TUNNER_ERROR;
          }

          break;
        }

      // --------------------------------------------------
      case CORE0_TUNNER_CHECK_WRITE_SUCCESS:
        {
          if (writeSuccessMCU == true || mcuOffline == true) {
            if (WiFi.status() == WL_CONNECTED) {
              char urlStatus[128];
              char urlAck[128];
              char macStr[32];

              sprintf(macStr, "%02X:%02X:%02X:%02X:%02X:%02X",
                      mac[0], mac[1], mac[2],
                      mac[3], mac[4], mac[5]);

              HTTPClient http;

              // -----------------------------
              // 1. ACK API (FIRST)
              // -----------------------------
              sprintf(urlAck,
                      "http://vehicle.aproposdrive.com/api/ack/%s",
                      macStr);

              http.begin(urlAck);
              http.addHeader("Content-Type", "application/json");

              StaticJsonDocument<128> ackDoc;

              if (writeSuccessMCU == true) {
                ackDoc["ackValue"] = "1";
                Serial.println("ACK SUCCESS");
              } else if (mcuOffline == true) {
                ackDoc["ackValue"] = "-1";
                Serial.println("ACK FAIL (MCU OFFLINE)");
              }

              String ackBody;
              serializeJson(ackDoc, ackBody);

              int ackCode = http.POST(ackBody);

              if (ackCode > 0) {
                Serial.println("ACK sent");
              } else {
                Serial.println("ACK POST failed");
              }

              http.end();

              // -----------------------------
              // 2. STATUS API (SECOND)
              // -----------------------------
              sprintf(urlStatus,
                      "http://vehicle.aproposdrive.com/api/status/%s",
                      macStr);

              http.begin(urlStatus);
              http.addHeader("Content-Type", "application/json");

              StaticJsonDocument<128> statusDoc;
              statusDoc["read"] = "0";
              statusDoc["write"] = "0";
              statusDoc["zeroAngleEstimate"] = "0";

              String statusBody;
              serializeJson(statusDoc, statusBody);

              int statusCode = http.POST(statusBody);

              if (statusCode > 0) {
                Serial.println("Status updated");
              } else {
                Serial.println("Status POST failed");
              }

              http.end();

              // -----------------------------
              // Reset flags
              // -----------------------------
              writeSuccessMCU = false;
              mcuOffline = false;

              state = CORE0_TUNNER_DONE;
            } else {
              Serial.println("WiFi not connected");
              state = CORE0_TUNNER_ERROR;
            }
          }

          break;
        }

      // --------------------------------------------------
      case CORE0_TUNNER_ZERO_ANGLE_ESTIMATE:
        {
          if (zeroAngleEstimateReadMCU) {
            if (WiFi.status() == WL_CONNECTED) {
              char url[128];
              char macStr[32];

              sprintf(macStr, "%02X:%02X:%02X:%02X:%02X:%02X",
                      mac[0], mac[1], mac[2],
                      mac[3], mac[4], mac[5]);

              sprintf(url,
                      "http://vehicle.aproposdrive.com/api/parameters/%s",
                      macStr);

              HTTPClient http;
              http.begin(url);
              http.addHeader("Content-Type", "application/json");

              // ✅ Build JSON (only 4 params)
              StaticJsonDocument<256> doc;

              doc["T8"] = tuner_params[8];
              doc["T9"] = tuner_params[9];
              doc["T10"] = tuner_params[10];
              doc["T11"] = tuner_params[11];

              String body;
              serializeJson(doc, body);

              Serial.println("ZERO ANGLE POST:");
              Serial.println(body);

              int httpCode = http.POST(body);

              if (httpCode > 0) {
                String response = http.getString();
                Serial.println("Zero Angle POST Success:");
                Serial.println(response);
              } else {
                Serial.println("Zero Angle POST Failed");
              }

              http.end();
            } else {
              Serial.println("WiFi not connected");
            }

            // ✅ Reset flags after success
            zeroAngleEstimateReadMCU = false;
            zeroAngleEstimate = 0;

            state = CORE0_TUNNER_DONE;
          }

          break;
        }

      // --------------------------------------------------
      case CORE0_TUNNER_DONE:
        {
          vTaskDelay(pdMS_TO_TICKS(2000));
          state = CORE0_TUNNER_CHECK_STATUS;
          break;
        }

      // --------------------------------------------------
      case CORE0_TUNNER_ERROR:
        {
          // Serial.println("CORE0: ERROR");

          vTaskDelay(pdMS_TO_TICKS(3000));
          state = CORE0_TUNNER_CHECK_STATUS;
          break;
        }
    }

    vTaskDelay(pdMS_TO_TICKS(10));
  }
}


void CORE1_TUNNER_HANDLER(void) {
  static uint32_t ackStartTime = 0;
  static CORE1_TUNNER_STATE state = CORE1_TUNNER_CHECK_STATUS;
  CanFrame rx;

  switch (state) {
    // --------------------------------------------------
    case CORE1_TUNNER_CHECK_STATUS:
      {
        if (tuner_read == 1) {
          state = CORE1_TUNNER_READ;
        } else if (tuner_write == 1) {
          state = CORE1_TUNNER_WRITE;
        } else if (zeroAngleEstimate == 1) {
          state = CORE1_TUNNER_ZERO_ANGLE_ESTIMATE;
        }
        break;
      }

    // --------------------------------------------------
    case CORE1_TUNNER_READ:
      {
        uint8_t dummy = 0;

        // Send request
        CAN_WriteFrame(0xE0, &dummy, 0);

        // Reset counters
        received_count = 0;
        memset(received_flags, 0, sizeof(received_flags));

        // Move to RX state
        state = CORE1_TUNNER_CAN_RX_READ;
        break;
      }

    // --------------------------------------------------
    case CORE1_TUNNER_CAN_RX_READ:
      {
        if (ESP32Can.readFrame(rx, 0)) {
          if (rx.identifier == 0xE1 && rx.data_length_code == 8) {
            uint32_t index =
              (rx.data[0] << 24) | (rx.data[1] << 16) | (rx.data[2] << 8) | (rx.data[3]);

            if (index < 44 && !received_flags[index]) {
              float value;
              memcpy(&value, &rx.data[4], sizeof(float));

              tuner_params[index] = value;
              received_flags[index] = true;
              received_count++;
            }
          }
        }

        if (received_count >= 44) {
          Serial.println("All params received");
          readSuccessMCU = true;
          tuner_read = 0;
          state = CORE1_TUNNER_CHECK_STATUS;
        }
      }
      break;

    // --------------------------------------------------
    case CORE1_TUNNER_WRITE:
      {
        if (readSuccessServer) {
          Serial.println("Sending params to MCU...");

          // ✅ STEP 1: FLUSH OLD CAN FRAMES (VERY IMPORTANT)
          CanFrame dummy;
          while (ESP32Can.readFrame(dummy, 0)) {
            // discard all old frames (E1, E3, anything)
          }

          // ✅ STEP 2: Reset flags
          writeSuccessMCU = false;
          mcuOffline = false;

          // ---- Send all 44 params ----
          for (uint32_t i = 0; i < 44; i++) {
            uint8_t txData[8];

            txData[0] = (i >> 24) & 0xFF;
            txData[1] = (i >> 16) & 0xFF;
            txData[2] = (i >> 8) & 0xFF;
            txData[3] = (i >> 0) & 0xFF;

            memcpy(&txData[4], &tuner_params[i], sizeof(float));

            CAN_WriteFrame(0xE2, txData, 8);

            delayMicroseconds(500);
          }

          // ✅ STEP 3: Reset server flag
          readSuccessServer = false;

          // ✅ STEP 4: Start ACK wait
          ackStartTime = millis();
          state = CORE1_TUNNER_CAN_RX_WRITE;  // (later you should split states)
        } else {
          state = CORE1_TUNNER_CHECK_STATUS;
        }

        break;
      }

    // --------------------------------------------------
    case CORE1_TUNNER_CAN_RX_WRITE:
      {
        if (ESP32Can.readFrame(rx, 0)) {
          if (rx.identifier == 0xE3) {
            Serial.println("MCU ACK received");

            writeSuccessMCU = true;
            tuner_write = 0;

            state = CORE1_TUNNER_CHECK_STATUS;
          }
        }

        // timeout
        if (millis() - ackStartTime > 2000) {
          Serial.println("MCU not responding");

          mcuOffline = true;
          tuner_write = 0;

          state = CORE1_TUNNER_CHECK_STATUS;
        }
      }
      break;

    // --------------------------------------------------
    case CORE1_TUNNER_ZERO_ANGLE_ESTIMATE:
      {
        Serial.println("Requesting Zero Angle Estimate...");

        // ✅ Flush old CAN frames (VERY IMPORTANT)
        CanFrame flush;
        while (ESP32Can.readFrame(flush, 0))
          ;

        // ✅ Send command 0xE4
        uint8_t dummy = 0;
        CAN_WriteFrame(0xE4, &dummy, 0);

        // Reset flag
        zeroAngleEstimateReadMCU = false;

        // Move to RX state
        state = CORE1_TUNNER_CAN_RX_ZERO_ANGLE_ESTIMATE;

        break;
      }

    // --------------------------------------------------
    case CORE1_TUNNER_CAN_RX_ZERO_ANGLE_ESTIMATE:
      {
        static uint8_t receivedCount = 0;
        static bool receivedFlags[4] = { 0 };  // for 0x08–0x0B

        CanFrame rx;

        if (ESP32Can.readFrame(rx, 0)) {
          if (rx.identifier == 0xE5 && rx.data_length_code == 8) {
            uint32_t index =
              (rx.data[0] << 24) | (rx.data[1] << 16) | (rx.data[2] << 8) | (rx.data[3]);

            float value;
            memcpy(&value, &rx.data[4], sizeof(float));

            // ✅ Only accept 4 params
            if (index >= 0x08 && index <= 0x0B) {
              uint8_t localIndex = index - 0x08;

              if (!receivedFlags[localIndex]) {
                tuner_params[index] = value;
                receivedFlags[localIndex] = true;
                receivedCount++;

                Serial.printf("ZeroParam[%lu] = %f\n", index, value);
              }
            }
          }
        }

        // ✅ Completion check (4 params)
        if (receivedCount >= 4) {
          Serial.println("Zero Angle Estimate params received");

          zeroAngleEstimateReadMCU = true;
          zeroAngleEstimate = 0;

          // Reset for next cycle
          receivedCount = 0;
          memset(receivedFlags, 0, sizeof(receivedFlags));

          state = CORE1_TUNNER_CHECK_STATUS;
        }

        break;
      }

    // --------------------------------------------------
    default:
      {
        state = CORE1_TUNNER_CHECK_STATUS;
        break;
      }
  }
}