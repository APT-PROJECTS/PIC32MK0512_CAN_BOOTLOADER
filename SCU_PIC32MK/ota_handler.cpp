#include "ota_handler.h"
#include "can_handler.h"
#include "wifi_mqtt_handler.h"
#include "mac_handler.h"
#include "mcp23017_handler.h"

// ----------------SOFTWARE ID----------------------
volatile uint16_t ota_server_major = 0;
volatile uint16_t ota_server_minor = 0;

uint16_t mcu_major_previous = 0;
uint16_t mcu_minor_previous = 0;

uint16_t mcu_major_latest = 0;
uint16_t mcu_minor_latest = 0;

volatile uint16_t OEM_ID = 0;
volatile uint16_t APPLICATION_ID = 0;

// -----------------PIC ID----------------------
uint8_t PIC_ID_RAW[16];
uint8_t PIC_ID[16];
static uint8_t picid_rx_count = 0;

// -----------------FLAGS-----------------------
static bool otaServerVersionUpdated = false;
static bool downloadBinFile = false;
static bool isBinFileDownloaded = false;
static bool mcuUpdateStarted = false;
static bool otaValidRx = false;
static bool sendReport = false;

// ---------------- OTA DATA TRANSFER ----------------
char fileURL[128];
const char *filePath = "/fw.bin";

static File fwFile;
static size_t fwSize = 0;
static size_t totalSent = 0;

static uint8_t chunkIndex = 1U;
static size_t bytesSentInChunk = 0U;
static bool waitingChunkAck = false;
static uint32_t firmwareCrc32 = 0xFFFFFFFFU;

static uint32_t otaRxStartTime = 0;

size_t totalFWSize = 0;
bool fileOpened = false;

// -----------------INITIAL STATES----------------------
static OTA_CORE0_STATE_t otaCore0State = OTA_CORE0_IDLE;
static OTA_CORE1_State_t otaCore1State = OTA_CORE1_IDLE;
static OTA_CORE1_State_t otaPrevTxState = OTA_CORE1_IDLE;
static OTA_REPORT_t otaReportStatus = ALREADY_UPDATED;

/* CRC-32/ISO-HDLC: reflected poly, init/final XOR 0xFFFFFFFF. */
static uint32_t OTA_CRC32_Update(uint32_t crc, const uint8_t *data, size_t length) {
  while (length-- > 0U) {
    crc ^= *data++;
    for (uint8_t bit = 0U; bit < 8U; bit++) {
      crc = (crc & 1U) ? ((crc >> 1U) ^ 0xEDB88320U) : (crc >> 1U);
    }
  }
  return crc;
}


static void macToString(const uint8_t *mac, char *out) {
  sprintf(out,
          "%02X%02X%02X%02X%02X%02X",
          mac[0], mac[1], mac[2],
          mac[3], mac[4], mac[5]);
}

void OTA_Handler_CORE0(const uint8_t *mac) {
  switch (otaCore0State) {

    // =================================================
    case OTA_CORE0_IDLE:
      if (!mcuUpdateStarted) {
        otaCore0State = OTA_CORE0_CHECK_SERVER_VERSION;
      }
      break;

    // =================================================
    case OTA_CORE0_CHECK_SERVER_VERSION:
      {
        if (WiFi.status() != WL_CONNECTED) {
          Serial.println("OTA CORE0: WiFi not connected");
          break;
        }

        char macStr[13];
        char url[128];

        macToString(mac, macStr);

        snprintf(url, sizeof(url),
                 "%s/%s",
                 OTA_VERSION_URL, macStr);

        HTTPClient http;
        http.begin(url);

        int code = http.GET();
        if (code != HTTP_CODE_OK) {
          Serial.printf("OTA CORE0: Version HTTP error %d\n", code);
          http.end();
          break;
        }

        DynamicJsonDocument doc(128);
        if (deserializeJson(doc, http.getString())) {
          Serial.println("OTA CORE0: Version JSON parse failed");
          http.end();
          break;
        }

        const char *ver = doc["version"];
        if (!ver) {
          Serial.println("OTA CORE0: Version field missing");
          http.end();
          break;
        }

        // -------- Parse version string --------
        uint16_t oem = 0;
        uint16_t app = 0;
        uint16_t major = 0;
        uint16_t minor = 0;

        sscanf(ver, "%hu.%hu.%hu.%hu", &oem, &app, &major, &minor);

        // -----------SOFTWARE ID---------------
        // -----------PROJECT ID----------------
        OEM_ID = oem;
        APPLICATION_ID = app;

        // -----------FIRMWARE ID---------------
        ota_server_major = major;
        ota_server_minor = minor;

        Serial.printf(
          "OTA CORE0: Server OEM:%u APP:%u VER:%u.%u\n",
          OEM_ID,
          APPLICATION_ID,
          ota_server_major,
          ota_server_minor);

        http.end();

        if (
          (ota_server_major > mcu_major_previous) || (ota_server_major == mcu_major_previous && ota_server_minor > mcu_minor_previous)) {
          otaServerVersionUpdated = true;

          Serial.println("OTA CORE0: OTA REQUIRED → downloading firmware");
          otaCore0State = OTA_CORE0_DOWNLOAD_BIN_FILE;
        } else {
          otaServerVersionUpdated = false;

          Serial.println("OTA CORE0: OTA NOT required");
          otaCore0State = OTA_CORE0_DONE;
        }

        break;
      }

    // =================================================
    case OTA_CORE0_DOWNLOAD_BIN_FILE:
      {
        if (!downloadBinFile) {
          // Serial.println("OTA CORE0: Download flag not set, skipping");
          // otaCore0State = OTA_CORE0_DONE;
          break;
        }

        SPIFFS.begin(true);

        char macStr[13];
        macToString(mac, macStr);

        snprintf(fileURL, sizeof(fileURL),
                 "%s/%s",
                 OTA_FIRMWARE_URL, macStr);

        Serial.printf("OTA CORE0: Downloading firmware:\n%s\n", fileURL);

        WiFiClientSecure client;
        client.setInsecure();  // Accept HTTPS certs

        HTTPClient http;
        if (!http.begin(client, fileURL)) {
          Serial.println("OTA CORE0: HTTPS begin failed");
          // otaCore0State = OTA_CORE0_ERROR;
          break;
        }

        int httpCode = http.GET();
        if (httpCode != HTTP_CODE_OK) {
          Serial.printf("OTA CORE0: HTTPS GET failed (%d)\n", httpCode);
          http.end();
          // otaCore0State = OTA_CORE0_ERROR;
          break;
        }

        int len = http.getSize();
        WiFiClient *stream = http.getStreamPtr();

        Serial.printf("OTA CORE0: Firmware size = %d bytes\n", len);

        SPIFFS.remove(filePath);
        File fw = SPIFFS.open(filePath, FILE_WRITE);
        if (!fw) {
          Serial.println("OTA CORE0: Failed to open file");
          http.end();
          // otaCore0State = OTA_CORE0_ERROR;
          break;
        }

        uint8_t buffer[256];
        size_t downloadedBytes = 0;

        while (http.connected() && (len > 0 || len == -1)) {
          size_t available = stream->available();
          if (available) {
            int toRead = min((int)available, (int)sizeof(buffer));
            int readBytes = stream->readBytes(buffer, toRead);
            if ((readBytes <= 0) || (fw.write(buffer, readBytes) != (size_t)readBytes)) {
              Serial.println("OTA CORE0: Firmware file write failed");
              break;
            }
            downloadedBytes += (size_t)readBytes;
            if (len > 0) len -= readBytes;
          }
          delay(1);  // yield to RTOS
        }

        fw.close();
        http.end();

        if ((downloadedBytes == 0U) || (len > 0)) {
          Serial.println("OTA CORE0: Firmware download incomplete");
          SPIFFS.remove(filePath);
          break;
        }

        Serial.printf("OTA CORE0: Firmware saved to SPIFFS (%u bytes)\n", downloadedBytes);
        totalFWSize = downloadedBytes;
        isBinFileDownloaded = true;

        otaCore0State = OTA_CORE0_REPORT;
        break;
      }

    case OTA_CORE0_REPORT:
      {
        if (!sendReport) {
          break;
        }

        Serial.println("OTA CORE0: Sending OTA report");

        if (WiFi.status() != WL_CONNECTED) {
          Serial.println("OTA CORE0: WiFi not connected");
          break;
        }

        WiFiClientSecure client;
        client.setInsecure();

        HTTPClient http;

        if (!http.begin(client, OTA_REPORT_URL)) {
          Serial.println("OTA CORE0: HTTP begin failed");
          break;
        }

        http.addHeader("Content-Type", "application/json");

        DynamicJsonDocument doc(256);

        // ---------------- PIC ID (NO REVERSE) ----------------
        char picidString[33] = { 0 };
        for (int i = 0; i < 16; i++)
          sprintf(&picidString[i * 2], "%02X", PIC_ID[i]);

        doc["pic_id"] = picidString;

        // ---------------- DEVICE ID (NO REVERSE) ----------------
        char macStr[13] = { 0 };
        for (int i = 0; i < 6; i++)
          sprintf(&macStr[i * 2], "%02X", mac[i]);

        doc["deviceId"] = macStr;

        // ---------------- STATUS ----------------
        char statusStr[4];
        sprintf(statusStr, "%d", otaReportStatus);
        doc["status"] = statusStr;

        // ---------------- VERSION ----------------
        char prevVer[8], newVer[8];
        sprintf(prevVer, "%d.%d", mcu_major_previous, mcu_minor_previous);
        sprintf(newVer, "%d.%d", mcu_major_latest, mcu_minor_latest);

        doc["previousVersion"] = prevVer;
        doc["updatedVersion"] = newVer;

        // ---------------- REPROGRAMMING FLAG ----------------
        uint8_t reprogrammingFlag =
          (otaReportStatus == TWO_STEP_SUCCESS) ? 1 : 0;

        doc["reprogramming"] = reprogrammingFlag;

        // ---------------- SEND ----------------
        String body;
        serializeJson(doc, body);

        Serial.println("OTA REPORT JSON:");
        Serial.println(body);

        int httpCode = http.POST(body);
        Serial.printf("OTA REPORT HTTP Code: %d\n", httpCode);

        http.end();

        otaCore0State = OTA_CORE0_DONE;
        break;
      }

    // =================================================
    case OTA_CORE0_DONE:
      // otaCore0State = OTA_CORE0_IDLE;
      break;

    // =================================================
    case OTA_CORE0_ERROR:
      // Serial.println("OTA CORE0: ERROR state");
      break;
  }
}

bool OTA_Handler_CORE1(void) {
  static CanFrame rx;

  switch (otaCore1State) {

    // ==================================================
    case OTA_CORE1_IDLE:
      {
        if (otaServerVersionUpdated) {
          LED_Green_On();
          otaCore1State = OTA_CORE1_READ_PIC_ID_1;
        }
      }
      break;

    // ==================================================
    case OTA_CORE1_READ_PIC_ID_1:
      {
        uint8_t txBuf[8] = { 0 };

        memset(PIC_ID_RAW, 0, sizeof(PIC_ID_RAW));

        txBuf[0] = OTA_CAN_CMD_0_PIC_ID;
        txBuf[1] = OTA_CAN_CMD_1_PIC_ID_1;
        txBuf[2] = OTA_CAN_CMD_2_READ;

        if (!CAN_WriteFrame(OTA_CAN_ID_HOST, txBuf, sizeof(txBuf))) {
          Serial.println("OTA CORE1: PICID_1 REQUEST SEND FAIL");
          break;
        }

        Serial.println("OTA CORE1: PICID_1 REQUEST SEND SUCCESS");

        otaPrevTxState = otaCore1State;
        otaRxStartTime = millis();
        otaCore1State = OTA_CORE1_CAN_RX_HANDLE;
      }
      break;

    // ==================================================
    case OTA_CORE1_READ_PIC_ID_2:
      {
        uint8_t txBuf[8] = { 0 };

        txBuf[0] = OTA_CAN_CMD_0_PIC_ID;
        txBuf[1] = OTA_CAN_CMD_1_PIC_ID_2;
        txBuf[2] = OTA_CAN_CMD_2_READ;

        if (!CAN_WriteFrame(OTA_CAN_ID_HOST, txBuf, sizeof(txBuf))) {
          Serial.println("OTA CORE1: PICID_2 REQUEST SEND FAIL");
          break;
        }

        Serial.println("OTA CORE1: PICID_2 REQUEST SEND SUCCESS");

        otaPrevTxState = otaCore1State;
        otaRxStartTime = millis();
        otaCore1State = OTA_CORE1_CAN_RX_HANDLE;
      }
      break;

    // ==================================================
    case OTA_CORE1_READ_PIC_ID_3:
      {
        uint8_t txBuf[8] = { 0 };

        txBuf[0] = OTA_CAN_CMD_0_PIC_ID;
        txBuf[1] = OTA_CAN_CMD_1_PIC_ID_3;
        txBuf[2] = OTA_CAN_CMD_2_READ;

        if (!CAN_WriteFrame(OTA_CAN_ID_HOST, txBuf, sizeof(txBuf))) {
          Serial.println("OTA CORE1: PICID_3 REQUEST SEND FAIL");
          break;
        }

        Serial.println("OTA CORE1: PICID_3 REQUEST SEND SUCCESS");

        otaPrevTxState = otaCore1State;
        otaRxStartTime = millis();
        otaCore1State = OTA_CORE1_CAN_RX_HANDLE;
      }
      break;

    // ==================================================
    case OTA_CORE1_READ_PIC_ID_4:
      {
        uint8_t txBuf[8] = { 0 };

        txBuf[0] = OTA_CAN_CMD_0_PIC_ID;
        txBuf[1] = OTA_CAN_CMD_1_PIC_ID_4;
        txBuf[2] = OTA_CAN_CMD_2_READ;

        if (!CAN_WriteFrame(OTA_CAN_ID_HOST, txBuf, sizeof(txBuf))) {
          Serial.println("OTA CORE1: PICID_4 REQUEST SEND FAIL");
          break;
        }

        Serial.println("OTA CORE1: PICID_4 REQUEST SEND SUCCESS");

        otaPrevTxState = otaCore1State;
        otaRxStartTime = millis();
        otaCore1State = OTA_CORE1_CAN_RX_HANDLE;
      }
      break;

    // ==================================================
    case OTA_CORE1_READ_PROJECT_ID:
      {
        uint8_t txBuf[8] = { 0 };

        txBuf[0] = OTA_CAN_CMD_0_SOFTWARE_ID;
        txBuf[1] = OTA_CAN_CMD_1_PROJECT_ID;
        txBuf[2] = OTA_CAN_CMD_2_READ;

        if (!CAN_WriteFrame(OTA_CAN_ID_HOST, txBuf, 8)) {
          Serial.println("OTA CORE1: PROJECT_ID REQUEST SEND FAIL");
          // otaCore1State = OTA_CORE1_ERROR;
          break;
        }

        Serial.println("OTA CORE1: PROJECT_ID REQUEST SEND SUCCESS");

        otaPrevTxState = otaCore1State;
        otaRxStartTime = millis();
        otaCore1State = OTA_CORE1_CAN_RX_HANDLE;
      }
      break;

    // ==================================================
    case OTA_CORE1_READ_FIRMWARE_ID:
      {
        uint8_t txBuf[8] = { 0 };

        txBuf[0] = OTA_CAN_CMD_0_SOFTWARE_ID;
        txBuf[1] = OTA_CAN_CMD_1_FIRMWARE_ID;
        txBuf[2] = OTA_CAN_CMD_2_READ;

        if (!CAN_WriteFrame(OTA_CAN_ID_HOST, txBuf, 8)) {
          Serial.println("OTA CORE1: FIRMWARE_ID REQUEST SEND FAIL");
          // otaCore1State = OTA_CORE1_ERROR;
          break;
        }

        Serial.println("OTA CORE1: FIRMWARE_ID REQUEST SEND SUCCESS");

        otaPrevTxState = otaCore1State;
        otaRxStartTime = millis();
        otaCore1State = OTA_CORE1_CAN_RX_HANDLE;
      }
      break;

    // ==================================================
    case OTA_CORE1_COMPARE_APP_SIZE:
      {
        uint8_t txBuf[8] = { 0 };

        if (!isBinFileDownloaded) {
          // Firmware not ready yet → wait here
          break;
        }

        txBuf[0] = OTA_CAN_CMD_0_APP_SIZE;
        txBuf[2] = OTA_CAN_CMD_2_COMPARE;

        /* Send firmware size in bytes */
        uint32_t appSize = (uint32_t)totalFWSize;

        txBuf[4] = (uint8_t)(appSize >> 24);
        txBuf[5] = (uint8_t)(appSize >> 16);
        txBuf[6] = (uint8_t)(appSize >> 8);
        txBuf[7] = (uint8_t)(appSize);

        if (!CAN_WriteFrame(OTA_CAN_ID_HOST, txBuf, sizeof(txBuf))) {
          Serial.println("OTA CORE1: APP SIZE SEND FAIL");
          break;
        }

        Serial.printf("OTA CORE1: APP Size = %lu bytes\n", appSize);

        otaPrevTxState = otaCore1State;
        otaRxStartTime = millis();
        otaCore1State = OTA_CORE1_CAN_RX_HANDLE;
      }
      break;

      // ==================================================
    case OTA_CORE1_BOOT_JUMP:
      {
        uint8_t txBuf[8] = { 0 };

        mcuUpdateStarted = true;
        txBuf[0] = OTA_CAN_CMD_0_JUMP;
        txBuf[1] = OTA_CAN_CMD_1_BOOT_JUMP;


        if (!CAN_WriteFrame(OTA_CAN_ID_HOST, txBuf, 8)) {
          Serial.println("OTA CORE1: BOOT JUMP REQUEST SEND FAIL");
          // otaCore1State = OTA_CORE1_ERROR;
          break;
        }

        Serial.println("OTA CORE1: BOOT JUMP REQUEST SEND SUCCESS");

        otaPrevTxState = otaCore1State;
        otaRxStartTime = millis();
        otaCore1State = OTA_CORE1_CAN_RX_HANDLE;
      }
      break;

      // ==================================================
    case OTA_CORE1_APP_ERASE:
      {
        uint8_t txBuf[8] = { 0 };
        txBuf[0] = OTA_CAN_CMD_0_APP_ERASE;

        if (!CAN_WriteFrame(OTA_CAN_ID_HOST, txBuf, 8)) {
          Serial.println("OTA CORE1: APP ERASE REQUEST SEND FAIL");
          // otaCore1State = OTA_CORE1_ERROR;
          break;
        }

        Serial.println("OTA CORE1: APP ERASE REQUEST SEND SUCCESS");

        otaPrevTxState = otaCore1State;
        otaRxStartTime = millis();
        otaCore1State = OTA_CORE1_CAN_RX_HANDLE;
      }
      break;

      // ==================================================
    case OTA_CORE1_WRITE_APP_CHUNK:
      {
        uint8_t txBuf[8] = { 0U };

        if (!fileOpened) {
          fwFile = SPIFFS.open(filePath, FILE_READ);
          if (!fwFile || fwFile.size() == 0) {
            Serial.println("OTA CORE1: Firmware file open failed");
            break;
          }

          totalFWSize = fwFile.size();
          totalSent = 0;
          chunkIndex = 1U;
          bytesSentInChunk = 0U;
          waitingChunkAck = false;
          firmwareCrc32 = 0xFFFFFFFFU;
          fileOpened = true;

          Serial.printf("OTA CORE1: Firmware size = %u bytes\n", totalFWSize);
        }

        if (waitingChunkAck) {
          break;
        }

        if ((bytesSentInChunk == 0U) && (totalSent >= totalFWSize)) {
          fwFile.close();
          Serial.println("OTA CORE1: All application chunks acknowledged");
          otaCore1State = OTA_CORE1_READ_APP_CRC;
          break;
        }

        uint8_t payload[OTA_FRAME_DATA] = { 0xFF, 0xFF, 0xFF, 0xFF };
        size_t fileBytesRemaining = totalFWSize - totalSent;
        size_t bytesToRead = min((size_t)OTA_FRAME_DATA, fileBytesRemaining);

        if (bytesToRead > 0U) {
          if (fwFile.read(payload, bytesToRead) != (int)bytesToRead) {
            Serial.println("OTA CORE1: SPIFFS read failed");
            fwFile.close();
            sendReport = true;
            otaReportStatus = APP_CHUNK_WRITE_FAIL;
            otaCore1State = OTA_CORE1_ERROR;
            otaCore0State = OTA_CORE0_REPORT;
            break;
          }
          firmwareCrc32 = OTA_CRC32_Update(firmwareCrc32, payload, bytesToRead);
          totalSent += bytesToRead;
        }

        txBuf[0] = OTA_CAN_CMD_0_APP_CHUNK;
        txBuf[1] = chunkIndex;
        txBuf[2] = OTA_CAN_CMD_2_WRITE;
        memcpy(&txBuf[4], payload, OTA_FRAME_DATA);

        if (!CAN_WriteFrame(OTA_CAN_ID_HOST, txBuf, sizeof(txBuf))) {
          Serial.println("OTA CORE1: CAN send failed (APP CHUNK)");
          break;
        }

        bytesSentInChunk += OTA_FRAME_DATA;

        /* Leave time for the MCP2515/PIC receiver to drain each frame. */
        delay(1);

        if (bytesSentInChunk == OTA_APP_CHUNK_SIZE) {
          waitingChunkAck = true;
          otaPrevTxState = otaCore1State;
          otaRxStartTime = millis();
          otaCore1State = OTA_CORE1_CAN_RX_HANDLE;
        }

        break;
      }

      // ==================================================
    case OTA_CORE1_READ_APP_CRC:
      {
        uint8_t txBuf[8] = { 0U };

        txBuf[0] = OTA_CAN_CMD_0_APP_CRC;
        txBuf[2] = OTA_CAN_CMD_2_READ;

        if (!CAN_WriteFrame(OTA_CAN_ID_HOST, txBuf, sizeof(txBuf))) {
          Serial.println("OTA CORE1: APP CRC request send failed");
          break;
        }

        Serial.println("OTA CORE1: APP CRC read request sent");
        otaPrevTxState = otaCore1State;
        otaRxStartTime = millis();
        otaCore1State = OTA_CORE1_CAN_RX_HANDLE;
      }
      break;

      // ==================================================
    case OTA_CORE1_WRITE_PROJECT_ID:
      {
        uint8_t txBuf[8] = { 0U };

        float projectId = (float)OEM_ID + ((float)APPLICATION_ID / 10.0f);

        txBuf[0] = OTA_CAN_CMD_0_SOFTWARE_ID;
        txBuf[1] = OTA_CAN_CMD_1_PROJECT_ID;
        txBuf[2] = OTA_CAN_CMD_2_WRITE;

        /* Copy IEEE-754 float (little-endian on ESP32) */
        memcpy(&txBuf[4], &projectId, sizeof(float));

        if (!CAN_WriteFrame(OTA_CAN_ID_HOST, txBuf, sizeof(txBuf))) {
          Serial.println("OTA CORE1: PROJECT_ID SEND FAIL");
          break;
        }

        Serial.printf("OTA CORE1: PROJECT_ID %.1f SEND SUCCESS\n", projectId);

        otaPrevTxState = otaCore1State;
        otaRxStartTime = millis();
        otaCore1State = OTA_CORE1_CAN_RX_HANDLE;
      }
      break;

      // ==================================================
    case OTA_CORE1_WRITE_FIRMWARE_ID:
      {
        uint8_t txBuf[8] = { 0U };

        float firmwareId = (float)ota_server_major + ((float)ota_server_minor / 10.0f);

        txBuf[0] = OTA_CAN_CMD_0_SOFTWARE_ID;
        txBuf[1] = OTA_CAN_CMD_1_FIRMWARE_ID;
        txBuf[2] = OTA_CAN_CMD_2_WRITE;

        /* Copy IEEE-754 float (little-endian on ESP32) */
        memcpy(&txBuf[4], &firmwareId, sizeof(float));

        if (!CAN_WriteFrame(OTA_CAN_ID_HOST, txBuf, sizeof(txBuf))) {
          Serial.println("OTA CORE1: FIRMWARE_ID SEND FAIL");
          break;
        }

        Serial.printf("OTA CORE1: FIRMWARE_ID %.1f SEND SUCCESS\n", firmwareId);

        otaPrevTxState = otaCore1State;
        otaRxStartTime = millis();
        otaCore1State = OTA_CORE1_CAN_RX_HANDLE;
      }
      break;

      // ==================================================
    case OTA_CORE1_APP_JUMP:
      {
        uint8_t txBuf[8] = { 0 };
        txBuf[0] = OTA_CAN_CMD_0_JUMP;
        txBuf[1] = OTA_CAN_CMD_1_APP_JUMP;

        if (!CAN_WriteFrame(OTA_CAN_ID_HOST, txBuf, 8)) {
          Serial.println("OTA CORE1: APP JUMP request send failed");
          // otaCore1State = OTA_CORE1_ERROR;
          break;
        }

        Serial.println("OTA CORE1: APP JUMP command sent");

        otaPrevTxState = otaCore1State;
        otaRxStartTime = millis();
        otaCore1State = OTA_CORE1_CAN_RX_HANDLE;
        break;
      }

      // ==================================================
    case OTA_CORE1_CAN_RX_HANDLE:
      {
        static uint8_t rxRetryCount = 0;

        if (CAN_ReadFrame(&rx, CAN_RX_TIMEOUT)) {

          otaValidRx = false;

          if (rx.rtr) {
            break;
          }

          switch (rx.identifier) {

            case OTA_CAN_ID_CLIENT:
              {
                otaValidRx = true;

                switch (rx.data[0]) {
                  case OTA_CAN_CMD_0_PIC_ID:
                    {
                      switch (rx.data[2]) {
                        case OTA_CAN_CMD_2_READ:
                          {
                            switch (rx.data[3]) {
                              /*---------------------- SUCCESS ----------------------*/
                              case OTA_CAN_CMD_3_SUCCESS:

                                switch (rx.data[1]) {
                                  case OTA_CAN_CMD_1_PIC_ID_1:
                                    {
                                      memcpy(&PIC_ID_RAW[0], &rx.data[4], 4);
                                      otaCore1State = OTA_CORE1_READ_PIC_ID_2;
                                    }
                                    break;

                                  case OTA_CAN_CMD_1_PIC_ID_2:
                                    {
                                      memcpy(&PIC_ID_RAW[4], &rx.data[4], 4);
                                      otaCore1State = OTA_CORE1_READ_PIC_ID_3;
                                    }
                                    break;

                                  case OTA_CAN_CMD_1_PIC_ID_3:
                                    {
                                      memcpy(&PIC_ID_RAW[8], &rx.data[4], 4);
                                      otaCore1State = OTA_CORE1_READ_PIC_ID_4;
                                    }
                                    break;

                                  case OTA_CAN_CMD_1_PIC_ID_4:
                                    {
                                      memcpy(&PIC_ID_RAW[12], &rx.data[4], 4);
                                      memcpy(PIC_ID, PIC_ID_RAW, 16);

                                      Serial.print("OTA CORE1: PIC ID = ");
                                      for (int i = 0; i < 16; i++) {
                                        Serial.printf("%02X", PIC_ID[i]);
                                      }
                                      Serial.println();

                                      otaCore1State = OTA_CORE1_READ_PROJECT_ID;
                                    }
                                    break;

                                  default:
                                    break;
                                }
                                break;

                              /*------------------------ FAIL -----------------------*/
                              case OTA_CAN_CMD_3_FAIL:

                                Serial.println("OTA CORE1: PIC ID READ FAILED");

                                sendReport = true;
                                otaReportStatus = PIC_ID_READ_FAIL;  // Define this report status

                                otaCore1State = OTA_CORE1_ERROR;
                                otaCore0State = OTA_CORE0_REPORT;
                                break;

                              default:
                                break;
                            }
                          }
                          break;

                        default:
                          break;
                      }
                    }
                    break;

                  case OTA_CAN_CMD_0_SOFTWARE_ID:
                    {
                      switch (rx.data[2]) {
                        case OTA_CAN_CMD_2_READ:
                          {
                            switch (rx.data[3]) {
                              /*---------------------- SUCCESS ----------------------*/
                              case OTA_CAN_CMD_3_SUCCESS:
                                {
                                  float softwareId;

                                  /* Reconstruct IEEE-754 float from bytes 4-7 */
                                  memcpy(&softwareId, &rx.data[4], sizeof(float));

                                  switch (rx.data[1]) {
                                    case OTA_CAN_CMD_1_PROJECT_ID:
                                      {
                                        uint16_t mcu_oem_id = (uint16_t)softwareId;
                                        uint16_t mcu_application_id =
                                          (uint16_t)((softwareId - (float)mcu_oem_id) * 10.0f + 0.5f);

                                        /* -------- NEW MCU CHECK -------- */
                                        bool newMCU = (mcu_oem_id == 0x0000 && mcu_application_id == 0x0000);

                                        /* -------- OEM ID CHECK -------- */
                                        if (!newMCU && (mcu_oem_id != OEM_ID)) {
                                          Serial.println("OTA CORE1: OEM ID mismatch");

                                          sendReport = true;
                                          otaReportStatus = OEM_ID_MISMATCH;

                                          otaCore1State = OTA_CORE1_ERROR;
                                          otaCore0State = OTA_CORE0_REPORT;
                                          break;
                                        }

                                        /* -------- APPLICATION ID CHECK -------- */
                                        if (!newMCU && (mcu_application_id != APPLICATION_ID)) {
                                          Serial.println("OTA CORE1: APPLICATION ID mismatch");

                                          sendReport = true;
                                          otaReportStatus = APP_ID_MISMATCH;

                                          otaCore1State = OTA_CORE1_ERROR;
                                          otaCore0State = OTA_CORE0_REPORT;
                                          break;
                                        }

                                        otaCore1State = OTA_CORE1_READ_FIRMWARE_ID;
                                      }
                                      break;

                                    case OTA_CAN_CMD_1_FIRMWARE_ID:
                                      {
                                        mcu_major_previous = (uint16_t)softwareId;
                                        mcu_minor_previous =
                                          (uint16_t)((softwareId - (float)mcu_major_previous) * 10.0f + 0.5f);

                                        Serial.printf(
                                          "OTA CORE1: MCU %u.%u | Server %u.%u\n",
                                          mcu_major_previous,
                                          mcu_minor_previous,
                                          ota_server_major,
                                          ota_server_minor);

                                        /* -------- VERSION CHECK -------- */
                                        if ((mcu_major_previous > ota_server_major) || ((mcu_major_previous == ota_server_major) && (mcu_minor_previous >= ota_server_minor))) {
                                          Serial.println("OTA CORE1: OTA NOT required");

                                          sendReport = true;

                                          mcu_major_latest = ota_server_major;
                                          mcu_minor_latest = ota_server_minor;

                                          otaReportStatus = ALREADY_UPDATED;

                                          otaCore1State = OTA_CORE1_DONE;
                                          otaCore0State = OTA_CORE0_REPORT;

                                          return false;
                                        }

                                        downloadBinFile = true;
                                        Serial.println("OTA CORE1: OTA REQUIRED");

                                        /* Boot jump can reset the PIC, so compare size afterwards. */
                                        otaCore1State = OTA_CORE1_BOOT_JUMP;
                                      }
                                      break;

                                    default:
                                      break;
                                  }
                                }
                                break;

                              /*------------------------ FAIL -----------------------*/
                              case OTA_CAN_CMD_3_FAIL:
                                {
                                  Serial.println("OTA CORE1: Software ID READ FAILED");

                                  sendReport = true;
                                  otaReportStatus = SOFTWARE_ID_READ_FAIL;  // Define this report status

                                  otaCore1State = OTA_CORE1_ERROR;
                                  otaCore0State = OTA_CORE0_REPORT;
                                }
                                break;

                              default:
                                break;
                            }
                          }
                          break;

                        case OTA_CAN_CMD_2_WRITE:
                          {
                            switch (rx.data[1]) {
                              case OTA_CAN_CMD_1_PROJECT_ID:
                                {
                                  switch (rx.data[3]) {
                                    case OTA_CAN_CMD_3_SUCCESS:
                                      {
                                        Serial.println("OTA CORE1: PROJECT_ID UPDATE SUCCESS");
                                        otaCore1State = OTA_CORE1_WRITE_FIRMWARE_ID;
                                      }
                                      break;

                                    case OTA_CAN_CMD_3_FAIL:
                                      {
                                        Serial.println("OTA CORE1: PROJECT_ID UPDATE FAIL");
                                        sendReport = true;
                                        otaReportStatus = SOFTWARE_ID_WRITE_FAIL;

                                        otaCore1State = OTA_CORE1_ERROR;
                                        otaCore0State = OTA_CORE0_REPORT;
                                      }
                                      break;

                                    default:
                                      break;
                                  }
                                }
                                break;

                              case OTA_CAN_CMD_1_FIRMWARE_ID:
                                {
                                  switch (rx.data[3]) {
                                    case OTA_CAN_CMD_3_SUCCESS:
                                      {
                                        mcu_major_latest = ota_server_major;
                                        mcu_minor_latest = ota_server_minor;

                                        Serial.println("OTA CORE1: FIRMWARE_ID UPDATE SUCCESS");
                                        otaCore1State = OTA_CORE1_APP_JUMP;
                                      }
                                      break;

                                    case OTA_CAN_CMD_3_FAIL:
                                      {
                                        Serial.println("OTA CORE1: FIRMWARE_ID UPDATE FAIL");
                                        sendReport = true;
                                        otaReportStatus = SOFTWARE_ID_WRITE_FAIL;

                                        otaCore1State = OTA_CORE1_ERROR;
                                        otaCore0State = OTA_CORE0_REPORT;
                                      }
                                      break;

                                    default:
                                      break;
                                  }
                                }
                                break;

                              default:
                                break;
                            }
                          }
                          break;

                        default:
                          break;
                      }
                    }
                    break;

                  case OTA_CAN_CMD_0_APP_SIZE:
                    {
                      switch (rx.data[2]) {
                        case OTA_CAN_CMD_2_COMPARE:
                          {
                            switch (rx.data[3]) {
                              case OTA_CAN_CMD_3_SUCCESS:
                                {
                                  Serial.println("OTA CORE1: APP SIZE SUCCESS");
                                  otaCore1State = OTA_CORE1_APP_ERASE;
                                }
                                break;

                              case OTA_CAN_CMD_3_FAIL:
                                {
                                  Serial.println("OTA CORE1: APP SIZE FAIL");
                                  sendReport = true;
                                  otaReportStatus = APP_SIZE_EXCEED;

                                  otaCore1State = OTA_CORE1_ERROR;
                                  otaCore0State = OTA_CORE0_REPORT;
                                }
                                break;

                              default:
                                break;
                            }
                          }
                          break;

                        default:
                          break;
                      }
                    }
                    break;

                  case OTA_CAN_CMD_0_JUMP:
                    {
                      switch (rx.data[1]) {
                        case OTA_CAN_CMD_1_BOOT_JUMP:
                          {
                            switch (rx.data[3]) {
                              case OTA_CAN_CMD_3_SUCCESS:
                                {
                                  Serial.println("OTA CORE1: BOOT JUMP SUCCESS");
                                  otaCore1State = OTA_CORE1_COMPARE_APP_SIZE;
                                }
                                break;

                              case OTA_CAN_CMD_3_FAIL:
                                {
                                  Serial.println("OTA CORE1: BOOT JUMP FAIL");
                                  sendReport = true;
                                  otaReportStatus = BOOT_JUMP_FAIL;

                                  otaCore1State = OTA_CORE1_ERROR;
                                  otaCore0State = OTA_CORE0_REPORT;
                                }
                                break;

                              default:
                                break;
                            }
                          }
                          break;

                        case OTA_CAN_CMD_1_APP_JUMP:
                          {
                            switch (rx.data[3]) {
                              case OTA_CAN_CMD_3_SUCCESS:
                                {
                                  Serial.println("OTA CORE1: APP JUMP SUCCESS");
                                  sendReport = true;
                                  otaReportStatus = TWO_STEP_SUCCESS;  // ✅ SET SUCCESS

                                  otaCore1State = OTA_CORE1_DONE;
                                  otaCore0State = OTA_CORE0_REPORT;  // Trigger report
                                }
                                break;

                              case OTA_CAN_CMD_3_FAIL:
                                {
                                  Serial.println("OTA CORE1: APP JUMP FAIL");
                                  sendReport = true;
                                  otaReportStatus = APP_JUMP_FAIL;

                                  otaCore1State = OTA_CORE1_ERROR;
                                  otaCore0State = OTA_CORE0_REPORT;
                                }
                                break;

                              default:
                                break;
                            }
                          }
                          break;

                        default:
                          break;
                      }
                    }
                    break;

                  case OTA_CAN_CMD_0_APP_ERASE:
                    {
                      switch (rx.data[3]) {
                        case OTA_CAN_CMD_3_SUCCESS:
                          {
                            Serial.println("OTA CORE1: APP ERASE SUCCESS");
                            otaCore1State = OTA_CORE1_WRITE_APP_CHUNK;
                          }
                          break;

                        case OTA_CAN_CMD_3_FAIL:
                          {
                            Serial.println("OTA CORE1: APP ERASE FAIL");
                            sendReport = true;
                            otaReportStatus = APP_ERASE_FAIL;

                            otaCore1State = OTA_CORE1_ERROR;
                            otaCore0State = OTA_CORE0_REPORT;
                          }
                          break;

                        default:
                          break;
                      }
                    }
                    break;

                  case OTA_CAN_CMD_0_APP_CHUNK:
                    {
                      if ((rx.data[1] != chunkIndex) || (rx.data[2] != OTA_CAN_CMD_2_WRITE) || !waitingChunkAck) {
                        break;
                      }

                      if (rx.data[3] == OTA_CAN_CMD_3_SUCCESS) {
                        Serial.printf("OTA CORE1: APP chunk %u ACK OK (%u/%u bytes)\n",
                                      chunkIndex, totalSent, totalFWSize);
                        chunkIndex = (chunkIndex == 0xFFU) ? 1U : (uint8_t)(chunkIndex + 1U);
                        bytesSentInChunk = 0U;
                        waitingChunkAck = false;
                        otaCore1State = OTA_CORE1_WRITE_APP_CHUNK;
                      } else if (rx.data[3] == OTA_CAN_CMD_3_FAIL) {
                        Serial.printf("OTA CORE1: APP chunk %u rejected by bootloader\n",
                                      chunkIndex);
                        sendReport = true;
                        otaReportStatus = APP_CHUNK_WRITE_FAIL;
                        otaCore1State = OTA_CORE1_ERROR;
                        otaCore0State = OTA_CORE0_REPORT;
                      }
                    }
                    break;

                  case OTA_CAN_CMD_0_APP_CRC:
                    {
                      uint32_t picCrc;
                      uint32_t hostCrc = firmwareCrc32 ^ 0xFFFFFFFFU;

                      if (rx.data[2] != OTA_CAN_CMD_2_READ) {
                        break;
                      }

                      picCrc = ((uint32_t)rx.data[4] << 24) |
                               ((uint32_t)rx.data[5] << 16) |
                               ((uint32_t)rx.data[6] << 8)  |
                               ((uint32_t)rx.data[7]);

                      if ((rx.data[3] == OTA_CAN_CMD_3_SUCCESS) && (picCrc == hostCrc)) {
                        Serial.printf("OTA CORE1: APP CRC VERIFIED (HOST/PIC: 0x%08lX)\n", picCrc);
                        otaCore1State = OTA_CORE1_WRITE_PROJECT_ID;
                      } else {
                        Serial.printf("OTA CORE1: APP CRC MISMATCH (HOST: 0x%08lX, PIC: 0x%08lX)\n",
                                      hostCrc, picCrc);
                        sendReport = true;
                        otaReportStatus = APP_CRC_FAIL;
                        otaCore1State = OTA_CORE1_ERROR;
                        otaCore0State = OTA_CORE0_REPORT;
                      }
                    }
                    break;

                  default:
                    break;
                }
              }
              break;

            default:
              break;
          }

          /* ---------- Reset timeout ONLY on valid OTA RX ---------- */
          if (otaValidRx) {
            otaRxStartTime = millis();
            rxRetryCount = 0;  // RESET retry counter on success
          }
        }

        if ((millis() - otaRxStartTime) >= CAN_RX_TIMEOUT) {
          rxRetryCount++;

          Serial.printf(
            "OTA CORE1: RX timeout → retry %u / 5 (state %d)\n",
            rxRetryCount,
            otaPrevTxState);

          if (rxRetryCount >= 5) {

            Serial.println("OTA CORE1: Max retries reached → ABORT OTA");

            rxRetryCount = 0;

            switch (otaPrevTxState) {
              /* ---------- PIC ID FAILURE ---------- */
              case OTA_CORE1_READ_PIC_ID_1:
              case OTA_CORE1_READ_PIC_ID_2:
              case OTA_CORE1_READ_PIC_ID_3:
              case OTA_CORE1_READ_PIC_ID_4:
                {
                  Serial.println("OTA CORE1: FAILED TO GET PIC ID");

                  sendReport = true;
                  otaReportStatus = PIC_ID_READ_FAIL;

                  otaCore1State = OTA_CORE1_ERROR;
                  otaCore0State = OTA_CORE0_REPORT;
                }
                break;

              case OTA_CORE1_READ_PROJECT_ID:
              case OTA_CORE1_READ_FIRMWARE_ID:
                {
                  Serial.println("OTA CORE1: FAILED TO GET SOFTWARE ID");

                  sendReport = true;
                  otaReportStatus = SOFTWARE_ID_READ_FAIL;

                  otaCore1State = OTA_CORE1_ERROR;
                  otaCore0State = OTA_CORE0_REPORT;
                }
                break;

              case OTA_CORE1_COMPARE_APP_SIZE:
                {
                  Serial.println("OTA CORE1: APPLICATION SIZE EXCEED");

                  sendReport = true;
                  otaReportStatus = APP_SIZE_EXCEED;

                  otaCore1State = OTA_CORE1_ERROR;
                  otaCore0State = OTA_CORE0_REPORT;
                }
                break;

              case OTA_CORE1_BOOT_JUMP:
                {
                  Serial.println("OTA CORE1: BOOTLOADER JUMP FAILED");

                  sendReport = true;
                  otaReportStatus = BOOT_JUMP_FAIL;

                  otaCore1State = OTA_CORE1_ERROR;
                  otaCore0State = OTA_CORE0_REPORT;
                }
                break;

              case OTA_CORE1_APP_ERASE:
                {
                  Serial.println("OTA CORE1: MCU MEMORY ERASE FAILED");

                  sendReport = true;
                  otaReportStatus = APP_ERASE_FAIL;

                  otaCore1State = OTA_CORE1_ERROR;
                  otaCore0State = OTA_CORE0_REPORT;
                }
                break;

              case OTA_CORE1_WRITE_APP_CHUNK:
                {
                  Serial.println("OTA CORE1: FIRMWARE UPLOAD FAILED");

                  sendReport = true;
                  otaReportStatus = APP_CHUNK_WRITE_FAIL;

                  otaCore1State = OTA_CORE1_ERROR;
                  otaCore0State = OTA_CORE0_REPORT;
                }
                break;

              case OTA_CORE1_READ_APP_CRC:
                {
                  Serial.println("OTA CORE1: APPLICATION CRC VALIDATION FAILED");
                  sendReport = true;
                  otaReportStatus = APP_CRC_FAIL;
                  otaCore1State = OTA_CORE1_ERROR;
                  otaCore0State = OTA_CORE0_REPORT;
                }
                break;

              case OTA_CORE1_WRITE_PROJECT_ID:
              case OTA_CORE1_WRITE_FIRMWARE_ID:
                {
                  Serial.println("OTA CORE1: SOFTWARE ID UPDATE FAILED");

                  sendReport = true;
                  otaReportStatus = SOFTWARE_ID_WRITE_FAIL;

                  otaCore1State = OTA_CORE1_ERROR;
                  otaCore0State = OTA_CORE0_REPORT;
                }
                break;

              case OTA_CORE1_APP_JUMP:
                {
                  Serial.println("OTA CORE1: APP JUMP FAIL");

                  sendReport = true;
                  otaReportStatus = APP_JUMP_FAIL;

                  otaCore1State = OTA_CORE1_ERROR;
                  otaCore0State = OTA_CORE0_REPORT;
                }

              default:
                {
                  otaCore1State = OTA_CORE1_ERROR;
                  otaCore0State = OTA_CORE0_ERROR;
                }
                break;
            }
          } else {
            otaCore1State = otaPrevTxState;  // retry previous TX state
          }
        }
      }
      break;

    // ==================================================
    case OTA_CORE1_DONE:
      {
        LED_Green_Off();

        otaServerVersionUpdated = false;
        downloadBinFile = false;
        isBinFileDownloaded = false;
        mcuUpdateStarted = false;

        // -------- RESET OTA DATA STATE --------
        totalFWSize = 0;
        totalSent = 0;
        chunkIndex = 1U;
        bytesSentInChunk = 0U;
        waitingChunkAck = false;
        firmwareCrc32 = 0xFFFFFFFFU;
        fileOpened = false;

        if (fwFile) fwFile.close();

        otaCore1State = OTA_CORE1_IDLE;
      }
      break;

    // ==================================================
    case OTA_CORE1_ERROR:
      {
        LED_Green_Off();
        // Serial.println("OTA CORE1: ERROR state");
      }
      break;

    default:
      break;
  }

  return false;
}

void CORE0_OTA_Task(void *p) {
  while (1) {

    if (WiFi.status() == WL_CONNECTED) {
      OTA_Handler_CORE0(mac);
    }

    // OTA FSM runs fast but non-blocking
    vTaskDelay(200 / portTICK_PERIOD_MS);
  }
}
