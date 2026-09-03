#include <stddef.h>                     // Defines NULL
#include <stdbool.h>                    // Defines true
#include <stdlib.h>                     // Defines EXIT_FAILURE
#include "definitions.h"                // SYS function prototypes
#include "middleware/device_stat.h"
#include "software/state_engine.h"

int main(void) {
    SYS_Initialize(NULL);
    
    RED_LED_Set();
    
    EEPROM_Initialize();

    while (true) {
        STATE_ENGINE_Tasks();
    }

    return ( EXIT_FAILURE);
}