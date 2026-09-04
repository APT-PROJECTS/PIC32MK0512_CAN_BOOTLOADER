#include "mcp23017_handler.h"

// ----------------------------------------------------
static void MCP_WriteRegister(uint8_t reg, uint8_t value)
{
    Wire.beginTransmission(MCP23017_ADDR);
    Wire.write(reg);
    Wire.write(value);
    Wire.endTransmission();
}

// ----------------------------------------------------
void MCP23017_Init(void)
{
    Wire.begin(21, 22);
    delay(10);

    // Set PORTA as output
    MCP_WriteRegister(IODIRA, 0x00);

    portA_state = 0x00;
    MCP_WriteRegister(GPIOA, portA_state);
}

// ================= RED =================
void LED_Red_On(void)
{
    portA_state |= (1 << RED_PIN);
    MCP_WriteRegister(GPIOA, portA_state);
}

void LED_Red_Off(void)
{
    portA_state &= ~(1 << RED_PIN);
    MCP_WriteRegister(GPIOA, portA_state);
}

// ================= GREEN =================
void LED_Green_On(void)
{
    portA_state |= (1 << GREEN_PIN);
    MCP_WriteRegister(GPIOA, portA_state);
}

void LED_Green_Off(void)
{
    portA_state &= ~(1 << GREEN_PIN);
    MCP_WriteRegister(GPIOA, portA_state);
}

// ================= BLUE =================
void LED_Blue_On(void)
{
    portA_state |= (1 << BLUE_PIN);
    MCP_WriteRegister(GPIOA, portA_state);
}

void LED_Blue_Off(void)
{
    portA_state &= ~(1 << BLUE_PIN);
    MCP_WriteRegister(GPIOA, portA_state);
}
