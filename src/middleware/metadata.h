#ifndef _METADATA_H    
#define _METADATA_H 

#include "definitions.h"

//--------------------------------------------------
// METADATA FIELD LENGTH
//--------------------------------------------------
#define BRAND_NAME                      3U
#define PHASES                          1U
#define SWITCHES_IN_PARALLEL            1U
#define SWITCH_TYPE                     1U
#define PCB_TYPE                        1U

#define PART_NUMBER_LENGTH              (BRAND_NAME + PHASES + \
                                        SWITCHES_IN_PARALLEL + \
                                        SWITCH_TYPE + PCB_TYPE)

#define CONNECTOR                       2U
#define COMS                            2U
#define SENSOR                          1U
#define NTC_RESISTOR                    3U
#define NTC_VOLT                        1U
#define NTC_VALUE_B                     2U

#define MODEL_NUMBER_LENGTH             (CONNECTOR + COMS + SENSOR + \
                                        NTC_RESISTOR + NTC_VOLT + \
                                        NTC_VALUE_B)

#define EMS                             2U
#define YEAR_MONTH                      4U
#define BATCH_NUMBER                    4U
#define SEQUENCE_NUMBER                 6U

#define SERIAL_NUMBER_LENGTH            (EMS + YEAR_MONTH + \
                                        BATCH_NUMBER + SEQUENCE_NUMBER)

#define OEM_ID                          2U
#define PROJECT_ID                      2U
#define APP_VERSION_L                   1U
#define APP_VERSION_M                   2U
#define APP_VERSION_R                   2U
#define APP_SIZE                        4U
#define BOOT_VERSION_L                  1U
#define BOOT_VERSION_M                  2U
#define BOOT_VERSION_R                  2U
#define STAY_IN_BOOT_FLAG               1U

#define SOFTWARE_ID_LENGTH              (OEM_ID + PROJECT_ID + \
                                        APP_VERSION_L + APP_VERSION_M + \
                                        APP_VERSION_R + APP_SIZE + \
                                        BOOT_VERSION_L + BOOT_VERSION_M + \
                                        BOOT_VERSION_R + STAY_IN_BOOT_FLAG)

#define MCU_ID_LENGTH                   (PART_NUMBER_LENGTH + \
                                        MODEL_NUMBER_LENGTH + \
                                        SERIAL_NUMBER_LENGTH)

#define FIRMWARE_ID_LENGTH              (PART_NUMBER_LENGTH + \
                                        MODEL_NUMBER_LENGTH + \
                                        SOFTWARE_ID_LENGTH)

#define METADATA_LENGTH                 (MCU_ID_LENGTH + FIRMWARE_ID_LENGTH)


//--------------------------------------------------
// METADATA FIELD OFFSET
//--------------------------------------------------
#define MCU_ID_OFFSET                   0U
#define FIRMWARE_ID_OFFSET              (MCU_ID_LENGTH)

//--------------------------------------------------
// FIRMWARE ID FIELD OFFSET
//--------------------------------------------------
#define BRAND_NAME_OFFSET               (FIRMWARE_ID_OFFSET)
#define PHASES_OFFSET                   (BRAND_NAME_OFFSET + BRAND_NAME)
#define SWITCHES_IN_PARALLEL_OFFSET     (PHASES_OFFSET + PHASES)
#define SWITCH_TYPE_OFFSET              (SWITCHES_IN_PARALLEL_OFFSET + \
                                         SWITCHES_IN_PARALLEL)
#define PCB_TYPE_OFFSET                 (SWITCH_TYPE_OFFSET + SWITCH_TYPE)

#define CONNECTOR_OFFSET                (PCB_TYPE_OFFSET + PCB_TYPE)
#define COMS_OFFSET                     (CONNECTOR_OFFSET + CONNECTOR)
#define SENSOR_OFFSET                   (COMS_OFFSET + COMS)
#define NTC_RESISTOR_OFFSET             (SENSOR_OFFSET + SENSOR)
#define NTC_VOLT_OFFSET                 (NTC_RESISTOR_OFFSET + NTC_RESISTOR)
#define NTC_VALUE_B_OFFSET              (NTC_VOLT_OFFSET + NTC_VOLT)

#define OEM_ID_OFFSET                   (NTC_VALUE_B_OFFSET + NTC_VALUE_B)
#define PROJECT_ID_OFFSET               (OEM_ID_OFFSET + OEM_ID)
#define APP_VERSION_L_OFFSET            (PROJECT_ID_OFFSET + PROJECT_ID)
#define APP_VERSION_M_OFFSET            (APP_VERSION_L_OFFSET + APP_VERSION_L)
#define APP_VERSION_R_OFFSET            (APP_VERSION_M_OFFSET + APP_VERSION_M)
#define APP_SIZE_OFFSET                 (APP_VERSION_R_OFFSET + APP_VERSION_R)
#define BOOT_VERSION_L_OFFSET           (APP_SIZE_OFFSET + APP_SIZE)
#define BOOT_VERSION_M_OFFSET           (BOOT_VERSION_L_OFFSET + BOOT_VERSION_L)
#define BOOT_VERSION_R_OFFSET           (BOOT_VERSION_M_OFFSET + BOOT_VERSION_M)
#define STAY_IN_BOOT_FLAG_OFFSET        (BOOT_VERSION_R_OFFSET + BOOT_VERSION_R)
    
    
//--------------------------------------------------
// EEPROM FIELD OFFSET
//-------------------------------------------------- 
#define EEPROM_METADATA_OFFSET          0UL
#define EEPROM_METADATA_SIZE            (EEPROM_DATA_SIZE / 2UL)


//--------------------------------------------------
// EXTERN VARIABLES
//--------------------------------------------------
extern uint32_t devsn[4];
extern uint32_t devid;    

extern char appSize[APP_SIZE + 1U];
extern char stayInBootFlag[STAY_IN_BOOT_FLAG + 1U];

extern char mcuId[MCU_ID_LENGTH + 1U];

extern char metadata[METADATA_LENGTH + 1U];  

    
//--------------------------------------------------
// API
//--------------------------------------------------
void Get_PIC_ID(void);
void Get_Part_Number(void);
void Get_Model_Number(void);
void Get_Software_ID(void);
void Get_Firmware_ID(void);
void Build_Metadata(void);
void Metadata_Init(void);


#endif