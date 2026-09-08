#include "definitions.h"
#include "driver.h"
#include "middleware.h"

uint32_t devsn[4];
uint32_t devid;

uint8_t eepromBuffer[EEPROM_DATA_SIZE];

float readMatrix[ROWS][COLS];
float writeMatrix[ROWS][COLS];

static int bootFlag = 0;
static float bootVersion = 1.0;

#define BOOT_STAY_WINDOW_COUNT    2000000UL

void Get_PIC_ID(void) {
    devsn[0] = DEVSN0;
    devsn[1] = DEVSN1;
    devsn[2] = DEVSN2;
    devsn[3] = DEVSN3;

    devid = DEVID & 0x0FFFFFFFU;
}

void EEPROM_Initialize(void) {
    FLASH_EEPROM_Init();
    Load_Matrix();
}

void Load_Matrix(void) {
    uint32_t ctr = 0U;

    __builtin_memset(eepromBuffer, 0, EEPROM_DATA_SIZE);
    __builtin_memset(readMatrix, 0, sizeof (readMatrix));
    __builtin_memset(writeMatrix, 0, sizeof (writeMatrix));

    FLASH_EEPROM_Read_Page(eepromBuffer, sizeof (readMatrix));

    //--------------------------------------------------
    // Copy eeprom buffer to read matrix
    //--------------------------------------------------
    __builtin_memcpy(readMatrix, eepromBuffer, sizeof (readMatrix));
    R_BOOT_VERSION = bootVersion;
    
    //--------------------------------------------------
    // Copy read matrix to write matrix     
    //--------------------------------------------------
    __builtin_memcpy(writeMatrix, readMatrix, sizeof (writeMatrix));

    bootFlag = R_BOOT_FLAG;

    //--------------------------------------------------
    // Check BOOT FLAG for bootloader mode
    //--------------------------------------------------
    if (bootFlag == 1) {
        W_BOOT_FLAG = 0.0;

        //--------------------------------------------------
        // Prepare EEPROM write buffer with the complete matrix.
        //--------------------------------------------------
        __builtin_memset(eepromBuffer, 0xFF, EEPROM_DATA_SIZE);
        __builtin_memcpy(eepromBuffer, writeMatrix, sizeof (writeMatrix));

        FLASH_EEPROM_Write_Page(eepromBuffer, sizeof (writeMatrix));
    } else {
        /*
         * Give the host a startup window to send STAY_IN_BOOT.  Once the
         * command is received, return to main() without jumping so its
         * normal infinite state-engine loop continues servicing CAN.
         */
        while ((ctr < BOOT_STAY_WINDOW_COUNT) &&
                !Boot_Stay_In_Boot_Requested()) {
            ctr++;
            CANbus_read();
        }

        if (!Boot_Stay_In_Boot_Requested() &&
                (Boot_Is_App_Present() == true)) {
            Boot_App_Jump();
        }
    }
}