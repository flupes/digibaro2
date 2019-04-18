// compile with:
/*
 pio ci .\test\CollectBaroSamples --board=zeroUSB -l lib\BaroUtils
   -l lib\BME280_driver -l lib\RTClib -l lib\BaroSample -l lib\SPIMemory
   -l lib\PermanentSamples -O "targets=upload" --keep-build-dir
*/

// monitor with:
// pio device monitor --port COM5 --baud 115200

#include "print_utils.h"

#include "RTClib.h"
#include "SPIMemory.h"
#include "baro_sample.h"
#include "bme280_sensor.h"
#include "permanent_samples.h"

// Global variables:

// On board SPI Flash Memory
SPIFlash flash(kMiniUltraProOnBoardChipSelectPin);

// Real Time Clock
RTC_DS3231 rtc;

// BME 280 Pressure/Temperature/Humidity sensor
Bme280Sensor bme(BME280_I2C_ADDR_SEC);

// Utility to write samples to flash
PermanentSamples samples(flash);

const int8_t kTimeZone = -8;

void setup() {
  Serial.begin(115200);
  uint8_t count = 0;
  while (!Serial && count < 12) {
    delay(1000);
    count++;
  }

  PRINTLN("CollectBaroSamples Starting...");

  if (!flash.begin()) {
    PRINTLN("Flash memory initialization error!");
    while (1)
      ;
  }

  if (!bme.Begin()) {
    PRINTLN("Sensor initialization error!");
    while (1)
      ;
  }


  PRINT("Start address of permanent sample = ");
  PRINTLN(samples.GetFirstSampleAddr());
  PRINT("Max number of samples = ");
  PRINTLN(samples.GetMaxNumberOfSamples());
  uint32_t index = samples.begin();
  PRINT("Last retrieved sample index = ");
  PRINTLN(index);
}

void collect_sample(DateTime &dt) {
  bme.PerformMeasurement();

  BaroSample sample(dt.unixtime(), bme.GetPressure() / 100,
                    bme.GetTemperature(), bme.GetHumidity() / 10);

  if (Serial) {
    char buffer[64];
    DateTime local = dt.getLocalTime(kTimeZone);
    local.toString(buffer);
    Serial.print("collect_sample at time = ");
    Serial.print(buffer);
    Serial.print(" : press = ");
    Serial.print(bme.GetPressure());
    Serial.print("  | temp = ");
    Serial.print(bme.GetTemperature());
    Serial.print("  | humi = ");
    Serial.print(bme.GetHumidity());
    Serial.println();
    Serial.print("--> sample created with seconds = ");
    Serial.print(sample.GetTimestamp());
    Serial.print(" : press = ");
    Serial.print(sample.PressureMilliBar());
    Serial.print(" | temp = ");
    Serial.print(sample.TemperatureDegCelcius());
    Serial.print(" | humi = ");
    Serial.print(sample.HumidityPercent());
    Serial.println();
  }
  uint32_t count = samples.AddSample(sample);
  if (Serial) {
    Serial.print("Sample #  ");
    Serial.print(count);
    Serial.print(" written at addr = ");
    Serial.println(samples.GetLastSampleAddr());
  }
}

void loop() {
  static uint8_t minute = 0;

  DateTime utc = rtc.now();

  if (utc.second() == 0) {
    if (utc.minute() > minute) {
      collect_sample(utc);
    }
  }

  delay(1000);
}
