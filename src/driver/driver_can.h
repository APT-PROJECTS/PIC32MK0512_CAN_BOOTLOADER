#ifndef _DRIVER_CAN_H   
#define _DRIVER_CAN_H

#include "definitions.h"

#define CAN_FRAME_DATA_SIZE     4U
    
void CANbus_init(void);
void CANbus_write(uint32_t Sadr, uint8_t * Sdata, uint8_t Sdata_L);
void CANbus_read(void);

#endif