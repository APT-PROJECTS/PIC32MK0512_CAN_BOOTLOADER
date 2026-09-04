#include "mcu_battery_handler.h"
#include "can_handler.h"

// ----------------------------------------------------
// Send 0xC0 request frame to Motor Controller
// ----------------------------------------------------
void MCU_DataRequest(void) {
  // Request payload
  uint8_t mcuDataRequest[8] = { 0, 1, 2, 3, 4, 5, 6, 7 };
  CAN_WriteFrame(MCU_DATA_REQUEST_ID, mcuDataRequest, 8);
}

// ----------------------------------------------------
// Read Motor Controller data frames C1–C6 into MCU[48]
// ----------------------------------------------------
void MCU_Battery_DataResponse(uint8_t *mcuBuffer, uint8_t *scuBuffer) {
  CanFrame rx;
  uint32_t start = millis();  // or your system tick
  bool frameReceived = false;

  while ((millis() - start) < 20)  // 20 ms window
  {
    if (CAN_ReadFrame(&rx, 1))  // small timeout (1 ms)
    {
      frameReceived = true;

      switch (rx.identifier) {
        case MCU_DUMMY_AND_THROTTLE:
          memcpy(&mcuBuffer[0], rx.data, 8);
          break;

        case MCU_BRAKE_AND_BATTERY_VOLTAGE:
          memcpy(&mcuBuffer[8], rx.data, 8);
          break;

        case MCU_BATTERY_CURRENT_AND_RPM:
          memcpy(&mcuBuffer[16], rx.data, 8);
          break;

        case MCU_TORQUE_AND_MCU_TEMPERATURE:
          memcpy(&mcuBuffer[24], rx.data, 8);
          break;

        case MCU_MOTOR_TEMPERATURE_AND_PHASE_CURRENT:
          memcpy(&mcuBuffer[32], rx.data, 8);
          break;

        case MCU_ID_AND_IQ:
          memcpy(&mcuBuffer[40], rx.data, 8);
          break;

        case BATTERY_STATE_ID:
          {
            // Make sure extended frame
            if (rx.extd == 1) {
              // SOC → Motorola 16bit starting at bit 8
              uint16_t soc_raw = (rx.data[0] << 8) | rx.data[1];
              battery_soc = soc_raw * 0.01f;
            }
          }
          break;

        case TEMPERATURE_INFO_CELL_ID:
          {
            if (rx.extd == 1) {
              int16_t temp_raw = (rx.data[4] << 8) | rx.data[5];
              battery_mean_temperature = temp_raw * 0.1f;
            }
          }
          break;

        default:
          break;
      }
    }
  }

  memcpy(&scuBuffer[5 * 4], &battery_mean_temperature, sizeof(float));
  // Serial.print("Battery SOC: ");
  // Serial.print(battery_soc);
  // Serial.println(" %");

  memcpy(&scuBuffer[7 * 4], &battery_soc, sizeof(float));
  // Serial.print("Mean Temp: ");
  // Serial.print(battery_mean_temperature);
  // Serial.println(" °C");

  if (!frameReceived) {
    memset(mcuBuffer, 0x00, 48);
  }
}
