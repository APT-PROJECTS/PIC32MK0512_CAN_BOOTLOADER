#ifndef IMU_HANDLER_H
#define IMU_HANDLER_H

#include <Arduino.h>
#include <Wire.h>
#include <math.h>

// I2C Addresses
#define MPU_ADDR 0x69
#define MAG_ADDR 0x0C

// Register map constants
#define PWR_MGMT_1 0x6B
#define INT_PIN_CFG 0x37
#define MAG_CNTL1 0x0A
#define TEMP_OUT_H 0x41
#define ACCEL_XOUT_H 0x3B
#define MAG_XOUT_L 0x03


// Function prototypes
void IMU_Init();
static void readAccelData(int16_t* data);
static void readMagData(int16_t* data);
static float readTemperature();
void IMU_Update(uint8_t *scuBuffer);
void IMU_Print(float roll, float pitch, float yaw, float temp);

#endif
