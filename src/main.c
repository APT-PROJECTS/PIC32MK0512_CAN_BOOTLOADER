#include <stddef.h>                     // Defines NULL
#include <stdbool.h>                    // Defines true
#include <stdlib.h>                     // Defines EXIT_FAILURE
#include "definitions.h"                // SYS function prototypes
#include "middleware.h"
#include "software.h"

int main(void) {
    SYS_Initialize(NULL);

    RED_LED_Set();

    while (true) {
        STATE_ENGINE_Tasks();
    }

    return ( EXIT_FAILURE);
}