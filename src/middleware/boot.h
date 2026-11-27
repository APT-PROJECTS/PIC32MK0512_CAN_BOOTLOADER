#ifndef _BOOT_H 
#define _BOOT_H

#include "definitions.h"  
    
/* =====================================================
 * PIC32MK1024MCM064 FLASH MEMORY MAP
 *
 * These are KSEG0 virtual addresses, matching the linker scripts used by
 * this project.  The bootloader executes from the 16-KiB upper boot flash
 * region and the application executes from program flash.  The final 4 KiB
 * program-flash page is reserved for EEPROM emulation.
 * =====================================================*/
//#define BOOTLOADER_START_ADDRESS        (0x9FC01000UL)
//#define BOOTLOADER_END_ADDRESS          (0x9FC04FFFUL)
//
//#define APPLICATION_START_ADDRESS       (0x9D000000UL)
//#define APPLICATION_END_ADDRESS         (0x9D0FEFFFUL)
//
//#define EEPROM_START_ADDRESS            (0x9D0FF000UL)
//#define EEPROM_END_ADDRESS              (0x9D0FFFFFUL)
//
//#define APPLICATION_JUMP_ADDRESS        (0xBD000000UL)
        
    
/* =====================================================
 * PIC32MK0512MCM064 FLASH MEMORY MAP
 *
 * These are KSEG0 virtual addresses, matching the linker scripts used by
 * this project.  The bootloader executes from the 16-KiB upper boot flash
 * region and the application executes from program flash.  The final 4 KiB
 * program-flash page is reserved for EEPROM emulation.
 * =====================================================*/
#define BOOTLOADER_START_ADDRESS        (0x9FC01000UL)
#define BOOTLOADER_END_ADDRESS          (0x9FC04FFFUL)

#define APPLICATION_START_ADDRESS       (0x9D000000UL)
#define APPLICATION_END_ADDRESS         (0x9D07EFFFUL)

#define EEPROM_START_ADDRESS            (0x9D07F000UL)
#define EEPROM_END_ADDRESS              (0x9D07FFFFUL)

#define APPLICATION_JUMP_ADDRESS        (0xBD000000UL)
    
    
/* =====================================================
 * CAN_ID
 * ===================================================== */
#define BOOT_CAN_ID_HOST                  0xB0U
#define BOOT_CAN_ID_CLIENT                0xB1U

/* =====================================================
 * CAN_CMD
 * ===================================================== */
#define BOOT_CAN_CMD_3_OPERATION_IDLE     0b000
#define BOOT_CAN_CMD_3_OPERATION_READ     0b001
#define BOOT_CAN_CMD_3_OPERATION_WRITE    0b010
#define BOOT_CAN_CMD_3_OPERATION_COMPARE  0b011

#define BOOT_CAN_CMD_3_STATUS_IDLE        0b00
#define BOOT_CAN_CMD_3_STATUS_SUCCESS     0b01
#define BOOT_CAN_CMD_3_STATUS_FAIL        0b10

#define BOOT_CAN_CMD_0_PIC_ID             0x01U
#define BOOT_CAN_CMD_1_PIC_MAC_ID_1       0x01U
#define BOOT_CAN_CMD_1_PIC_MAC_ID_2       0x02U
#define BOOT_CAN_CMD_1_PIC_MAC_ID_3       0x03U
#define BOOT_CAN_CMD_1_PIC_MAC_ID_4       0x04U
#define BOOT_CAN_CMD_1_PIC_DEVICE_ID      0x05U

#define BOOT_CAN_CMD_0_METADATA           0x02U
#define BOOT_CAN_CMD_1_METADATA_CHUNK_1   0x01U
#define BOOT_CAN_CMD_1_METADATA_CHUNK_2   0x02U
#define BOOT_CAN_CMD_1_METADATA_CHUNK_3   0x03U
#define BOOT_CAN_CMD_1_METADATA_CHUNK_4   0x04U
#define BOOT_CAN_CMD_1_METADATA_CHUNK_5   0x05U
#define BOOT_CAN_CMD_1_METADATA_CHUNK_6   0x06U
#define BOOT_CAN_CMD_1_METADATA_CHUNK_7   0x07U
#define BOOT_CAN_CMD_1_METADATA_CHUNK_8   0x08U
#define BOOT_CAN_CMD_1_METADATA_CHUNK_9   0x09U
#define BOOT_CAN_CMD_1_METADATA_CHUNK_10  0x0AU
#define BOOT_CAN_CMD_1_METADATA_CHUNK_11  0x0BU
#define BOOT_CAN_CMD_1_METADATA_CHUNK_12  0x0CU
#define BOOT_CAN_CMD_1_METADATA_CHUNK_13  0x0DU
#define BOOT_CAN_CMD_1_METADATA_CHUNK_14  0x0EU
#define BOOT_CAN_CMD_1_METADATA_CHUNK_15  0x0FU
#define BOOT_CAN_CMD_1_METADATA_CHUNK_16  0x10U
#define BOOT_CAN_CMD_1_METADATA_CHUNK_17  0x11U
#define BOOT_CAN_CMD_1_METADATA_CHUNK_18  0x12U

#define BOOT_CAN_CMD_0_STATUS             0x03U
#define BOOT_CAN_CMD_1_STATUS_BOOT        0x01U
#define BOOT_CAN_CMD_1_STATUS_APP         0x02U
#define BOOT_CAN_CMD_1_STATUS_UNKNOWN     0x03U

#define BOOT_CAN_CMD_0_APP_SIZE           0x04U

#define BOOT_CAN_CMD_0_JUMP               0x05U
#define BOOT_CAN_CMD_1_BOOT_JUMP          0x01U
#define BOOT_CAN_CMD_1_APP_JUMP           0x02U

#define BOOT_CAN_CMD_0_APP_ERASE          0x06U

#define BOOT_CAN_CMD_0_APP_CHUNK          0x07U

#define BOOT_CAN_CMD_0_APP_CRC            0x08U

#define BOOT_CAN_CMD_0_STAY_IN_BOOT       0x09U


/* =====================================================
 * API
 * ===================================================== */
void Boot_Init(void);
void Boot_PIC_ID_Handler(void);
void Boot_Metadata_Handler(void);
void Boot_Status_Handler(void);
void Boot_App_Size_Handler(void);
bool Boot_Is_App_Present(void);
void Boot_App_Jump(void);
void Boot_Jump_Handler(void);
void Boot_App_Erase_Handler(void);
bool App_Firmware_Write_Flash(void);
void Boot_App_Chunk_Ack(uint8_t chunkIndex, uint8_t operation, uint8_t status);
uint8_t Next_Chunk_Index(uint8_t chunkIndex);
void Boot_App_Chunk_Handler(void);
uint32_t Boot_APP_CRC32_Software(const volatile uint8_t *data, uint32_t length);
void Boot_APP_CRC_Handler(void);
void Boot_Stay_In_Boot_Handler(void);
bool Boot_Stay_In_Boot_Requested(void);
void Run_Boot_Handler(uint32_t id, uint8_t *rxBuffer);


#endif