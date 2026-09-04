#include "system_init.h"
#include "mac_handler.h"
#include "eeprom_handler.h"
#include "can_handler.h"
#include "imu_handler.h"
#include "rtc_handler.h"
#include "mcp23017_handler.h"
#include "gps_handler.h"
#include "sd_handler.h"
#include "tcu_handler.h"
#include "config_handler.h"
#include "ota_handler.h"
#include "tune_handler.h"

TaskHandle_t core0MqttTaskHandle;
TaskHandle_t core0ConfigTaskHandle;
TaskHandle_t core0OtaTaskHandle;
TaskHandle_t core0TunerTaskHandle;

// ----------------------------------------------------
// Initialize all drivers
// ----------------------------------------------------
void System_Init(void) {
  uint32_t magicFlagRead;

  Serial.begin(115200);
  delay(200);

  pinMode(BATTERY_CONTROL, OUTPUT);
  digitalWrite(BATTERY_CONTROL, HIGH);

  pinMode(INPUT_VOLTAGE_SENSE, INPUT);

  MAC_Init();
  EEPROM_Init();
  CAN_Init();
  // IMU_Init();
  // RTC_Init();
  // MCP23017_Init();
  // GPS_Init();
  // SD_Init();

  EEPROM_Read(MAGIC_FLAG, (uint8_t *)&magicFlagRead, sizeof(magicFlagRead));
  if (magicFlagRead == MAGIC_VALUE) {
    EEPROM_Read(SD_READ_INDEX, (uint8_t *)&sdReadIndex, sizeof(sdReadIndex));
    EEPROM_Read(SD_WRITE_INDEX, (uint8_t *)&sdWriteIndex, sizeof(sdWriteIndex));
    EEPROM_Read(LONGITUDE, (uint8_t *)&longitude, sizeof(longitude));
    EEPROM_Read(LATITUDE, (uint8_t *)&latitude, sizeof(latitude));
  } else {
    sdReadIndex = 0;
    sdWriteIndex = 0;
  }

  Serial.println("Peripherals Initialized.");
}

// ----------------------------------------------------
// Start 1ms timer
// ----------------------------------------------------
void System_StartTimer(void) {
  const esp_timer_create_args_t args = {
    .callback = &onTimer,
    .arg = NULL,
    .dispatch_method = ESP_TIMER_TASK,
    .name = "1ms_tick"
  };

  esp_timer_create(&args, &periodic_timer);
  esp_timer_start_periodic(periodic_timer, 1000);
}

// ----------------------------------------------------
// Create Core0 Tasks
// ----------------------------------------------------
void System_CreateTasks(void)
{
    xTaskCreatePinnedToCore(
        CORE0_MQTT_Task,
        "MQTT0",
        20480,
        NULL,
        1,
        &core0MqttTaskHandle,
        0);

    xTaskCreatePinnedToCore(
        CORE0_CONFIG_Task,
        "CFG0",
        12288,
        NULL,
        1,
        &core0ConfigTaskHandle,
        0);

    xTaskCreatePinnedToCore(
        CORE0_OTA_Task,
        "OTA0",
        16384,
        NULL,
        1,
        &core0OtaTaskHandle,
        0);

    // xTaskCreatePinnedToCore(
    //     CORE0_TUNER_Task,
    //     "TUNER0",
    //     12288,              // adjust if using JSON parsing
    //     NULL,
    //     1,
    //     &core0TunerTaskHandle,
    //     0);

    Serial.println("Core0 Tasks Created.");
}