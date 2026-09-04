#ifndef TCU_HANDLER_H
#define TCU_HANDLER_H

#include <Arduino.h>

#define BATTERY_CONTROL         13    // digital out
#define INPUT_VOLTAGE_SENSE     33    // analog in * 2

#define THROTTLE_SIGNAL         34    // analog in * 0.66
#define BRAKE_HIGH              35    // analog in * 0.11

// --------------------------
// STATE MACHINE
// --------------------------
typedef enum {
  STATE_RUN = 0,
  STATE_SLEEP
} SystemState;

typedef enum {
  RUN_SD_READ = 0,
  RUN_WAIT_SAMPLE,
  RUN_COLLECT_SAMPLE,
  RUN_BUILD_SECOND,
  RUN_RTC_UPDATE,
  RUN_SD_WRITE
} RunState_t;

typedef enum
{
    SLEEP_READ_INPUT_VOLTAGE = 0,
    STORE_LAST_DATA
} SleepState_t;

// --------------------------
// TIMER
// --------------------------
extern volatile bool tick_1ms;
extern esp_timer_handle_t periodic_timer;

// --------------------------
// COUNTERS
// --------------------------
extern uint16_t runCounter;
extern uint16_t packetIndex;

extern uint32_t sdWriteIndex;
extern uint32_t sdReadIndex;

// --------------------------
// BUFFERS
// --------------------------
extern uint8_t MCU[48];
extern uint8_t SCU[32];
extern uint8_t data800[800];

extern uint8_t RTC[8];
extern uint8_t SCU_ID[4];

extern uint8_t SD_WriteBuf[808];
extern uint8_t SD_ReadBuf[808];
extern uint8_t MQTTpacket[812];

// --------------------------
// FLAGS
// --------------------------
extern volatile bool dataPrepared;
extern volatile bool dataPublished;


// --------------------------
// FUNCTIONS
// --------------------------
void IRAM_ATTR onTimer(void *arg);
void stateRUN();
void stateSLEEP();
void tcuStateMachine();
void CORE0_MQTT_Task(void *p);


#endif
