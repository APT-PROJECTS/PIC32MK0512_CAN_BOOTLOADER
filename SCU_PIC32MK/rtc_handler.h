#ifndef RTC_HANDLER_H
#define RTC_HANDLER_H

#include <Arduino.h>
#include <Wire.h>

// DS3231 I2C address
#define DS3231_ADDR 0x68


// Structure to hold RTC time
typedef struct {
    uint8_t second;
    uint8_t minute;
    uint8_t hour;
    uint8_t day;    // day of month (1..31)
    uint8_t month;  // month (1..12)
    uint16_t year;  // full year, e.g. 2025
} RTC_Time_t;

// Function declarations
void RTC_Init();
void RTC_SetTime(const RTC_Time_t &t);   // set using day-of-month; weekday computed automatically
void RTC_GetTime(RTC_Time_t *t);
void ApplyISTOffset(RTC_Time_t *t);
void RTC_PrintTime(const RTC_Time_t &t, const uint8_t *rtcBuffer);
void RTC_Store(uint8_t *rtcBuffer, const RTC_Time_t &t);

#endif
