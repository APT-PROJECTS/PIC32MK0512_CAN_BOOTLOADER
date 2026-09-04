#ifndef TUNE_HANDLER_H
#define TUNE_HANDLER_H

#include <stdint.h>
#include <WiFi.h>
#include <HTTPClient.h>
#include <ArduinoJson.h>

// ---- Globals ----
extern uint8_t tuner_read;
extern uint8_t tuner_write;
extern uint8_t zeroAngleEstimate;

// ---- State Machine ----
typedef enum
{
    CORE0_TUNNER_CHECK_STATUS = 0,
    CORE0_TUNNER_READ,
    CORE0_TUNNER_WRITE,
    CORE0_TUNNER_CHECK_WRITE_SUCCESS,
    CORE0_TUNNER_ZERO_ANGLE_ESTIMATE,
    CORE0_TUNNER_DONE,
    CORE0_TUNNER_ERROR
} CORE0_TUNNER_STATE;

typedef enum
{
    CORE1_TUNNER_CHECK_STATUS = 0,
    CORE1_TUNNER_READ,
    CORE1_TUNNER_CAN_RX_READ,
    CORE1_TUNNER_WRITE,
    CORE1_TUNNER_CAN_RX_WRITE,
    CORE1_TUNNER_ZERO_ANGLE_ESTIMATE,
    CORE1_TUNNER_CAN_RX_ZERO_ANGLE_ESTIMATE
} CORE1_TUNNER_STATE;

// ---- Task ----
void CORE0_TUNER_Task(void *pvParameters);
void CORE1_TUNNER_HANDLER(void);

#endif