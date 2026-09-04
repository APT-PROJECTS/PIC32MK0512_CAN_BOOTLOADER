#ifndef SYSTEM_INIT_H
#define SYSTEM_INIT_H

#include <Arduino.h>

// Initialize all peripherals
void System_Init(void);

// Create Core0 tasks
void System_CreateTasks(void);

// Start 1ms timer
void System_StartTimer(void);

#endif
