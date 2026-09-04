#ifndef CONFIG_HANDLER_H
#define CONFIG_HANDLER_H

#include <Arduino.h>
#include <stdbool.h>

// --------------------------
// API BASE URL
// --------------------------
#define API_BASE_URL "http://vehicle.aproposdrive.com/api/device-configs"

typedef enum {
  CFG_STATE_IDLE = 0,
  CFG_STATE_CHECK_UPDATE,
  CFG_STATE_GET_CONFIG,
  CFG_STATE_DONE_UPDATE,
  CFG_STATE_DONE
} ConfigState_t;

typedef enum {
  CFG_SUB_SCU_ID = 0,
  CFG_SUB_WIFI,
  CFG_SUB_LIVE_MODE,
  CFG_SUB_FINISH
} ConfigSubState_t;

// --------------------------
// API
// --------------------------
bool Config_Handler_Run(const uint8_t *mac, uint8_t *scu_id);
void CORE0_CONFIG_Task(void *p);

#endif  // API_HANDLER_H
