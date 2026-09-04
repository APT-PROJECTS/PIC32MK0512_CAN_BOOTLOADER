#ifndef GPS_HANDLER_H
#define GPS_HANDLER_H

#include <Arduino.h>
#include <TinyGPS++.h>

// GPS on UART2
#define GPS_RX_PIN 17 //32
#define GPS_TX_PIN 16 //33

static float latitude         = NAN;
static float longitude        = NAN;
static float speedKmh         = 0.0;
static float altitudeMeters   = 0.0;

static bool rtcSetFromGPS = false;

void GPS_Init();
void GPS_Update(uint8_t *scuBuffer);

#endif
