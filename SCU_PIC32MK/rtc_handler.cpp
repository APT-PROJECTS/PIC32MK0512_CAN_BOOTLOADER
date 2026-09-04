#include "rtc_handler.h"

// Convert normal decimal numbers to BCD
static uint8_t dec2bcd(uint8_t val) {
    return (uint8_t)((val / 10 * 16) + (val % 10));
}

// Convert BCD to normal decimal numbers
static uint8_t bcd2dec(uint8_t val) {
    return (uint8_t)((val / 16 * 10) + (val % 16));
}

// Zeller-like day of week (returns 1..7, Sunday=1)
// Works for Gregorian calendar (years >= 1900)
static uint8_t computeWeekday(uint16_t year, uint8_t month, uint8_t day) {
    int y = (int) year;
    int m = (int) month;
    int d = (int) day;
    if (m < 3) { m += 12; y -= 1; }
    int K = y % 100;
    int J = y / 100;
    int h = (d + (13*(m + 1))/5 + K + K/4 + J/4 + 5*J) % 7; // 0 = Saturday
    // Convert h to 1..7 where 1 = Sunday
    int dow = ((h + 6) % 7) + 1;
    return (uint8_t)dow;
}

// ----------------------------------------------------
// Initialize RTC
// ----------------------------------------------------
void RTC_Init() {
    Wire.begin(21, 22);  // SDA, SCL on ESP32
    delay(10);

    // (Optional) Clear EOSC bit if set to ensure oscillator runs.
    // Read control/status registers then clear EOSC if necessary.
    // For now just print that RTC is ready.
    Serial.println("DS3231 RTC Initialized (I2C 21/22)");
}

// ----------------------------------------------------
// Set RTC Time
// Writes seconds, minutes, hours, day-of-week, date, month, year
// ----------------------------------------------------
void RTC_SetTime(const RTC_Time_t &t) {
    uint8_t weekday = computeWeekday(t.year, t.month, t.day);

    Wire.beginTransmission(DS3231_ADDR);
    Wire.write(0x00); // start at seconds register

    Wire.write(dec2bcd(t.second));      // 0x00 Seconds
    Wire.write(dec2bcd(t.minute));      // 0x01 Minutes
    Wire.write(dec2bcd(t.hour));        // 0x02 Hours (24-hour assumed)
    Wire.write(dec2bcd(weekday));       // 0x03 Day (day-of-week)
    Wire.write(dec2bcd(t.day));         // 0x04 Date (day-of-month)
    Wire.write(dec2bcd(t.month));       // 0x05 Month (century bit not handled here)
    Wire.write(dec2bcd((uint8_t)(t.year - 2000))); // 0x06 Year (00..99)
    Wire.endTransmission();

    Serial.println("RTC Time Set");
}

// ----------------------------------------------------
// Get RTC Time
// Reads registers 0x00..0x06 and fills RTC_Time_t
// ----------------------------------------------------
void RTC_GetTime(RTC_Time_t *t) {
    Wire.beginTransmission(DS3231_ADDR);
    Wire.write(0x00); // start at seconds register
    Wire.endTransmission();

    Wire.requestFrom(DS3231_ADDR, 7);

    if (Wire.available() < 7) {
        Serial.println("RTC read error!");
        return;
    }

    t->second = bcd2dec(Wire.read()); // 0x00 Seconds
    t->minute = bcd2dec(Wire.read()); // 0x01 Minutes
    t->hour   = bcd2dec(Wire.read()); // 0x02 Hours
    uint8_t dow = bcd2dec(Wire.read()); // 0x03 Day of week (ignored)
    (void)dow;
    t->day   = bcd2dec(Wire.read());   // 0x04 Date (day-of-month)
    t->month = bcd2dec(Wire.read() & 0x7F); // 0x05 Month (mask century bit)
    t->year  = 2000 + bcd2dec(Wire.read()); // 0x06 Year
}

void ApplyISTOffset(RTC_Time_t *t)
{
    t->minute += 30;
    t->hour   += 5;

    if (t->minute >= 60) {
        t->minute -= 60;
        t->hour++;
    }

    if (t->hour >= 24) {
        t->hour -= 24;
        t->day++;
        // NOTE: month/year rollover skipped for simplicity
        // (not needed for IST offset in practice)
    }
}

// ----------------------------------------------------
// Print RTC Time
// ----------------------------------------------------
void RTC_PrintTime(const RTC_Time_t &t, const uint8_t *rtcBuffer)
{
    // Print normal date/time
    Serial.printf("RTC → %02d/%02d/%04d %02d:%02d:%02d\n",
                  t.day, t.month, t.year,
                  t.hour, t.minute, t.second);

    // Print RTC buffer
    Serial.print("RTC Buffer: ");
    for (int i = 0; i < 8; i++)
    {
        Serial.printf("%02X ", rtcBuffer[i]);
    }
    Serial.println();
}


void RTC_Store(uint8_t *rtcBuffer, const RTC_Time_t &t)
{
    rtcBuffer[0] = 0x00;                 // reserved

    uint16_t y = t.year;
    rtcBuffer[1] = (y >> 8) & 0xFF;      // Year MSB
    rtcBuffer[2] = y & 0xFF;             // Year LSB

    rtcBuffer[3] = t.month;
    rtcBuffer[4] = t.day;
    rtcBuffer[5] = t.hour;
    rtcBuffer[6] = t.minute;
    rtcBuffer[7] = t.second;
}


