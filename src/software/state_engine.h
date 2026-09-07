#ifndef STATE_ENGINE_H
#define STATE_ENGINE_H

#ifdef __cplusplus
extern "C" {
#endif

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

#ifdef __cplusplus
}
#endif
 
#endif // STATE_ENGINE_H