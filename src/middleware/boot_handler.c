#include "sys/kmem.h"
#include "definitions.h"
#include "driver.h"
#include "middleware.h"

#define ERASE_BLOCK_SIZE        (4096UL)
#define FLASH_CHUNK_SIZE        1024U
#define FLASH_ROW_SIZE          (NVM_FLASH_ROWSIZE)
#define BOOT_FLASH_PAGE_SIZE    (NVM_FLASH_PAGESIZE * sizeof(uint32_t))
#define CAN_DATA_SIZE           4U

static uint8_t flashBuffer[FLASH_CHUNK_SIZE];
static uint32_t flashBufferIndex = 0;
static uint32_t flashWriteAddress = APPLICATION_START_ADDRESS;
static bool firmwareUpdateActive = false;
static bool firmwareUpdateFailed = false;
static bool firmwareImageReady = false;
static bool firmwareCrcVerified = false;
static volatile bool bootStayRequested = false;
static uint32_t firmwareExpectedSize = 0U;
static uint8_t expectedChunkIndex = 1U;
static uint8_t lastAcceptedChunkIndex = 0U;

static uint8_t canRxBuffer[8] = {0U};

void Boot_PIC_ID_Handler(void) {
    uint8_t txBuf[8];
    uint32_t udidValue;

    if (canRxBuffer[0] != BOOT_CAN_CMD_0_PIC_ID) {
        return;
    }

    switch (canRxBuffer[2]) {
            /*------------------------- READ --------------------------*/
        case BOOT_CAN_CMD_2_READ:

            txBuf[0] = BOOT_CAN_CMD_0_PIC_ID;
            txBuf[1] = canRxBuffer[1];
            txBuf[2] = BOOT_CAN_CMD_2_READ;

            /* Check if PIC ID is available */
            if ((devsn[0] == 0U) &&
                    (devsn[1] == 0U) &&
                    (devsn[2] == 0U) &&
                    (devsn[3] == 0U)) {
                txBuf[3] = BOOT_CAN_CMD_3_FAIL;
            } else {
                txBuf[3] = BOOT_CAN_CMD_3_SUCCESS;
            }

            switch (canRxBuffer[1]) {
                case BOOT_CAN_CMD_1_PIC_ID_1:
                    udidValue = devsn[0];
                    break;

                case BOOT_CAN_CMD_1_PIC_ID_2:
                    udidValue = devsn[1];
                    break;

                case BOOT_CAN_CMD_1_PIC_ID_3:
                    udidValue = devsn[2];
                    break;

                case BOOT_CAN_CMD_1_PIC_ID_4:
                    udidValue = devsn[3];
                    break;

                default:
                    return;
            }

            txBuf[4] = (uint8_t) (udidValue >> 24);
            txBuf[5] = (uint8_t) (udidValue >> 16);
            txBuf[6] = (uint8_t) (udidValue >> 8);
            txBuf[7] = (uint8_t) (udidValue);

            CANbus_write(BOOT_CAN_ID_CLIENT, txBuf, sizeof (txBuf));
            break;

        default:
            break;
    }
}

void Boot_Software_ID_Handler(void) {
    uint8_t txBuf[8];
    float softwareId;
    bool commitFirmwareUpdate;

    if (canRxBuffer[0] != BOOT_CAN_CMD_0_SOFTWARE_ID) {
        return;
    }

    switch (canRxBuffer[2]) {
            /*------------------------- READ --------------------------*/
        case BOOT_CAN_CMD_2_READ:

            txBuf[0] = BOOT_CAN_CMD_0_SOFTWARE_ID;
            txBuf[1] = canRxBuffer[1];
            txBuf[2] = BOOT_CAN_CMD_2_READ;

            switch (canRxBuffer[1]) {
                case BOOT_CAN_CMD_1_PROJECT_ID:
                    softwareId = R_PROJECT_ID;
                    break;

                case BOOT_CAN_CMD_1_FIRMWARE_ID:
                    softwareId = R_FIRMWARE_ID;
                    break;

                default:
                    return;
            }

            /* Validate Software ID */
            if (softwareId < 0.0f) {
                txBuf[3] = BOOT_CAN_CMD_3_FAIL;
            } else {
                txBuf[3] = BOOT_CAN_CMD_3_SUCCESS;
            }

            __builtin_memcpy(&txBuf[4], &softwareId, sizeof (float));

            CANbus_write(BOOT_CAN_ID_CLIENT, txBuf, sizeof (txBuf));
            break;

            /*------------------------- WRITE -------------------------*/
        case BOOT_CAN_CMD_2_WRITE:

            /*
             * The firmware-ID WRITE replaces the former CANID_REQ_END frame.
             * It is accepted only after all application data has been written
             * successfully.  A partial buffer is flushed here as a safeguard.
             */
            commitFirmwareUpdate =
                    ((canRxBuffer[1] == BOOT_CAN_CMD_1_FIRMWARE_ID) &&
                    firmwareUpdateActive);

            if (commitFirmwareUpdate) {
                if (!firmwareUpdateFailed && (flashBufferIndex > 0U) &&
                        !App_Firmware_Write_Flash()) {
                    firmwareUpdateFailed = true;
                }

                if (firmwareUpdateFailed || !firmwareCrcVerified) {
                    txBuf[0] = BOOT_CAN_CMD_0_SOFTWARE_ID;
                    txBuf[1] = canRxBuffer[1];
                    txBuf[2] = BOOT_CAN_CMD_2_WRITE;
                    txBuf[3] = BOOT_CAN_CMD_3_FAIL;
                    CANbus_write(BOOT_CAN_ID_CLIENT, txBuf, sizeof (txBuf));
                    return;
                }
            }

            txBuf[0] = BOOT_CAN_CMD_0_SOFTWARE_ID;
            txBuf[1] = canRxBuffer[1];
            txBuf[2] = BOOT_CAN_CMD_2_WRITE;
            txBuf[3] = 0x00;

            switch (canRxBuffer[1]) {
                case BOOT_CAN_CMD_1_PROJECT_ID:
                    __builtin_memcpy(&W_PROJECT_ID, &canRxBuffer[4], sizeof (float));
                    break;

                case BOOT_CAN_CMD_1_FIRMWARE_ID:
                    __builtin_memcpy(&W_FIRMWARE_ID, &canRxBuffer[4], sizeof (float));
                    break;

                default:
                    return;
            }

            __builtin_memset(eepromBuffer, 0xFF, EEPROM_DATA_SIZE);
            __builtin_memcpy(eepromBuffer, writeMatrix, sizeof (writeMatrix));

            if (FLASH_EEPROM_Write_Page(eepromBuffer, sizeof (writeMatrix)) == FLASH_EEPROM_OK) {
                txBuf[3] = BOOT_CAN_CMD_3_SUCCESS;

                /* Commit the application only after its firmware ID is saved. */
                if (commitFirmwareUpdate) {
                    firmwareImageReady = true;
                    firmwareUpdateActive = false;
                }
            } else {
                txBuf[3] = BOOT_CAN_CMD_3_FAIL;
            }

            CANbus_write(BOOT_CAN_ID_CLIENT, txBuf, sizeof (txBuf));
            break;

        default:
            break;
    }
}

void Boot_Status_Handler(void) {
    uint8_t txBuf[8] = {0};
    uint32_t addr;

    if (canRxBuffer[0] != BOOT_CAN_CMD_0_STATUS) {
        return;
    }

    switch (canRxBuffer[2]) {
            /*------------------------- READ --------------------------*/
        case BOOT_CAN_CMD_2_READ:

            /* Get the address of this function */
            /* Function pointers on PIC32MK are KSEG virtual addresses. */
            addr = (uint32_t) (uintptr_t) & Boot_Status_Handler;

            txBuf[0] = BOOT_CAN_CMD_0_STATUS;
            txBuf[2] = BOOT_CAN_CMD_2_READ;

            if ((addr >= BOOTLOADER_START_ADDRESS) &&
                    (addr <= BOOTLOADER_END_ADDRESS)) {
                txBuf[1] = BOOT_CAN_CMD_1_STATUS_BOOT;
            } else if ((addr >= APPLICATION_START_ADDRESS) &&
                    (addr <= APPLICATION_END_ADDRESS)) {
                txBuf[1] = BOOT_CAN_CMD_1_STATUS_APP;
            } else {
                txBuf[1] = BOOT_CAN_CMD_1_STATUS_UNKNOWN;
            }

            CANbus_write(BOOT_CAN_ID_CLIENT, txBuf, sizeof (txBuf));
            break;

        default:
            break;
    }
}

void Boot_App_Size_Handler(void) {
    uint8_t txBuf[8] = {0};
    uint32_t firmwareSize;
    uint32_t maxAppSize;

    if (canRxBuffer[0] != BOOT_CAN_CMD_0_APP_SIZE) {
        return;
    }

    switch (canRxBuffer[2]) {
            /*----------------------- COMPARE -------------------------*/
        case BOOT_CAN_CMD_2_COMPARE:

            /* Extract firmware size (bytes) from CAN bytes 4-7 */
            firmwareSize = ((uint32_t) canRxBuffer[4] << 24) |
                    ((uint32_t) canRxBuffer[5] << 16) |
                    ((uint32_t) canRxBuffer[6] << 8) |
                    ((uint32_t) canRxBuffer[7]);

            /* Maximum allowed firmware size = Application partition - 4 KB */
            maxAppSize = (APPLICATION_END_ADDRESS - APPLICATION_START_ADDRESS + 1U) - (1U * 1024U);

            txBuf[0] = BOOT_CAN_CMD_0_APP_SIZE;
            txBuf[2] = BOOT_CAN_CMD_2_COMPARE;
            firmwareExpectedSize = 0U;

            if ((firmwareSize > 0U) && (firmwareSize < maxAppSize)) {
                /* CRC covers only this exact file size, never CAN padding. */
                firmwareExpectedSize = firmwareSize;
                txBuf[3] = BOOT_CAN_CMD_3_SUCCESS;
            } else {
                txBuf[3] = BOOT_CAN_CMD_3_FAIL;
            }

            CANbus_write(BOOT_CAN_ID_CLIENT, txBuf, sizeof (txBuf));
            break;

            /*------------------------- READ --------------------------*/
        case BOOT_CAN_CMD_2_READ:

            firmwareSize = (uint32_t) R_APP_SIZE;

            txBuf[0] = BOOT_CAN_CMD_0_APP_SIZE;
            txBuf[2] = BOOT_CAN_CMD_2_READ;
            txBuf[3] = BOOT_CAN_CMD_3_SUCCESS;

            txBuf[4] = (uint8_t) (firmwareSize >> 24);
            txBuf[5] = (uint8_t) (firmwareSize >> 16);
            txBuf[6] = (uint8_t) (firmwareSize >> 8);
            txBuf[7] = (uint8_t) (firmwareSize);

            CANbus_write(BOOT_CAN_ID_CLIENT, txBuf, sizeof (txBuf));
            break;

            /*------------------------- WRITE -------------------------*/
        case BOOT_CAN_CMD_2_WRITE:

            W_APP_SIZE = ((uint32_t) canRxBuffer[4] << 24) |
                    ((uint32_t) canRxBuffer[5] << 16) |
                    ((uint32_t) canRxBuffer[6] << 8) |
                    ((uint32_t) canRxBuffer[7]);

            memset(eepromBuffer, 0xFF, EEPROM_DATA_SIZE);
            memcpy(eepromBuffer, writeMatrix, sizeof (writeMatrix));

            txBuf[0] = BOOT_CAN_CMD_0_APP_SIZE;
            txBuf[2] = BOOT_CAN_CMD_2_WRITE;

            if (FLASH_EEPROM_Write_Page(eepromBuffer, sizeof (writeMatrix)) == FLASH_EEPROM_OK) {
                txBuf[3] = BOOT_CAN_CMD_3_SUCCESS;
            } else {
                txBuf[3] = BOOT_CAN_CMD_3_FAIL;
            }

            CANbus_write(BOOT_CAN_ID_CLIENT, txBuf, sizeof (txBuf));
            break;

        default:
            break;
    }
}

bool Boot_Is_App_Present(void) {
    uint32_t jumpAddrVal;

    jumpAddrVal = *(uint32_t *) (APPLICATION_JUMP_ADDRESS + 4U);

    if (jumpAddrVal == 0xFFFFFFFFU) {
        return false;
    }

    return true;
}

void Boot_App_Jump(void) {
    if (!Boot_Is_App_Present()) {
        RED_LED_Set();
        return;
    }

    __builtin_disable_interrupts();

    void (*fptr)(void) = (void (*)(void))(APPLICATION_JUMP_ADDRESS + 4U);

    fptr();

    while (1) {
        /* Should never return */
    }
}

void Boot_Jump_Handler(void) {
    uint8_t txBuf[8] = {0};
    uint32_t addr;

    if (canRxBuffer[0] != BOOT_CAN_CMD_0_JUMP) {
        return;
    }

    addr = (uint32_t) & Boot_Jump_Handler;

    txBuf[0] = BOOT_CAN_CMD_0_JUMP;
    txBuf[1] = canRxBuffer[1];

    switch (canRxBuffer[1]) {
        case BOOT_CAN_CMD_1_BOOT_JUMP:

            /* Already running in Bootloader */
            if ((addr >= BOOTLOADER_START_ADDRESS) &&
                    (addr <= BOOTLOADER_END_ADDRESS)) {
                txBuf[3] = BOOT_CAN_CMD_3_SUCCESS;
                CANbus_write(BOOT_CAN_ID_CLIENT, txBuf, sizeof (txBuf));
                break;
            }

            /* Set boot flag and reset */
            W_BOOT_FLAG = 1.0f;

            __builtin_memset(eepromBuffer, 0xFF, EEPROM_DATA_SIZE);
            __builtin_memcpy(eepromBuffer, writeMatrix, sizeof (writeMatrix));

            if (FLASH_EEPROM_Write_Page(eepromBuffer,
                    sizeof (writeMatrix)) == FLASH_EEPROM_OK) {
                /* Perform system unlock sequence */
                SYSKEY = 0x00000000U;
                SYSKEY = 0xAA996655U;
                SYSKEY = 0x556699AAU;

                RSWRSTSET = _RSWRST_SWRST_MASK;
                (void) RSWRST;
            }
            break;

        case BOOT_CAN_CMD_1_APP_JUMP:
            /* Already running in Application */
            if ((addr >= APPLICATION_START_ADDRESS) &&
                    (addr <= APPLICATION_END_ADDRESS)) {
                txBuf[3] = BOOT_CAN_CMD_3_SUCCESS;
                CANbus_write(BOOT_CAN_ID_CLIENT, txBuf, sizeof (txBuf));
                break;
            }

            /* Safety fallback: never accept a received image without CRC. */
            if (firmwareUpdateActive && !firmwareUpdateFailed &&
                    (flashBufferIndex > 0U)) {
                if (!App_Firmware_Write_Flash()) {
                    firmwareUpdateFailed = true;
                }
            }

            if (firmwareUpdateActive) {
                firmwareImageReady = firmwareCrcVerified && !firmwareUpdateFailed;
                firmwareUpdateActive = false;

                if (!firmwareImageReady) {
                    txBuf[3] = BOOT_CAN_CMD_3_FAIL;
                    CANbus_write(BOOT_CAN_ID_CLIENT, txBuf, sizeof (txBuf));
                    break;
                }
            }

            if (!firmwareUpdateFailed && (Boot_Is_App_Present() == true)) {
                txBuf[3] = BOOT_CAN_CMD_3_SUCCESS;
                CANbus_write(BOOT_CAN_ID_CLIENT, txBuf, sizeof (txBuf));
                Boot_App_Jump();
            } else {
                txBuf[3] = BOOT_CAN_CMD_3_FAIL;
                CANbus_write(BOOT_CAN_ID_CLIENT, txBuf, sizeof (txBuf));
            }
            break;

        default:
            break;
    }
}

void Boot_App_Erase_Handler(void) {
    uint8_t txBuf[8] = {0};
    uint32_t address;

    flashWriteAddress = APPLICATION_START_ADDRESS;
    flashBufferIndex = 0;
    firmwareUpdateActive = false;
    firmwareUpdateFailed = false;
    firmwareImageReady = false;
    firmwareCrcVerified = false;
    expectedChunkIndex = 1U;
    lastAcceptedChunkIndex = 0U;

    __builtin_memset(flashBuffer, 0xFF, sizeof (flashBuffer));

    if (canRxBuffer[0] != BOOT_CAN_CMD_0_APP_ERASE) {
        return;
    }

    txBuf[0] = BOOT_CAN_CMD_0_APP_ERASE;

    /* Erase complete application region */
    for (address = APPLICATION_START_ADDRESS;
            address < APPLICATION_END_ADDRESS;
            address += ERASE_BLOCK_SIZE) {
        if ((address % ERASE_BLOCK_SIZE) == 0) {
            if (!NVM_PageErase(address)) {
                txBuf[3] = BOOT_CAN_CMD_3_FAIL;
                CANbus_write(BOOT_CAN_ID_CLIENT, txBuf, sizeof (txBuf));
                return;
            }

            while (NVM_IsBusy()) {
            }

            if (NVM_ErrorGet() != NVM_ERROR_NONE) {
                txBuf[3] = BOOT_CAN_CMD_3_FAIL;
                CANbus_write(BOOT_CAN_ID_CLIENT, txBuf, sizeof (txBuf));
                return;
            }
        }
    }

    firmwareUpdateActive = true;

    /* Erase successful */
    txBuf[3] = BOOT_CAN_CMD_3_SUCCESS;
    CANbus_write(BOOT_CAN_ID_CLIENT, txBuf, sizeof (txBuf));
}

/* PIC32MK program-flash row programming (NVM_FLASH_ROWSIZE is bytes). */
static bool App_Firmware_Write_Flash(void) {
    uint32_t rowAddr = flashWriteAddress;
    uint32_t offset;
    /* The PIC32MK row API consumes exactly 512 source bytes.  Keep this as a
     * byte buffer (as in the working automatic bootloader) so CAN/file byte
     * order is preserved, and align it for the NVM/DMA source address. */
    static uint8_t rowData[NVM_FLASH_ROWSIZE]
            __attribute__((aligned(16)));

    for (offset = 0U; offset < flashBufferIndex; offset += FLASH_ROW_SIZE) {
        uint32_t rowBytes = flashBufferIndex - offset;

        if (rowBytes > FLASH_ROW_SIZE) {
            rowBytes = FLASH_ROW_SIZE;
        }

        if ((rowAddr < APPLICATION_START_ADDRESS) ||
                (rowAddr > (APPLICATION_END_ADDRESS - (FLASH_ROW_SIZE - 1U)))) {
            return false;
        }

        __builtin_memset(rowData, 0xFF, sizeof (rowData));
        (void) memcpy(rowData, &flashBuffer[offset], rowBytes);

        if (!NVM_RowWrite((uint32_t *) (void *) rowData, rowAddr)) {
            return false;
        }

        while (NVM_IsBusy()) {
        }

        if (NVM_ErrorGet() != NVM_ERROR_NONE) {
            return false;
        }

        /* Read back through KSEG1 so the data cache cannot hide a failure. */
        if (memcmp((const void *) KVA0_TO_KVA1(rowAddr), rowData,
                FLASH_ROW_SIZE) != 0) {
            return false;
        }

        rowAddr += FLASH_ROW_SIZE;
    }

    flashWriteAddress = rowAddr;
    flashBufferIndex = 0U;
    __builtin_memset(flashBuffer, 0xFF, sizeof (flashBuffer));
    return true;
}

static uint8_t Next_Chunk_Index(uint8_t chunkIndex) {
    return (chunkIndex == 0xFFU) ? 1U : (uint8_t) (chunkIndex + 1U);
}

static void Boot_App_Chunk_Ack(uint8_t chunkIndex, uint8_t result) {
    uint8_t txBuf[8] = {0U};

    txBuf[0] = BOOT_CAN_CMD_0_APP_CHUNK;
    txBuf[1] = chunkIndex;
    txBuf[2] = BOOT_CAN_CMD_2_WRITE;
    txBuf[3] = result;
    CANbus_write(BOOT_CAN_ID_CLIENT, txBuf, sizeof (txBuf));
}

void Boot_App_Chunk_Handler(void) {
    uint8_t chunkIndex = canRxBuffer[1];

    if ((canRxBuffer[0] != BOOT_CAN_CMD_0_APP_CHUNK) ||
            (canRxBuffer[2] != BOOT_CAN_CMD_2_WRITE) ||
            (chunkIndex == 0U) || !firmwareUpdateActive || firmwareUpdateFailed) {
        Boot_App_Chunk_Ack(chunkIndex, BOOT_CAN_CMD_3_FAIL);
        return;
    }

    /* A lost ACK may cause the sender to repeat the completed chunk. */
    if (chunkIndex == lastAcceptedChunkIndex) {
        Boot_App_Chunk_Ack(chunkIndex, BOOT_CAN_CMD_3_SUCCESS);
        return;
    }

    if ((chunkIndex != expectedChunkIndex) ||
            ((flashBufferIndex + CAN_DATA_SIZE) > FLASH_CHUNK_SIZE)) {
        Boot_App_Chunk_Ack(chunkIndex, BOOT_CAN_CMD_3_FAIL);
        return;
    }

    (void) memcpy(&flashBuffer[flashBufferIndex], &canRxBuffer[4],
            CAN_DATA_SIZE);
    flashBufferIndex += CAN_DATA_SIZE;

    /* One protocol chunk contains 1024 firmware bytes. */
    if (flashBufferIndex < FLASH_CHUNK_SIZE) {
        return;
    }

    if (!App_Firmware_Write_Flash()) {
        firmwareUpdateFailed = true;
        Boot_App_Chunk_Ack(chunkIndex, BOOT_CAN_CMD_3_FAIL);
        return;
    }

    lastAcceptedChunkIndex = chunkIndex;
    expectedChunkIndex = Next_Chunk_Index(chunkIndex);
    Boot_App_Chunk_Ack(chunkIndex, BOOT_CAN_CMD_3_SUCCESS);
}

//

static uint32_t Boot_APP_CRC32_Software(const volatile uint8_t *data,
        uint32_t length) {
    uint32_t crc = 0xFFFFFFFFU;
    uint8_t bit;

    while (length-- > 0U) {
        crc ^= *data++;
        for (bit = 0U; bit < 8U; bit++) {
            crc = ((crc & 1U) != 0U) ?
                    ((crc >> 1U) ^ 0xEDB88320U) : (crc >> 1U);
        }
    }

    return crc ^ 0xFFFFFFFFU;
}

void Boot_APP_CRC_Handler(void) {
    uint8_t txBuf[8] = {0U};
    uint32_t flashCrc;

    if ((canRxBuffer[0] != BOOT_CAN_CMD_0_APP_CRC) ||
            (canRxBuffer[2] != BOOT_CAN_CMD_2_READ)) {
        return;
    }

    txBuf[0] = BOOT_CAN_CMD_0_APP_CRC;
    txBuf[2] = BOOT_CAN_CMD_2_READ;

    if (!firmwareUpdateActive || firmwareUpdateFailed ||
            (firmwareExpectedSize == 0U) || (flashBufferIndex != 0U)) {
        firmwareCrcVerified = false;
        txBuf[3] = BOOT_CAN_CMD_3_FAIL;
        CANbus_write(BOOT_CAN_ID_CLIENT, txBuf, sizeof (txBuf));
        return;
    }

    flashCrc = Boot_APP_CRC32_Software(
            (const volatile uint8_t *)APPLICATION_START_ADDRESS,
            firmwareExpectedSize);

    firmwareCrcVerified = true;
    txBuf[3] = BOOT_CAN_CMD_3_SUCCESS;
    txBuf[4] = (uint8_t) (flashCrc >> 24);
    txBuf[5] = (uint8_t) (flashCrc >> 16);
    txBuf[6] = (uint8_t) (flashCrc >> 8);
    txBuf[7] = (uint8_t) flashCrc;
    CANbus_write(BOOT_CAN_ID_CLIENT, txBuf, sizeof (txBuf));
}

void Boot_Stay_In_Boot_Handler(void) {
    uint8_t txBuf[8] = {0U};

    if (canRxBuffer[0] == BOOT_CAN_CMD_0_STAY_IN_BOOT) {
        bootStayRequested = true;
    }

    txBuf[0] = BOOT_CAN_CMD_0_STAY_IN_BOOT;
    txBuf[3] = BOOT_CAN_CMD_3_SUCCESS;
    CANbus_write(BOOT_CAN_ID_CLIENT, txBuf, sizeof (txBuf));
}

bool Boot_Stay_In_Boot_Requested(void) {
    return bootStayRequested;
}

void Run_Boot_Handler(uint32_t ID, uint8_t *rxBuffer) {
    __builtin_memcpy(canRxBuffer, rxBuffer, sizeof (canRxBuffer));

    switch (ID) {
        case BOOT_CAN_ID_HOST:
        {
            switch (rxBuffer[0]) {
                case BOOT_CAN_CMD_0_PIC_ID:
                {
                    Boot_PIC_ID_Handler();
                    break;
                }

                case BOOT_CAN_CMD_0_SOFTWARE_ID:
                {
                    Boot_Software_ID_Handler();
                    break;
                }

                case BOOT_CAN_CMD_0_STATUS:
                {
                    Boot_Status_Handler();
                    break;
                }

                case BOOT_CAN_CMD_0_APP_SIZE:
                {
                    Boot_App_Size_Handler();
                    break;
                }

                case BOOT_CAN_CMD_0_JUMP:
                {
                    Boot_Jump_Handler();
                    break;
                }

                case BOOT_CAN_CMD_0_APP_ERASE:
                {
                    Boot_App_Erase_Handler();
                    break;
                }

                case BOOT_CAN_CMD_0_APP_CHUNK:
                {
                    Boot_App_Chunk_Handler();
                    break;
                }

                case BOOT_CAN_CMD_0_APP_CRC:
                {
                    Boot_APP_CRC_Handler();
                    break;
                }

                case BOOT_CAN_CMD_0_STAY_IN_BOOT:
                {
                    Boot_Stay_In_Boot_Handler();
                    break;
                }

                default:
                    break;
            }
            break;
        }

        default:
            break;
    }
}
