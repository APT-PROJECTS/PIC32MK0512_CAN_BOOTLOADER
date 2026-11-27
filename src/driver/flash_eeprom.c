#include "sys/kmem.h"
#include "definitions.h"
#include "driver.h"

typedef struct {
    uint32_t marker;
    uint32_t sequence;
    uint32_t crc;
    uint32_t length;
    uint32_t reserved;
} EEPROM_HEADER;

static uint32_t activePage = EEPROM_PAGE_ADDR;

uint8_t eepromBuffer[EEPROM_DATA_SIZE];

static uint32_t calculate_crc(uint8_t *data, uint32_t len) {
    uint32_t crc = 0;

    for (uint32_t i = 0; i < len; i++) {
        crc ^= data[i];

        for (uint8_t j = 0; j < 8; j++) {
            if (crc & 1)
                crc = (crc >> 1) ^ 0xEDB88320;
            else
                crc >>= 1;
        }
    }

    return crc;
}

static bool flash_wait_complete(void) {
    while (NVM_IsBusy());

    if (NVM_ErrorGet())
        return false;

    return true;
}

static bool erase_page(uint32_t addr) {
    NVM_PageErase(addr);

    return flash_wait_complete();
}

static bool write_word(uint32_t addr, uint32_t data) {
    NVM_WordWrite(data, addr);

    return flash_wait_complete();
}

static bool page_is_valid(uint32_t addr, EEPROM_HEADER *header) {
    memcpy(
            header,
            (void*) KVA0_TO_KVA1(addr),
            sizeof (EEPROM_HEADER)
            );

    if (header->marker != EEPROM_VALID_MARKER)
        return false;

    if (header->sequence == 0xFFFFFFFF)
        return false;

    if (header->length > EEPROM_DATA_SIZE)
        return false;

    if (header->reserved != 0xAAAAAAAA)
        return false;

    /* verify actual stored data CRC */
    static uint8_t tempBuffer[EEPROM_DATA_SIZE];

    memcpy(
            tempBuffer,
            (void*) KVA0_TO_KVA1(addr + sizeof (EEPROM_HEADER)),
            header->length
            );

    uint32_t crc = calculate_crc(tempBuffer, header->length);

    if (crc != header->crc)
        return false;

    return true;
}

FLASH_EEPROM_STATUS Flash_EEPROM_Init(void) {
    /* A blank page is valid startup state; it is erased on the first write. */
    activePage = EEPROM_PAGE_ADDR;
    
    __builtin_memset(eepromBuffer, 0, sizeof (eepromBuffer));
    Flash_EEPROM_Read_Page(eepromBuffer, sizeof (eepromBuffer));
    
    return FLASH_EEPROM_OK;
}

FLASH_EEPROM_STATUS Flash_EEPROM_Write_Page(uint8_t *data, uint32_t length) {
    if (length > EEPROM_DATA_SIZE)
        return FLASH_EEPROM_ERROR;

    EEPROM_HEADER oldHdr;
    EEPROM_HEADER newHdr;

    if (page_is_valid(activePage, &oldHdr)) {
        newHdr.sequence = oldHdr.sequence + 1;
    } else {
        newHdr.sequence = 1;
    }

    if (!erase_page(activePage))
        return FLASH_EEPROM_ERROR;

    newHdr.marker = EEPROM_VALID_MARKER;
    newHdr.length = length;
    newHdr.crc = calculate_crc(data, length);
    newHdr.reserved = 0xAAAAAAAA;

    uint32_t writeAddr = activePage + sizeof (EEPROM_HEADER);

    /* Write DATA first */
    for (uint32_t i = 0; i < length; i += 4) {
        uint32_t word = 0xFFFFFFFF;

        memcpy(&word,
                &data[i],
                (length - i) >= 4 ? 4 : (length - i));

        if (!write_word(writeAddr + i, word)) {
            return FLASH_EEPROM_ERROR;
        }
    }

    /* Write HEADER last -> commit page */
    uint32_t *hdrPtr = (uint32_t*) & newHdr;

    /* Write sequence, crc, length, reserved first */
    for (int i = 1; i < sizeof (EEPROM_HEADER) / 4; i++) {
        if (!write_word(activePage + (i * 4), hdrPtr[i])) {
            return FLASH_EEPROM_ERROR;
        }
    }

    /* Write marker LAST -> final commit */
    if (!write_word(activePage, EEPROM_VALID_MARKER)) {
        return FLASH_EEPROM_ERROR;
    }

    return FLASH_EEPROM_OK;
}

FLASH_EEPROM_STATUS Flash_EEPROM_Read_Page(uint8_t *data, uint32_t length) {
    EEPROM_HEADER hdr;

    if (!page_is_valid(activePage, &hdr))
        return FLASH_EEPROM_NO_VALID_PAGE;

    if (length > hdr.length)
        length = hdr.length;

    if (!NVM_Read(
            (uint32_t*) data,
            length,
            activePage + sizeof (EEPROM_HEADER)
            )) {
        return FLASH_EEPROM_ERROR;
    }

    return FLASH_EEPROM_OK;
}

FLASH_EEPROM_STATUS FLASH_EEPROM_WriteWord(uint32_t address, uint32_t data) {
    if (address >= (EEPROM_DATA_SIZE / 4)) {
        return FLASH_EEPROM_ERROR;
    }

    static uint8_t buffer[EEPROM_DATA_SIZE];
    memset(buffer, 0xFF, EEPROM_DATA_SIZE);

    FLASH_EEPROM_STATUS status;

    status = Flash_EEPROM_Read_Page(buffer, EEPROM_DATA_SIZE);

    if ((status != FLASH_EEPROM_OK) &&
            (status != FLASH_EEPROM_NO_VALID_PAGE)) {
        return status;
    }

    uint32_t *wordBuffer = (uint32_t*) buffer;

    wordBuffer[address] = data;

    return Flash_EEPROM_Write_Page(buffer, EEPROM_DATA_SIZE);
}

FLASH_EEPROM_STATUS FLASH_EEPROM_ReadWord(uint32_t address, uint32_t *data) {
    if (address >= (EEPROM_DATA_SIZE / 4)) {
        return FLASH_EEPROM_ERROR;
    }

    static uint8_t buffer[EEPROM_DATA_SIZE];

    FLASH_EEPROM_STATUS status;

    status = Flash_EEPROM_Read_Page(buffer, EEPROM_DATA_SIZE);

    if (status != FLASH_EEPROM_OK) {
        return status;
    }

    uint32_t *wordBuffer = (uint32_t*) buffer;

    *data = wordBuffer[address];

    return FLASH_EEPROM_OK;
}

FLASH_EEPROM_STATUS FLASH_EEPROM_EraseAll(void) {
    if (!erase_page(EEPROM_PAGE_ADDR))
        return FLASH_EEPROM_ERROR;

    activePage = EEPROM_PAGE_ADDR;

    return FLASH_EEPROM_OK;
}
