#ifndef _DEVICE_STAT_H    /* Guard against multiple inclusion */
#define _DEVICE_STAT_H


#ifdef __cplusplus
extern "C" {
#endif
    
#include "driver.h"
    
extern uint32_t devsn[4];
extern uint32_t devid;

//--------------------------------------------------
// MATRIX SIZE
//--------------------------------------------------
#define ROWS    16
#define COLS    6

//--------------------------------------------------
// EEPROM BUFFER
//--------------------------------------------------
extern uint8_t eepromBuffer[EEPROM_DATA_SIZE];

//--------------------------------------------------
// EXTERNAL MATRIX
//--------------------------------------------------
extern float readMatrix[ROWS][COLS];
extern float writeMatrix[ROWS][COLS];

//==================================================
// ROW 0
//==================================================
#define R_PROJECT_ID                 readMatrix[0][0]
#define R_FIRMWARE_ID                readMatrix[0][1]
#define R_BOOT_FLAG                  readMatrix[0][2]
#define R_BOOT_VERSION               readMatrix[0][3]
#define R_APP_SIZE                   readMatrix[0][4]
#define R_APP_CRC                    readMatrix[0][5]

#define W_PROJECT_ID                 writeMatrix[0][0]
#define W_FIRMWARE_ID                writeMatrix[0][1]
#define W_BOOT_FLAG                  writeMatrix[0][2]
#define W_BOOT_VERSION               writeMatrix[0][3]
#define W_APP_SIZE                   writeMatrix[0][4]
#define W_APP_CRC                    writeMatrix[0][5]

    
void Get_PIC_ID(void);
void EEPROM_Initialize(void);
void Load_Read_Matrix(void);

#ifdef __cplusplus
}
#endif

#endif /* _DEVICE_STAT_H */