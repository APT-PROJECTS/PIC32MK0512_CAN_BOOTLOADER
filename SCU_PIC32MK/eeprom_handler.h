#ifndef EEPROM_HANDLER_H
#define EEPROM_HANDLER_H

#include <Arduino.h>
#include <EEPROM.h>

// --------------------------
// CONFIG
// --------------------------
#define EEPROM_TOTAL_SIZE     256     // adjust if needed
#define EEPROM_SCU_ADDR       0x00    // 4 bytes
#define EEPROM_WIFI_SSID      0x10    // 32 bytes
#define EEPROM_WIFI_PASS      0x30    // 32 bytes
#define SD_READ_INDEX         0x50    
#define SD_WRITE_INDEX        0x60
#define LONGITUDE             0x70
#define LATITUDE              0x80
#define MAGIC_FLAG            0x90
#define MAGIC_VALUE           0x12345678   // any unique number

// --------------------------
// API
// --------------------------
void EEPROM_Init(void);
bool EEPROM_Read(uint16_t addr, uint8_t *data, uint16_t len);
bool EEPROM_Write(uint16_t addr, const uint8_t *data, uint16_t len);
bool isEEPROMEmpty(const uint8_t *buf, size_t len);

#endif  // EEPROM_HANDLER_H
