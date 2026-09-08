#include "definitions.h"
#include "driver.h"
#include "middleware.h"

uint32_t devsn[4];
uint32_t devid;

uint8_t eepromBuffer[EEPROM_DATA_SIZE];

float readMatrix[ROWS][COLS];
float writeMatrix[ROWS][COLS];

static int BOOT_FLAG = 0;

#define BOOT_STAY_WINDOW_COUNT    2000000UL

void Get_PIC_ID(void)
{
    devsn[0] = DEVSN0;
    devsn[1] = DEVSN1;
    devsn[2] = DEVSN2;
    devsn[3] = DEVSN3;
    
    devid = DEVID & 0x0FFFFFFFU;
}

void EEPROM_Initialize(void) {
    FLASH_EEPROM_Init();
    Load_Read_Matrix();
}

void Load_Read_Matrix(void) {
    uint32_t temp32;
    uint32_t ctr = 0U;

    __builtin_memset(eepromBuffer, 0, EEPROM_DATA_SIZE);

    if (FLASH_EEPROM_Read_Page(eepromBuffer, sizeof (readMatrix)) ==
            FLASH_EEPROM_OK) {
        //--------------------------------------------------
        // Validate all float entries
        //--------------------------------------------------
        for (uint32_t i = 0; i < (ROWS * COLS); i++) {
            __builtin_memcpy(&temp32, &eepromBuffer[i * 4], 4);

            //--------------------------------------------------
            // Check erased flash
            //--------------------------------------------------
            if (temp32 == 0xFFFFFFFF) {
                break;
            }
        }

        //--------------------------------------------------
        // Copy to matrix only after validation
        //--------------------------------------------------
        __builtin_memcpy(readMatrix, eepromBuffer, sizeof (readMatrix));
    }

    BOOT_FLAG = R_BOOT_FLAG;

    //--------------------------------------------------
    // Copy read matrix to write matrix     
    //--------------------------------------------------
    __builtin_memcpy(writeMatrix, readMatrix, sizeof(writeMatrix));

    //--------------------------------------------------
    // Check BOOT FLAG for bootloader mode
    //--------------------------------------------------
    if (BOOT_FLAG == 1) {
        W_BOOT_FLAG = 0.0;

        //--------------------------------------------------
        // Prepare EEPROM write buffer with the complete matrix.
        //--------------------------------------------------
        __builtin_memset(eepromBuffer, 0xFF, EEPROM_DATA_SIZE);
        __builtin_memcpy(eepromBuffer, writeMatrix, sizeof (writeMatrix));

        FLASH_EEPROM_Write_Page(eepromBuffer, sizeof (writeMatrix));
    }
    else{
        /*
         * Give the host a startup window to send STAY_IN_BOOT.  Once the
         * command is received, return to main() without jumping so its
         * normal infinite state-engine loop continues servicing CAN.
         */
        while ((ctr < BOOT_STAY_WINDOW_COUNT) &&
               !Boot_Stay_In_Boot_Requested())
        {
            ctr++;
            CANbus_read();
        }

        if (!Boot_Stay_In_Boot_Requested() &&
            (Boot_Is_App_Present() == true))
        {
            Boot_App_Jump();
        }
    }
}