#ifndef SD_HANDLER_H
#define SD_HANDLER_H

#include <Arduino.h>
#include <SPI.h>
#include <SdFat.h>

// SD Pins for LilyGO T-SIM7000G
#define SD_MISO   19 //2
#define SD_MOSI   23 //15
#define SD_SCK    18 //14
#define SD_CS     32 //13

// 512-byte working buffer
static uint8_t blockBuf[512];

// Functions
bool SD_Init();
bool SD_Write808Bytes(uint8_t *buf, uint32_t blockIndex);
bool SD_Read808Bytes(uint8_t *buf, uint32_t blockIndex);

#endif
