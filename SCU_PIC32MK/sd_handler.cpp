#include "sd_handler.h"

SdFs sd;

// -----------------------------------------
// SD Initialization
// -----------------------------------------
bool SD_Init() {
  SPI.begin(SD_SCK, SD_MISO, SD_MOSI, SD_CS);

  SdSpiConfig spiConfig(SD_CS, DEDICATED_SPI, SD_SCK_MHZ(25));

  if (!sd.begin(spiConfig)) {
    Serial.println("ERROR: SD Card failed to initialize!");
    return false;
  }

  Serial.println("SD Card Initialized (SdFat v2)");
  return true;
}

// -----------------------------------------
// Write 808 bytes as 2 x 512-byte blocks
// buf = [RTC(8) + DATA(800)]
// -----------------------------------------
bool SD_Write808Bytes(uint8_t *buf, uint32_t blockIndex) {
  // ============ BLOCK 0 ============
  memset(blockBuf, 0, 512);
  memcpy(blockBuf, buf, 404);  // 404 + 404 = 808

  if (!sd.card()->writeSector(blockIndex, blockBuf)) {
    Serial.println("SD Write ERROR: Block 0");
    return false;
  }

  // ============ BLOCK 1 ============
  memset(blockBuf, 0, 512);
  memcpy(blockBuf, buf + 404, 404);

  if (!sd.card()->writeSector(blockIndex + 1, blockBuf)) {
    Serial.println("SD Write ERROR: Block 1");
    return false;
  }

  Serial.printf("SD: Wrote 808 bytes @ blocks %lu & %lu\n",
                blockIndex, blockIndex + 1);
  return true;
}

// -----------------------------------------
// Read 808 bytes from SD card
// Reads 2 x 512-byte blocks internally
// buf = [RTC(8) + DATA(800)]
// -----------------------------------------
bool SD_Read808Bytes(uint8_t *buf, uint32_t blockIndex) {
  uint8_t block0[512];
  uint8_t block1[512];

  // ---- Read first block ----
  if (!sd.card()->readSector(blockIndex, block0)) {
    Serial.printf("SD Read ERROR @ block %lu\n", blockIndex);
    return false;
  }

  // ---- Read second block ----
  if (!sd.card()->readSector(blockIndex + 1, block1)) {
    Serial.printf("SD Read ERROR @ block %lu\n", blockIndex + 1);
    return false;
  }

  // ---- Extract 808 bytes ----
  memcpy(buf, block0, 404);
  memcpy(buf + 404, block1, 404);

  return true;
}
