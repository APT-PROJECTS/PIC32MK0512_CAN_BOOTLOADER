#include "definitions.h"
#include "driver.h"
#include "middleware.h"
#include "software.h"

//--------------------------------------------------
// STATIC STATE VARIABLE
//--------------------------------------------------
static STATE_ENGINE_STATES currentState = STATE_INIT;


//--------------------------------------------------
// STATE MACHINE TASKS
//--------------------------------------------------

void STATE_ENGINE_Tasks(void) {

    switch (currentState) {
        case STATE_INIT:
        {
            Flash_EEPROM_Init();
            Metadata_Init();
            Boot_Init();
            currentState = STATE_RUN;
            break;
        }

        case STATE_RUN:
        {
            CANbus_read();
            break;
        }

        default:
            break;
    }
}