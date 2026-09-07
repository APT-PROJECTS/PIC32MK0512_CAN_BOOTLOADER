#ifndef _BOOT_HANDLER_NAME_H 
#define _BOOT_HANDLER_NAME_H

#include "definitions.h"

#ifdef __cplusplus
extern "C" {
#endif    
    
/* =====================================================
 * PIC32MK1024MCM064 FLASH MEMORY MAP
 *
 * These are KSEG0 virtual addresses, matching the linker scripts used by
 * this project.  The bootloader executes from the 16-KiB upper boot flash
 * region and the application executes from program flash.  The final 4 KiB
 * program-flash page is reserved for EEPROM emulation.
 * =====================================================*/
#define BOOTLOADER_START_ADDRESS        (0x9FC01000UL)
#define BOOTLOADER_END_ADDRESS          (0x9FC04FFFUL)

#define APPLICATION_START_ADDRESS       (0x9D000000UL)
#define APPLICATION_END_ADDRESS         (0x9D0FEFFFUL)

#define EEPROM_START_ADDRESS            (0x9D0FF000UL)
#define EEPROM_END_ADDRESS              (0x9D0FFFFFUL)

#define APPLICATION_JUMP_ADDRESS        (0xBD000000UL)
    
/* =====================================================
 * CAN_ID
 * ===================================================== */
typedef enum {
  BOOT_CAN_ID_HOST = 0xB0U,
  BOOT_CAN_ID_CLIENT = 0xB1U
} CAN_ID_t;

/* =====================================================
 * CAN_CMD
 * ===================================================== */
typedef enum {
  BOOT_CAN_CMD_2_READ = 0x01,
  BOOT_CAN_CMD_2_WRITE = 0x02,
  BOOT_CAN_CMD_2_COMPARE = 0x03,

  BOOT_CAN_CMD_3_SUCCESS = 0x01,
  BOOT_CAN_CMD_3_FAIL = 0x02,

  BOOT_CAN_CMD_0_PIC_ID = 0x01U,
  BOOT_CAN_CMD_1_PIC_ID_1 = 0x01U,
  BOOT_CAN_CMD_1_PIC_ID_2 = 0x02U,
  BOOT_CAN_CMD_1_PIC_ID_3 = 0x03U,
  BOOT_CAN_CMD_1_PIC_ID_4 = 0x04U,

  BOOT_CAN_CMD_0_SOFTWARE_ID = 0x02U,
  BOOT_CAN_CMD_1_PROJECT_ID = 0x01U,
  BOOT_CAN_CMD_1_FIRMWARE_ID = 0x02U,

  BOOT_CAN_CMD_0_STATUS = 0x03U,
  BOOT_CAN_CMD_1_STATUS_BOOT = 0x01U,
  BOOT_CAN_CMD_1_STATUS_APP = 0x02U,
  BOOT_CAN_CMD_1_STATUS_UNKNOWN = 0x03U,

  BOOT_CAN_CMD_0_APP_SIZE = 0x04U,

  BOOT_CAN_CMD_0_JUMP = 0x05U,
  BOOT_CAN_CMD_1_BOOT_JUMP = 0x01U,
  BOOT_CAN_CMD_1_APP_JUMP = 0x02U,

  BOOT_CAN_CMD_0_APP_ERASE = 0x06U,

  BOOT_CAN_CMD_0_APP_CHUNK = 0x07U,

  BOOT_CAN_CMD_0_APP_CRC = 0x08U,

  BOOT_CAN_CMD_0_STAY_IN_BOOT = 0x09U,
} CAN_CMD_t;


/* =====================================================
 * API
 * ===================================================== */
void Boot_PIC_ID_Handler(void);
void Boot_Software_ID_Handler(void);
void Boot_Status_Handler(void);
void Boot_App_Size_Handler(void);
bool Boot_Is_App_Present(void);
void Boot_App_Jump(void);
void Boot_Jump_Handler(void);
void Boot_App_Erase_Handler(void);
static bool App_Firmware_Write_Flash(void);
static void Boot_App_Chunk_Ack(uint8_t chunkIndex, uint8_t result);
static uint8_t Next_Chunk_Index(uint8_t chunkIndex);
void Boot_App_Chunk_Handler(void);
static uint32_t Boot_APP_CRC32_Software(const volatile uint8_t *data, uint32_t length);
void Boot_APP_CRC_Handler(void);
void Boot_Stay_In_Boot_Handler(void);
bool Boot_Stay_In_Boot_Requested(void);
void Run_Boot_Handler(uint32_t ID, uint8_t *rxBuffer);


#ifdef __cplusplus
}
#endif

#endif
