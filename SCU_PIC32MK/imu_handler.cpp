#include "imu_handler.h"

// --------------------------------------------------
// Initialize MPU9250 and magnetometer
// --------------------------------------------------
void IMU_Init() {
  Wire.begin(21, 22);
  delay(100);

  Serial.println("Initializing MPU9250...");

  // Wake up MPU9250
  Wire.beginTransmission(MPU_ADDR);
  Wire.write(PWR_MGMT_1);
  Wire.write(0x00);
  Wire.endTransmission();
  delay(100);

  // Enable I2C bypass to access magnetometer
  Wire.beginTransmission(MPU_ADDR);
  Wire.write(INT_PIN_CFG);
  Wire.write(0x02);
  Wire.endTransmission();
  delay(100);

  // Configure magnetometer: 16-bit, continuous mode 2
  Wire.beginTransmission(MAG_ADDR);
  Wire.write(MAG_CNTL1);
  Wire.write(0x16);
  Wire.endTransmission();
  delay(100);

  // WHO_AM_I check
  Wire.beginTransmission(MPU_ADDR);
  Wire.write(0x75);
  Wire.endTransmission(false);
  Wire.requestFrom(MPU_ADDR, 1);
  Serial.print("MPU9250 WHO_AM_I: 0x");
  Serial.println(Wire.read(), HEX);

  Wire.beginTransmission(MAG_ADDR);
  Wire.write(0x00);
  Wire.endTransmission(false);
  Wire.requestFrom(MAG_ADDR, 1);
  Serial.print("Magnetometer WHO_AM_I: 0x");
  Serial.println(Wire.read(), HEX);

  Serial.println("MPU9250 Initialized.");
}

// --------------------------------------------------
// Read raw accelerometer data
// --------------------------------------------------
static void readAccelData(int16_t* data) {
  Wire.beginTransmission(MPU_ADDR);
  Wire.write(ACCEL_XOUT_H);
  Wire.endTransmission();
  Wire.requestFrom(MPU_ADDR, 6);

  data[0] = (Wire.read() << 8) | Wire.read();
  data[1] = (Wire.read() << 8) | Wire.read();
  data[2] = (Wire.read() << 8) | Wire.read();
}

// --------------------------------------------------
// Read magnetometer data
// --------------------------------------------------
static void readMagData(int16_t* data) {
  uint8_t raw[6];

  Wire.beginTransmission(MAG_ADDR);
  Wire.write(MAG_XOUT_L);
  Wire.endTransmission();
  Wire.requestFrom(MAG_ADDR, 6);

  for (int i = 0; i < 6; i++) raw[i] = Wire.read();

  data[0] = (raw[1] << 8) | raw[0];
  data[1] = (raw[3] << 8) | raw[2];
  data[2] = (raw[5] << 8) | raw[4];

  // Clear data ready flag
  Wire.beginTransmission(MAG_ADDR);
  Wire.write(0x09);
  Wire.endTransmission(false);
  Wire.requestFrom(MAG_ADDR, 1);
  Wire.read();
}

// --------------------------------------------------
// Read temperature data
// --------------------------------------------------
static float readTemperature() {
  Wire.beginTransmission(MPU_ADDR);
  Wire.write(TEMP_OUT_H);
  Wire.endTransmission();
  Wire.requestFrom(MPU_ADDR, 2);

  int16_t raw = (Wire.read() << 8) | Wire.read();
  return (raw / 333.87f) + 21.0f;
}

// --------------------------------------------------
// Update Roll, Pitch, Yaw, Temperature
// --------------------------------------------------
void IMU_Update(uint8_t *scuBuffer)
{
    int16_t accel[3], mag[3];
    readAccelData(accel);
    readMagData(mag);

    float temp = readTemperature();

    // Convert to real units
    float ax = accel[0] / 16384.0f;
    float ay = accel[1] / 16384.0f;
    float az = accel[2] / 16384.0f;

    float mx = mag[0] * 0.6f;
    float my = mag[1] * 0.6f;
    float mz = mag[2] * 0.6f;

    // Normalize accelerometer vector
    float norm = sqrt(ax * ax + ay * ay + az * az);
    if (norm == 0) return;   // safety check

    ax /= norm;
    ay /= norm;
    az /= norm;

    float roll  = atan2(ay, az) * 180.0f / M_PI;
    if (roll > 180.0f) roll -= 360.0f;

    float pitch = atan2(ax, az) * 180.0f / M_PI;
    if (pitch > 180.0f) pitch -= 360.0f;

    // Tilt compensated yaw
    float mx_tilt = mx * az - mz * ax;
    float my_tilt = my * az - mz * ay;

    float yaw = atan2(my_tilt, mx_tilt) * 180.0f / M_PI;
    if (yaw > 180.0f)  yaw -= 360.0f;
    if (yaw < -180.0f) yaw += 360.0f;

    // -------------------------------------------------
    // ✅ DIRECT STORAGE INTO SCU BUFFER
    // -------------------------------------------------
    memcpy(&scuBuffer[3 * 4], &roll, sizeof(float));   // index 3
    memcpy(&scuBuffer[4 * 4], &pitch, sizeof(float));  // index 4
    // memcpy(&scuBuffer[5 * 4], &temp, sizeof(float));   // index 5
    memcpy(&scuBuffer[6 * 4], &yaw, sizeof(float));    // index 6
}

// --------------------------------------------------
// Print IMU data
// --------------------------------------------------
void IMU_Print(float roll, float pitch, float yaw, float temp) {
  Serial.printf("IMU → Roll=%.2f°  Pitch=%.2f°  Yaw=%.2f°  Temp=%.2f°C\n",
                roll, pitch, yaw, temp);
}
