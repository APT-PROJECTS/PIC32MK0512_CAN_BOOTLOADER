#include "tcu_handler.h"
#include "eeprom_handler.h"
#include "wifi_mqtt_handler.h"
#include "can_handler.h"
#include "gps_handler.h"
#include "sd_handler.h"
#include "imu_handler.h"
#include "rtc_handler.h"
#include "mcp23017_handler.h"
#include "mcu_battery_handler.h"

RTC_Time_t now;

// --------------------------
// STATE INIT
// --------------------------
SystemState currentState = STATE_RUN;
RunState_t runState = RUN_SD_READ;
SleepState_t sleepState = SLEEP_READ_INPUT_VOLTAGE;

// --------------------------
// TIMER
// --------------------------
volatile bool tick_1ms = false;
esp_timer_handle_t periodic_timer = NULL;

// --------------------------
// COUNTERS
// --------------------------
uint16_t runCounter = 0;
uint16_t packetIndex = 0;

uint32_t sdWriteIndex = 0;
uint32_t sdReadIndex = 0;

// --------------------------
// BUFFERS
// --------------------------
uint8_t MCU[48] = { 0 };
uint8_t SCU[32] = { 0 };
uint8_t data800[800] = { 0 };

uint8_t RTC[8] = { 0 };
uint8_t SCU_ID[4] = { 0xFF, 0xFF, 0xFF, 0xFF };

uint8_t SD_WriteBuf[808] = { 0 };
uint8_t SD_ReadBuf[808] = { 0 };
uint8_t MQTTpacket[812] = { 0 };

// --------------------------
// FLAGS
// --------------------------
volatile bool dataPrepared = false;
volatile bool dataPublished = true;

// --------------------------
// 1 ms timer
// --------------------------
void IRAM_ATTR onTimer(void *arg) {
  tick_1ms = true;
}

void stateRUN() {
  // static uint32_t sampleTimer = 0;

  switch (runState) {
    // -------------------------------------------------
    case RUN_SD_READ:
      {
        if ((dataPublished == true) && (sdWriteIndex > sdReadIndex)) {
          if (SD_Read808Bytes(SD_ReadBuf, sdReadIndex)) {
            Serial.printf("CORE1: SD READ OK @ %lu\n", sdReadIndex);
            sdReadIndex += 2;
            dataPrepared = true;
            dataPublished = false;
          } else {
            Serial.println("CORE1: SD READ FAIL!");
          }
        }

        // sampleTimer = millis();
        runState = RUN_WAIT_SAMPLE;
      }
      break;

    // -------------------------------------------------
    case RUN_WAIT_SAMPLE:
      {
        if (runCounter < 72) {
          runState = RUN_SD_READ;
          // currentState = STATE_SLEEP;
        } else {
          runCounter = 0;
          runState = RUN_COLLECT_SAMPLE;
          currentState = STATE_SLEEP;
        }
      }
      break;

    // -------------------------------------------------
    case RUN_COLLECT_SAMPLE:
      {
        LED_Red_Off();

        MCU_DataRequest();
        MCU_Battery_DataResponse(MCU, SCU);

        IMU_Update(SCU);
        GPS_Update(SCU);

        uint16_t offset = packetIndex * 80;

        memcpy(&data800[offset], MCU, 48);
        memcpy(&data800[offset + 48], SCU, 32);

        // Clear buffers
        memset(MCU, 0x00, sizeof(MCU));
        memset(SCU, 0x00, sizeof(SCU));

        packetIndex++;

        if (packetIndex >= 10) {
          packetIndex = 0;
          runState = RUN_RTC_UPDATE;
        } else {
          // sampleTimer = millis();
          runState = RUN_WAIT_SAMPLE;
        }
      }
      break;

    // -------------------------------------------------
    case RUN_RTC_UPDATE:
      {
        RTC_GetTime(&now);
        RTC_Store(RTC, now);
        RTC_PrintTime(now, RTC);

        memcpy(&SD_WriteBuf[0], RTC, 8);
        memcpy(&SD_WriteBuf[8], data800, 800);

        runState = RUN_SD_WRITE;
      }
      break;

    // -------------------------------------------------
    case RUN_SD_WRITE:
      {
        if (SD_Write808Bytes(SD_WriteBuf, sdWriteIndex)) {
          Serial.printf("CORE1: SD WRITE OK @ %lu\n", sdWriteIndex);
          sdWriteIndex += 2;
        } else {
          Serial.println("CORE1: SD WRITE FAIL!");
        }
        LED_Red_On();

        runState = RUN_SD_READ;  // restart cycle
      }
      break;
  }
}

void stateSLEEP() {
  uint32_t magic = MAGIC_VALUE;

  switch (sleepState) {
    // ------------------------------------------
    case SLEEP_READ_INPUT_VOLTAGE:
      {
        uint16_t adcValue = analogRead(INPUT_VOLTAGE_SENSE);

        float voltage = ((adcValue / 4095.0f) * 3.3f) + 0.15;
        voltage *= 2.0f;  // because of voltage divider (*2)

        if (voltage < 4.2) {
          sleepState = STORE_LAST_DATA;
        }

        // Serial.printf("SLEEP: Input Voltage = %.2f V\n", voltage);
        currentState = STATE_RUN;
      }
      break;

    // --------------------------------------------
    case STORE_LAST_DATA:
      {
        LED_Red_On();
        LED_Blue_On();
        LED_Green_On();

        EEPROM_Write(SD_READ_INDEX, (uint8_t *)&sdReadIndex, sizeof(sdReadIndex));
        EEPROM_Write(SD_WRITE_INDEX, (uint8_t *)&sdWriteIndex, sizeof(sdWriteIndex));
        EEPROM_Write(LONGITUDE, (uint8_t *)&longitude, sizeof(longitude));
        EEPROM_Write(LATITUDE, (uint8_t *)&latitude, sizeof(latitude));
        EEPROM_Write(MAGIC_FLAG, (uint8_t *)&magic, sizeof(magic));

        delay(2000);

        LED_Red_Off();
        LED_Blue_Off();
        LED_Green_Off();

        digitalWrite(BATTERY_CONTROL, LOW);

        sleepState = SLEEP_READ_INPUT_VOLTAGE;
      }
      break;
  }
}


// --------------------------
// STATE MACHINE RUNNER
// --------------------------
void tcuStateMachine() {
  if (!tick_1ms) return;
  tick_1ms = false;
  runCounter++;

  if(mqttWasConnected == true && wifiWasConnected == true){
    LED_Blue_On();
  }
  else{
    LED_Blue_Off();
  }

  switch (currentState) {
    case STATE_RUN:
      stateRUN();
      break;

    case STATE_SLEEP:
      stateSLEEP();
      break;
  }
}

void CORE0_MQTT_Task(void *p) {
  WIFI_MQTT_InitCore0();

  EEPROM_Read(EEPROM_SCU_ADDR, SCU_ID, 4);

  while (1) {

    // ❌ BLOCK MQTT IF SCU_ID INVALID
    if (isEEPROMEmpty(SCU_ID, sizeof(SCU_ID))) {
      vTaskDelay(1000 / portTICK_PERIOD_MS);
      continue;
    }

    if (dataPrepared) {

      memcpy(&MQTTpacket[0], SCU_ID, 4);
      memcpy(&MQTTpacket[4], &SD_ReadBuf[0], 8);
      memcpy(&MQTTpacket[12], &SD_ReadBuf[8], 800);

      MQTT_Publish812(MQTTpacket);
    }

    vTaskDelay(5 / portTICK_PERIOD_MS);
  }
}
