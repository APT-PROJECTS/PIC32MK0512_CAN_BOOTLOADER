#ifndef MCU_BATTERY_HANDLER_H
#define MCU_BATTERY_HANDLER_H

#include <Arduino.h>

// ---------------- MCU IDs ----------------
#define MCU_DATA_REQUEST_ID 0x0C0

#define MCU_DUMMY_AND_THROTTLE                      0x0C1
#define MCU_BRAKE_AND_BATTERY_VOLTAGE               0x0C2
#define MCU_BATTERY_CURRENT_AND_RPM                 0x0C3
#define MCU_TORQUE_AND_MCU_TEMPERATURE              0x0C4
#define MCU_MOTOR_TEMPERATURE_AND_PHASE_CURRENT     0x0C5
#define MCU_ID_AND_IQ                               0x0C6

// ---------------- MURATA BATTERY IDs ----------------
#define BATTERY_STATE_ID                            0x18FF05D0  
#define TEMPERATURE_INFO_CELL_ID                    0x18FF03D0  
#define VOLTAGE_INFO_ID                             0x18FF01D0  
#define CURRENT_ID                                  0x18FF02D0
#define ERROR_INFO_ID                               0x18FF10D0
#define BMS_EVENT_INFO_ID                           0x18FF07D0

static float battery_soc;
static float battery_mean_temperature;

void MCU_DataRequest(void);
void MCU_Battery_DataResponse(uint8_t *mcuBuffer, uint8_t *scuBuffer);

#endif