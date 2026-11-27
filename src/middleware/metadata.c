#include "definitions.h"
#include "driver.h"
#include "middleware.h"

uint32_t devsn[4];
uint32_t devid;

/* Part Number */
char brandName[BRAND_NAME + 1U] = "***";
char phases[PHASES + 1U] = "*";
char switchesInParallel[SWITCHES_IN_PARALLEL + 1U] = "*";
char switchType[SWITCH_TYPE + 1U] = "*";
char pcbType[PCB_TYPE + 1U] = "*";

char partNumber[PART_NUMBER_LENGTH + 1U];

/* Model Number */
char connector[CONNECTOR + 1U] = "**";
char coms[COMS + 1U] = "**";
char sensor[SENSOR + 1U] = "*";
char ntcResistor[NTC_RESISTOR + 1U] = "***";
char ntcVolt[NTC_VOLT + 1U] = "*";
char ntcValueB[NTC_VALUE_B + 1U] = "**";

char modelNumber[MODEL_NUMBER_LENGTH + 1U];

/* Software ID */
char oemId[OEM_ID + 1U] = "**";
char projectId[PROJECT_ID + 1U] = "**";
char appVersionL[APP_VERSION_L + 1U] = "*";
char appVersionM[APP_VERSION_M + 1U] = "**";
char appVersionR[APP_VERSION_R + 1U] = "**";
char appSize[APP_SIZE + 1U] = "****";
char bootVersionL[BOOT_VERSION_L + 1U] = "1";
char bootVersionM[BOOT_VERSION_M + 1U] = "00";
char bootVersionR[BOOT_VERSION_R + 1U] = "01";
char stayInBootFlag[STAY_IN_BOOT_FLAG + 1U] = "0";

char softwareId[SOFTWARE_ID_LENGTH + 1U];

char mcuId[MCU_ID_LENGTH + 1U];
char firmwareId[FIRMWARE_ID_LENGTH + 1U];

char metadata[METADATA_LENGTH + 1U];

void Get_PIC_ID(void) {
    devsn[0] = DEVSN0;
    devsn[1] = DEVSN1;
    devsn[2] = DEVSN2;
    devsn[3] = DEVSN3;

    devid = DEVID & 0x0FFFFFFFU;
}

void Get_Part_Number(void) {
    __builtin_memset(partNumber, 0, sizeof (partNumber));

    __builtin_memcpy(&partNumber[0],
            brandName,
            BRAND_NAME);

    __builtin_memcpy(&partNumber[BRAND_NAME],
            phases,
            PHASES);

    __builtin_memcpy(&partNumber[BRAND_NAME + PHASES],
            switchesInParallel,
            SWITCHES_IN_PARALLEL);

    __builtin_memcpy(&partNumber[BRAND_NAME + PHASES + SWITCHES_IN_PARALLEL],
            switchType,
            SWITCH_TYPE);

    __builtin_memcpy(&partNumber[BRAND_NAME + PHASES +
            SWITCHES_IN_PARALLEL + SWITCH_TYPE],
            pcbType,
            PCB_TYPE);

    partNumber[PART_NUMBER_LENGTH] = '\0';
}

void Get_Model_Number(void) {
    __builtin_memset(modelNumber, 0, sizeof (modelNumber));

    __builtin_memcpy(&modelNumber[0], connector, CONNECTOR);

    __builtin_memcpy(&modelNumber[CONNECTOR], coms, COMS);

    __builtin_memcpy(&modelNumber[CONNECTOR + COMS], sensor, SENSOR);

    __builtin_memcpy(&modelNumber[CONNECTOR + COMS + SENSOR],
            ntcResistor, NTC_RESISTOR);

    __builtin_memcpy(&modelNumber[CONNECTOR + COMS + SENSOR +
            NTC_RESISTOR], ntcVolt, NTC_VOLT);

    __builtin_memcpy(&modelNumber[CONNECTOR + COMS + SENSOR +
            NTC_RESISTOR + NTC_VOLT], ntcValueB, NTC_VALUE_B);

    modelNumber[MODEL_NUMBER_LENGTH] = '\0';
}

void Get_Software_ID(void) {
    __builtin_memset(softwareId, 0, sizeof (softwareId));

    __builtin_memcpy(&softwareId[0],
            oemId, OEM_ID);

    __builtin_memcpy(&softwareId[OEM_ID],
            projectId, PROJECT_ID);

    __builtin_memcpy(&softwareId[OEM_ID + PROJECT_ID],
            appVersionL, APP_VERSION_L);

    __builtin_memcpy(&softwareId[OEM_ID + PROJECT_ID + APP_VERSION_L],
            appVersionM, APP_VERSION_M);

    __builtin_memcpy(&softwareId[OEM_ID + PROJECT_ID + APP_VERSION_L +
            APP_VERSION_M],
            appVersionR, APP_VERSION_R);

    __builtin_memcpy(&softwareId[OEM_ID + PROJECT_ID + APP_VERSION_L +
            APP_VERSION_M + APP_VERSION_R],
            appSize, APP_SIZE);

    __builtin_memcpy(&softwareId[OEM_ID + PROJECT_ID + APP_VERSION_L +
            APP_VERSION_M + APP_VERSION_R + APP_SIZE],
            bootVersionL, BOOT_VERSION_L);

    __builtin_memcpy(&softwareId[OEM_ID + PROJECT_ID + APP_VERSION_L +
            APP_VERSION_M + APP_VERSION_R + APP_SIZE +
            BOOT_VERSION_L],
            bootVersionM, BOOT_VERSION_M);

    __builtin_memcpy(&softwareId[OEM_ID + PROJECT_ID + APP_VERSION_L +
            APP_VERSION_M + APP_VERSION_R + APP_SIZE +
            BOOT_VERSION_L + BOOT_VERSION_M],
            bootVersionR, BOOT_VERSION_R);

    __builtin_memcpy(&softwareId[SOFTWARE_ID_LENGTH - STAY_IN_BOOT_FLAG],
            stayInBootFlag, STAY_IN_BOOT_FLAG);

    softwareId[SOFTWARE_ID_LENGTH] = '\0';
}

void Get_Firmware_ID(void) {
    __builtin_memset(firmwareId, 0, sizeof (firmwareId));

    __builtin_memcpy(&firmwareId[0],
            partNumber, PART_NUMBER_LENGTH);

    __builtin_memcpy(&firmwareId[PART_NUMBER_LENGTH],
            modelNumber, MODEL_NUMBER_LENGTH);

    __builtin_memcpy(&firmwareId[PART_NUMBER_LENGTH + MODEL_NUMBER_LENGTH],
            softwareId, SOFTWARE_ID_LENGTH);

    firmwareId[FIRMWARE_ID_LENGTH] = '\0';
}

void Build_Metadata(void) {
    __builtin_memset(metadata, 0, sizeof (metadata));

    __builtin_memcpy(&metadata[0],
            mcuId, MCU_ID_LENGTH);

    __builtin_memcpy(&metadata[MCU_ID_LENGTH],
            firmwareId, FIRMWARE_ID_LENGTH);

    metadata[METADATA_LENGTH] = '\0';
}

void Metadata_Init(void) {
    bool mcuIdNeedsDefault;
    bool firmwareIdValid = true;
    bool metadataChanged = false;
    bool allZero = true;
    bool allFF = true;
    bool allPrintable = true;
    size_t i;

    Get_PIC_ID();

    __builtin_memcpy(metadata,
            &eepromBuffer[EEPROM_METADATA_OFFSET],
            METADATA_LENGTH);

    metadata[METADATA_LENGTH] = '\0';

    /* An all-star MCU ID is a valid "not provisioned" placeholder. */
    for (i = 0U; i < MCU_ID_LENGTH; i++) {
        uint8_t value = (uint8_t) metadata[MCU_ID_OFFSET + i];

        if (value != 0x00U) {
            allZero = false;
        }

        if (value != 0xFFU) {
            allFF = false;
        }

        if ((value < 0x20U) || (value > 0x7EU)) {
            allPrintable = false;
        }
    }

    mcuIdNeedsDefault = allZero || allFF || !allPrintable;

    if (mcuIdNeedsDefault) {
        __builtin_memset(mcuId, '*', MCU_ID_LENGTH);
        mcuId[MCU_ID_LENGTH] = '\0';
    } else {
        __builtin_memcpy(mcuId,
                &metadata[MCU_ID_OFFSET],
                MCU_ID_LENGTH);
        mcuId[MCU_ID_LENGTH] = '\0';
    }

    allZero = true;
    allFF = true;

    for (i = 0U; i < FIRMWARE_ID_LENGTH; i++) {
        uint8_t value = (uint8_t) metadata[FIRMWARE_ID_OFFSET + i];

        if (value != 0x00U) {
            allZero = false;
        }

        if (value != 0xFFU) {
            allFF = false;
        }
    }

    if (allZero || allFF) {
        firmwareIdValid = false;
    }

    if (!firmwareIdValid) {
        Get_Part_Number();
        Get_Model_Number();
        Get_Software_ID();
        Get_Firmware_ID();
        Build_Metadata();
        metadataChanged = true;
    } else {
        __builtin_memcpy(brandName, &metadata[BRAND_NAME_OFFSET], BRAND_NAME);
        __builtin_memcpy(phases, &metadata[PHASES_OFFSET], PHASES);
        __builtin_memcpy(switchesInParallel,
                &metadata[SWITCHES_IN_PARALLEL_OFFSET], SWITCHES_IN_PARALLEL);
        __builtin_memcpy(switchType, &metadata[SWITCH_TYPE_OFFSET], SWITCH_TYPE);
        __builtin_memcpy(pcbType, &metadata[PCB_TYPE_OFFSET], PCB_TYPE);

        __builtin_memcpy(sensor, &metadata[SENSOR_OFFSET], SENSOR);
        __builtin_memcpy(ntcResistor,
                &metadata[NTC_RESISTOR_OFFSET], NTC_RESISTOR);
        __builtin_memcpy(ntcVolt, &metadata[NTC_VOLT_OFFSET], NTC_VOLT);
        __builtin_memcpy(ntcValueB, &metadata[NTC_VALUE_B_OFFSET], NTC_VALUE_B);
        __builtin_memcpy(connector, &metadata[CONNECTOR_OFFSET], CONNECTOR);
        __builtin_memcpy(coms, &metadata[COMS_OFFSET], COMS);

        __builtin_memcpy(oemId, &metadata[OEM_ID_OFFSET], OEM_ID);
        __builtin_memcpy(projectId, &metadata[PROJECT_ID_OFFSET], PROJECT_ID);
        __builtin_memcpy(appVersionL,
                &metadata[APP_VERSION_L_OFFSET], APP_VERSION_L);
        __builtin_memcpy(appVersionM,
                &metadata[APP_VERSION_M_OFFSET], APP_VERSION_M);
        __builtin_memcpy(appVersionR,
                &metadata[APP_VERSION_R_OFFSET], APP_VERSION_R);
        __builtin_memcpy(appSize, &metadata[APP_SIZE_OFFSET], APP_SIZE);
        appSize[APP_SIZE] = '\0';

        __builtin_memcpy(stayInBootFlag,
                &metadata[STAY_IN_BOOT_FLAG_OFFSET], STAY_IN_BOOT_FLAG);
        stayInBootFlag[STAY_IN_BOOT_FLAG] = '\0';

        Get_Part_Number();
        Get_Model_Number();
        Get_Software_ID();
        Get_Firmware_ID();

        if (mcuIdNeedsDefault ||
                (__builtin_memcmp(firmwareId,
                &metadata[FIRMWARE_ID_OFFSET],
                FIRMWARE_ID_LENGTH) != 0)) {
            Build_Metadata();
            metadataChanged = true;
        }
    }

    if (metadataChanged) {
        __builtin_memset(&eepromBuffer[EEPROM_METADATA_OFFSET], 0xFF,
                EEPROM_METADATA_SIZE);

        __builtin_memcpy(&eepromBuffer[EEPROM_METADATA_OFFSET],
                metadata,
                METADATA_LENGTH);

        Flash_EEPROM_Write_Page(eepromBuffer,
                EEPROM_DATA_SIZE);
    }
}