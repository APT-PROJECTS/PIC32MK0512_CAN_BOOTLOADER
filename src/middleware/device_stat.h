#ifndef _DEVICE_STAT_H    /* Guard against multiple inclusion */
#define _DEVICE_STAT_H


#ifdef __cplusplus
extern "C" {
#endif
    
#include "../driver/flash_eeprom.h"
    
extern uint32_t devsn[4];

//--------------------------------------------------
// MATRIX SIZE
//--------------------------------------------------
#define ROWS    16
#define COLS    4

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
#define R_APP_SIZE                   readMatrix[0][3]

#define W_PROJECT_ID                 writeMatrix[0][0]
#define W_FIRMWARE_ID                writeMatrix[0][1]
#define W_BOOT_FLAG                  writeMatrix[0][2]
#define W_APP_SIZE                   writeMatrix[0][3]

//==================================================
// ROW 1
//==================================================
#define R_FF01_01                    readMatrix[1][0]
#define R_FF01_02                    readMatrix[1][1]
#define R_FF01_03                    readMatrix[1][2]
#define R_FF01_04                    readMatrix[1][3]

#define W_FF01_01                    writeMatrix[1][0]
#define W_FF01_02                    writeMatrix[1][1]
#define W_FF01_03                    writeMatrix[1][2]
#define W_FF01_04                    writeMatrix[1][3]

//==================================================
// ROW 2
//==================================================
#define R_FF02_01                    readMatrix[2][0]
#define R_FF02_02                    readMatrix[2][1]
#define R_FF02_03                    readMatrix[2][2]
#define R_FF02_04                    readMatrix[2][3]

#define W_FF02_01                    writeMatrix[2][0]
#define W_FF02_02                    writeMatrix[2][1]
#define W_FF02_03                    writeMatrix[2][2]
#define W_FF02_04                    writeMatrix[2][3]

//==================================================
// ROW 3
//==================================================
#define R_FF03_01                    readMatrix[3][0]
#define R_FF03_02                    readMatrix[3][1]
#define R_FF03_03                    readMatrix[3][2]
#define R_FF03_04                    readMatrix[3][3]

#define W_FF03_01                    writeMatrix[3][0]
#define W_FF03_02                    writeMatrix[3][1]
#define W_FF03_03                    writeMatrix[3][2]
#define W_FF03_04                    writeMatrix[3][3]

//==================================================
// ROW 4
//==================================================
#define R_FF04_01                    readMatrix[4][0]
#define R_FF04_02                    readMatrix[4][1]   //(NA)
#define R_FF04_03                    readMatrix[4][2]
#define R_FF04_04                    readMatrix[4][3]

#define W_FF04_01                    writeMatrix[4][0]
#define W_FF04_02                    writeMatrix[4][1]   //(NA)
#define W_FF04_03                    writeMatrix[4][2]
#define W_FF04_04                    writeMatrix[4][3]

//==================================================
// ROW 5
//==================================================
#define R_FF05_01                    readMatrix[5][0]
#define R_FF05_02                    readMatrix[5][1]
#define R_FF05_03                    readMatrix[5][2]
#define R_FF05_04                    readMatrix[5][3]

#define W_FF05_01                    writeMatrix[5][0]
#define W_FF05_02                    writeMatrix[5][1]
#define W_FF05_03                    writeMatrix[5][2]
#define W_FF05_04                    writeMatrix[5][3]

//==================================================
// ROW 6
//==================================================
#define R_FF06_01                    readMatrix[6][0]
#define R_FF06_02                    readMatrix[6][1]
#define R_FF06_03                    readMatrix[6][2]
#define R_FF06_04                    readMatrix[6][3]

#define W_FF06_01                    writeMatrix[6][0]
#define W_FF06_02                    writeMatrix[6][1]
#define W_FF06_03                    writeMatrix[6][2]
#define W_FF06_04                    writeMatrix[6][3]

//==================================================
// ROW 7
//==================================================
#define R_FF07_01                    readMatrix[7][0]
#define R_FF07_02                    readMatrix[7][1]
#define R_FF07_03                    readMatrix[7][2]
#define R_FF07_04                    readMatrix[7][3]

#define W_FF07_01                    writeMatrix[7][0]
#define W_FF07_02                    writeMatrix[7][1]
#define W_FF07_03                    writeMatrix[7][2]
#define W_FF07_04                    writeMatrix[7][3]

//==================================================
// ROW 8
//==================================================
#define R_FF08_01                    readMatrix[8][0]
#define R_FF08_02                    readMatrix[8][1]
#define R_FF08_03                    readMatrix[8][2]
#define R_FF08_04                    readMatrix[8][3]

#define W_FF08_01                    writeMatrix[8][0]
#define W_FF08_02                    writeMatrix[8][1]
#define W_FF08_03                    writeMatrix[8][2]
#define W_FF08_04                    writeMatrix[8][3]

//==================================================
// ROW 9
//==================================================
#define R_FF09_01                    readMatrix[9][0]
#define R_FF09_02                    readMatrix[9][1]
#define R_FF09_03                    readMatrix[9][2]
#define R_FF09_04                    readMatrix[9][3]

#define W_FF09_01                    writeMatrix[9][0]
#define W_FF09_02                    writeMatrix[9][1]
#define W_FF09_03                    writeMatrix[9][2]
#define W_FF09_04                    writeMatrix[9][3]

//==================================================
// ROW 10
//==================================================
#define R_FF0A_01                    readMatrix[10][0]
#define R_FF0A_02                    readMatrix[10][1]
#define R_FF0A_03                    readMatrix[10][2]
#define R_FF0A_04                    readMatrix[10][3]

#define W_FF0A_01                    writeMatrix[10][0]
#define W_FF0A_02                    writeMatrix[10][1]
#define W_FF0A_03                    writeMatrix[10][2]
#define W_FF0A_04                    writeMatrix[10][3]

//==================================================
// ROW 11
//==================================================
#define R_FF0B_01                    readMatrix[11][0]
#define R_FF0B_02                    readMatrix[11][1]
#define R_FF0B_03                    readMatrix[11][2]
#define R_FF0B_04                    readMatrix[11][3]

#define W_FF0B_01                    writeMatrix[11][0]
#define W_FF0B_02                    writeMatrix[11][1]
#define W_FF0B_03                    writeMatrix[11][2]
#define W_FF0B_04                    writeMatrix[11][3]

//==================================================
// ROW 12
//==================================================
#define R_FF0C_01                    readMatrix[12][0]
#define R_FF0C_02                    readMatrix[12][1]
#define R_FF0C_03                    readMatrix[12][2]
#define R_FF0C_04                    readMatrix[12][3]

#define W_FF0C_01                    writeMatrix[12][0]
#define W_FF0C_02                    writeMatrix[12][1]
#define W_FF0C_03                    writeMatrix[12][2]
#define W_FF0C_04                    writeMatrix[12][3]

//==================================================
// ROW 13
//==================================================
#define R_FF0D_01                    readMatrix[13][0]
#define R_FF0D_02                    readMatrix[13][1]
#define R_FF0D_03                    readMatrix[13][2]
#define R_FF0D_04                    readMatrix[13][3]

//==================================================
// ROW 14
//==================================================
#define R_FF0E_01                    readMatrix[14][0]
#define R_FF0E_02                    readMatrix[14][1]
#define R_FF0E_03                    readMatrix[14][2]
#define R_FF0E_04                    readMatrix[14][3]

//==================================================
// ROW 15
//==================================================
#define R_FF0F_01                    readMatrix[15][0]
#define R_FF0F_02                    readMatrix[15][1]
#define R_FF0F_03                    readMatrix[15][2]
#define R_FF0F_04                    readMatrix[15][3]
    
void Get_MAC_ID(void);
void EEPROM_Initialize(void);
void Load_Read_Matrix(void);
void Get_Config(void);
void Update_Config(void);

#ifdef __cplusplus
}
#endif

#endif /* _DEVICE_STAT_H */