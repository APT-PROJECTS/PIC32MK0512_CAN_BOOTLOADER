#include "can_handler.h"

// ----------------------------------------------------
// Initialize CAN Bus
// ----------------------------------------------------
void CAN_Init(void)
{
    ESP32Can.setPins(CAN_TX_PIN, CAN_RX_PIN);
    ESP32Can.setSpeed(ESP32Can.convertSpeed(CAN_SPEED));

    if (ESP32Can.begin()) {
        Serial.println("CAN started OK");
    } else {
        Serial.println("CAN start FAILED!");
    }
}

// ----------------------------------------------------
// GENERIC CAN WRITE
// ----------------------------------------------------
bool CAN_WriteFrame(uint32_t id, const uint8_t *data, uint8_t len)
{
    if (len > 8) return false;

    CanFrame tx;
    tx.identifier = id;
    tx.extd = 0;
    tx.rtr = 0;
    tx.data_length_code = len;

    memset(tx.data, 0, 8);
    memcpy(tx.data, data, len);

    // return ESP32Can.writeFrame(tx);
    ESP32Can.writeFrame(tx);
    return true;
}

// ----------------------------------------------------
// GENERIC CAN READ (WITH TIMEOUT)
// ----------------------------------------------------
bool CAN_ReadFrame(CanFrame *rx, uint32_t timeout_ms)
{
    unsigned long endTime = millis() + timeout_ms;

    while (millis() < endTime) {
        if (ESP32Can.readFrame(*rx, 0)) {
            return true;
        }
    }
    return false;
}


