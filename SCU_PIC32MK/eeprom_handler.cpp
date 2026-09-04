#include "eeprom_handler.h"

// --------------------------
// INIT
// --------------------------
void EEPROM_Init(void)
{
  EEPROM.begin(EEPROM_TOTAL_SIZE);
}

// --------------------------
// GENERIC READ
// --------------------------
bool EEPROM_Read(uint16_t addr, uint8_t *data, uint16_t len)
{
  if ((addr + len) > EEPROM_TOTAL_SIZE)
    return false;

  for (uint16_t i = 0; i < len; i++) {
    data[i] = EEPROM.read(addr + i);
  }

  return true;
}

// --------------------------
// GENERIC WRITE
// --------------------------
bool EEPROM_Write(uint16_t addr, const uint8_t *data, uint16_t len)
{
  if ((addr + len) > EEPROM_TOTAL_SIZE)
    return false;

  for (uint16_t i = 0; i < len; i++) {
    EEPROM.write(addr + i, data[i]);
  }

  return EEPROM.commit();
}

bool isEEPROMEmpty(const uint8_t *buf, size_t len)
{
  for (size_t i = 0; i < len; i++) {
    if (buf[i] != 0xFF)
      return false;
  }
  return true;
}
