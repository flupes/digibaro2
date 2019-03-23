#include <Arduino.h>

// We use a Zero compatible board
#define Serial SerialUSB

#include "bme280_sensor.h"

Bme280Sensor sensor(BME280_I2C_ADDR_SEC);
// Bmp280Sensor sensor(BMP280_I2C_ADDR_PRIM);

// compile with:
// pio ci .\test\TestBme280Sensor --board=zeroUSB --lib lib/BME280_driver --project-option="targets=upload" --keep-build-dir

// monitor with:
// pio device monitor --port COM5 --baud 115200

void setup() {
  Serial.begin(115200);
  while (!Serial)
    ;

  Serial.println("Starting...");

  if ( !sensor.Begin() ) {
    Serial.println("Sensor initialization error!");
    while (1);
  }

}

void loop() {

  sensor.PerformMeasurement();
  Serial.print("press = ");
  Serial.print(sensor.GetPressure());
  Serial.print("  | temp = ");
  Serial.print(sensor.GetTemperature());
  Serial.print("  | humi = ");
  Serial.print(sensor.GetHumidity());
  Serial.println();
  delay(1000);

}