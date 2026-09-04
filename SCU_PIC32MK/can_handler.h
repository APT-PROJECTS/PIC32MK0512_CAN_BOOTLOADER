#ifndef CAN_HANDLER_H
#define CAN_HANDLER_H

#include <Arduino.h>
#include <ESP32-TWAI-CAN.hpp>

#define CAN_TX_PIN 18 // 5  //25 
#define CAN_RX_PIN 19 // 4  //26 
#define CAN_SPEED  500

void CAN_Init();
bool CAN_WriteFrame(uint32_t id, const uint8_t *data, uint8_t len);
bool CAN_ReadFrame(CanFrame *rx, uint32_t timeout_ms);

#endif
