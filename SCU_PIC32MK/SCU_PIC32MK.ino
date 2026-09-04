#include "system_init.h"
#include "tcu_handler.h"
#include "ota_handler.h"
#include "tune_handler.h"

// --------------------------
// SETUP
// --------------------------
void setup() {
  System_Init();
  System_StartTimer();
  System_CreateTasks();

  Serial.println("System Ready.");
}

// --------------------------
// LOOP (CORE1)
// --------------------------
void loop() {
  // tcuStateMachine();
  OTA_Handler_CORE1();
  // CORE1_TUNNER_HANDLER();
}
