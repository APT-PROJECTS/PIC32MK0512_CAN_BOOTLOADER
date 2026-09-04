#ifndef MCP23017_HANDLER_H
#define MCP23017_HANDLER_H

#include <Arduino.h>
#include <Wire.h>

#define MCP23017_ADDR 0x20

// MCP23017 Registers
#define IODIRA   0x00
#define GPIOA    0x12

// PORT A Mapping (PORT A)
#define IMMOBILIZE    3       // GPA3
#define DICKY_LOCK    4       // GPA4
#define BLUE_PIN      5       // GPA5
#define GREEN_PIN     6       // GPA6
#define RED_PIN       7       // GPA7

// PORT B Mapping (PORT B)
#define LOW_GEAR              0       // GPB0
#define HIGH_GEAR             1       // GPB1
#define BRAKE_LOW             2       // GPB2
#define SIDE_STAND_SWITCH     3       // GPB3
#define REVERSE               4       // GPB4
#define CRUISE                5       // GPB5
#define GLOVE_BOX             6       // GPB6

static uint8_t portA_state = 0x00;

// Initialize MCP23017
void MCP23017_Init(void);

// LED Control Functions
void LED_Red_On(void);
void LED_Red_Off(void);

void LED_Green_On(void);
void LED_Green_Off(void);

void LED_Blue_On(void);
void LED_Blue_Off(void);

#endif
