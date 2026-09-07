#ifndef _DRIVER_CANBUS_H    /* Guard against multiple inclusion */
#define _DRIVER_CANBUS_H

/* Provide C++ Compatibility */
#ifdef __cplusplus
extern "C" {
#endif

#include "definitions.h"
    
void CANbus_init(void);         //Initiialize pins 
void CANbus_write(uint32_t Sadr, uint8_t * Sdata, uint8_t Sdata_L);        //add arguments as per protocol requirement
void CANbus_read(void);         //add arguments as per protocol requirements

/* Provide C++ Compatibility */
#ifdef __cplusplus
}
#endif

#endif /* _DRIVER_CANBUS */

/* *****************************************************************************
 End of File
 */
