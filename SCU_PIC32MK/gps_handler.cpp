#include "gps_handler.h"
#include "rtc_handler.h"
#include "eeprom_handler.h"

HardwareSerial GPS_Serial(2);
TinyGPSPlus gps;

void GPS_Init() {
  GPS_Serial.begin(9600, SERIAL_8N1, GPS_RX_PIN, GPS_TX_PIN);
  delay(200);
}

void GPS_Update(uint8_t *scuBuffer) {
  while (GPS_Serial.available() > 0) {
    gps.encode(GPS_Serial.read());
  }

  // --- Latitude & Longitude ---
  if (gps.location.isValid()) {
    latitude = gps.location.lat();
    longitude = gps.location.lng();
  }

  // Always copy current value (which will be NAN until first fix)
  memcpy(&scuBuffer[0 * 4], &latitude, sizeof(float));
  memcpy(&scuBuffer[1 * 4], &longitude, sizeof(float));

  // --- Speed in km/h ---
  if (gps.speed.isValid()) {
    speedKmh = gps.speed.kmph();
    memcpy(&scuBuffer[2 * 4], &speedKmh, sizeof(float));  // index 2
  }

  // --- Altitude in meters ---
  if (gps.altitude.isValid()) {
    altitudeMeters = gps.altitude.meters();
    // memcpy(&scuBuffer[7 * 4], &altitudeMeters, sizeof(float));  // index 7
  }

  // --------------------------------------------------
  // GPS → RTC SYNC (ONLY ONCE PER BOOT)
  // --------------------------------------------------
  if (!rtcSetFromGPS && gps.date.isValid() && gps.time.isValid() && gps.location.isValid()) {
    RTC_Time_t t;

    t.year = gps.date.year();
    t.month = gps.date.month();
    t.day = gps.date.day();
    t.hour = gps.time.hour();
    t.minute = gps.time.minute();
    t.second = gps.time.second();

    if (t.year >= 2024 && t.year <= 2035) {
      ApplyISTOffset(&t);
      RTC_SetTime(t);
      rtcSetFromGPS = true;

      Serial.printf(
        "RTC set from GPS: %02d/%02d/%04d %02d:%02d:%02d (IST)\n",
        t.day, t.month, t.year,
        t.hour, t.minute, t.second);
    }
  }
}
