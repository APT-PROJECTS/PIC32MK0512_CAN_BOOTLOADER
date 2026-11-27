#ifndef STATE_ENGINE_H
#define STATE_ENGINE_H

#include "definitions.h"

//--------------------------------------------------
// STATE ENUM
//--------------------------------------------------
typedef enum
{
    STATE_INIT = 0,
    STATE_RUN
} STATE_ENGINE_STATES;

//--------------------------------------------------
// PUBLIC FUNCTIONS
//--------------------------------------------------
void STATE_ENGINE_Tasks(void);
 
#endif