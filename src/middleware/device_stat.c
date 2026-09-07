#include "definitions.h"
#include "driver.h"
#include "middleware.h"

uint32_t devsn[4];

uint8_t eepromBuffer[EEPROM_DATA_SIZE];

float readMatrix[ROWS][COLS];
float writeMatrix[ROWS][COLS];

static int MCU_ID = 0;
static int BOOT_FLAG = 0;

#define BOOT_STAY_WINDOW_COUNT    2000000UL

void Get_MAC_ID(void)
{
    devsn[0] = DEVSN0;
    devsn[1] = DEVSN1;
    devsn[2] = DEVSN2;
    devsn[3] = DEVSN3;
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

    MCU_ID = R_FF01_01;
    BOOT_FLAG = R_BOOT_FLAG;

    //--------------------------------------------------
    // Check ESC ID
    //--------------------------------------------------
    if (MCU_ID <= 0) {
        Get_Config();
    }
    else{
        Update_Config();
    }

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

void Get_Config(void){
    // Row 1
    R_FF01_01 = R_FF01_01;
    R_FF01_02 = CONFIG_H_IPH;
    R_FF01_03 = CONFIG_H_RPM;
    R_FF01_04 = CONFIG_POLES;

    // Row 2
    R_FF02_01 = CONFIG_RPH;
    R_FF02_02 = CONFIG_LD;
    R_FF02_03 = CONFIG_LQ;
    R_FF02_04 = CONFIG_V_CONSTANT;

    // Row 3
    R_FF03_01 = CONFIG_ZERO_ANGLE;
    R_FF03_02 = CONFIG_SENSOR_DIRECTION;
    R_FF03_03 = CONFIG_ZERO_ANGLE_ESTIMATE;
    R_FF03_04 = CONFIG_SENSOR_DIRECTION_ESTIMATE;

    // Row 4
    R_FF04_01 = CONFIG_ROTATION_DIRECTION;
    R_FF04_02 = CONFIG_CAN_BAUD;
    R_FF04_03 = CONFIG_RPM_TO_KMPH;
    R_FF04_04 = CONFIG_RPM_FAULT;

    // Row 5
    R_FF05_01 = CONFIG_MOTOR_DERATE;
    R_FF05_02 = CONFIG_MOTOR_TFAULT;
    R_FF05_03 = CONFIG_MCU_DERATE;
    R_FF05_04 = CONFIG_MCU_TFAULT;

    // Row 6
    R_FF06_01 = CONFIG_VDC_MAX;
    R_FF06_02 = CONFIG_IDC_DEFAULT;
    R_FF06_03 = CONFIG_OV_FAULT;
    R_FF06_04 = CONFIG_UV_FAULT;

    // Row 7
    R_FF07_01 = CONFIG_IDC_PMAX;
    R_FF07_02 = CONFIG_IDC_NMAX;
    R_FF07_03 = CONFIG_IPH_FAULT;
    R_FF07_04 = CONFIG_IDC_FAULT;

    // Row 8
    R_FF08_01 = CONFIG_MOTOR_MODE;
    R_FF08_02 = CONFIG_REVERSE_RPM;
    R_FF08_03 = CONFIG_L_RPM;
    R_FF08_04 = CONFIG_M_RPM;

    // Row 9
    R_FF09_01 = CONFIG_THROTTLE_ZERO;
    R_FF09_02 = CONFIG_THROTTLE_MAX;
    R_FF09_03 = CONFIG_GEN_VLIMIT;
    R_FF09_04 = CONFIG_AUTO_GEN;

    // Row 10
    R_FF0A_01 = CONFIG_RPM_KP;
    R_FF0A_02 = CONFIG_RPM_KI;
    R_FF0A_03 = CONFIG_L_ACCELERATION;
    R_FF0A_04 = CONFIG_M_ACCELERATION;

    // Row 11
    R_FF0B_01 = CONFIG_L_IDC;
    R_FF0B_02 = CONFIG_M_IDC;
    R_FF0B_03 = CONFIG_L_IPH;
    R_FF0B_04 = CONFIG_M_IPH;

    // Row 12
    R_FF0C_01 = CONFIG_BRAKE_IPH;
    R_FF0C_02 = CONFIG_BRAKE_ITIME;
    R_FF0C_03 = CONFIG_GEN_VMARGIN;
    R_FF0C_04 = CONFIG_NA1;

    // Row 13
    R_FF0D_01 = CONFIG_IPH_MAX;
    R_FF0D_02 = CONFIG_FMOTOR_MAX;
    R_FF0D_03 = CONFIG_MOTOR_TMAX;
    R_FF0D_04 = CONFIG_MCU_TMAX;

    // Row 14
    R_FF0E_01 = CONFIG_VMAX;
    R_FF0E_02 = CONFIG_VMIN;
    R_FF0E_03 = CONFIG_IDC_MAX;
    R_FF0E_04 = CONFIG_NA2;

    // Row 15
    R_FF0F_01 = CONFIG_RPM_KP_MIN;
    R_FF0F_02 = CONFIG_RPM_KP_MAX;
    R_FF0F_03 = CONFIG_RPM_KI_MIN;
    R_FF0F_04 = CONFIG_RPM_KI_MAX;
}

void Update_Config(void){
    // Row 1
    R_FF01_01 = R_FF01_01;
    CONFIG_H_IPH = R_FF01_02;
    CONFIG_H_RPM = R_FF01_03;
    CONFIG_POLES = R_FF01_04;

    // Row 2
    CONFIG_RPH = R_FF02_01;
    CONFIG_LD = R_FF02_02;
    CONFIG_LQ = R_FF02_03;
    CONFIG_V_CONSTANT = R_FF02_04;

    // Row 3
    CONFIG_ZERO_ANGLE = R_FF03_01;
    CONFIG_SENSOR_DIRECTION = R_FF03_02;
    CONFIG_ZERO_ANGLE_ESTIMATE = R_FF03_03;
    CONFIG_SENSOR_DIRECTION_ESTIMATE = R_FF03_04;

    // Row 4
    CONFIG_ROTATION_DIRECTION = R_FF04_01;
    CONFIG_CAN_BAUD = R_FF04_02;
    CONFIG_RPM_TO_KMPH = R_FF04_03;
    CONFIG_RPM_FAULT = R_FF04_04;

    // Row 5
    CONFIG_MOTOR_DERATE = R_FF05_01;
    CONFIG_MOTOR_TFAULT = R_FF05_02;
    CONFIG_MCU_DERATE = R_FF05_03;
    CONFIG_MCU_TFAULT = R_FF05_04;

    // Row 6
    CONFIG_VDC_MAX = R_FF06_01;
    CONFIG_IDC_DEFAULT = R_FF06_02;
    CONFIG_OV_FAULT = R_FF06_03;
    CONFIG_UV_FAULT = R_FF06_04;

    // Row 7
    CONFIG_IDC_PMAX = R_FF07_01;
    CONFIG_IDC_NMAX = R_FF07_02;
    CONFIG_IPH_FAULT = R_FF07_03;
    CONFIG_IDC_FAULT = R_FF07_04;

    // Row 8
    CONFIG_MOTOR_MODE = R_FF08_01;
    CONFIG_REVERSE_RPM = R_FF08_02;
    CONFIG_L_RPM = R_FF08_03;
    CONFIG_M_RPM = R_FF08_04;

    // Row 9
    CONFIG_THROTTLE_ZERO = R_FF09_01;
    CONFIG_THROTTLE_MAX = R_FF09_02;
    CONFIG_GEN_VLIMIT = R_FF09_03;
    CONFIG_AUTO_GEN = R_FF09_04;

    // Row 10
    CONFIG_RPM_KP = R_FF0A_01;
    CONFIG_RPM_KI = R_FF0A_02;
    CONFIG_L_ACCELERATION = R_FF0A_03;
    CONFIG_M_ACCELERATION = R_FF0A_04;

    // Row 11
    CONFIG_L_IDC = R_FF0B_01;
    CONFIG_M_IDC = R_FF0B_02;
    CONFIG_L_IPH = R_FF0B_03;
    CONFIG_M_IPH = R_FF0B_04;

    // Row 12
    CONFIG_BRAKE_IPH = R_FF0C_01;
    CONFIG_BRAKE_ITIME = R_FF0C_02;
    CONFIG_GEN_VMARGIN = R_FF0C_03;
    CONFIG_NA1 = R_FF0C_04;

    // Row 13
    CONFIG_IPH_MAX = R_FF0D_01;
    CONFIG_FMOTOR_MAX = R_FF0D_02;
    CONFIG_MOTOR_TMAX = R_FF0D_03;
    CONFIG_MCU_TMAX = R_FF0D_04;

    // Row 14
    CONFIG_VMAX = R_FF0E_01;
    CONFIG_VMIN = R_FF0E_02;
    CONFIG_IDC_MAX = R_FF0E_03;
    CONFIG_NA2 = R_FF0E_04;

    // Row 15
    CONFIG_RPM_KP_MIN = R_FF0F_01;
    CONFIG_RPM_KP_MAX = R_FF0F_02;
    CONFIG_RPM_KI_MIN = R_FF0F_03;
    CONFIG_RPM_KI_MAX = R_FF0F_04;
}