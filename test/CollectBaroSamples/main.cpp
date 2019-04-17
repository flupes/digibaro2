// compile with:
// pio ci .\test\CollectBaroSamples --board=zeroUSB
// -l lib\BME280_driver -l lib\RTClib -l lib\BaroSample -l lib\SPIMemory -l src
// -O "targets=upload" --keep-build-dir

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
    while (1);
  }

  if (!bme.Begin()) {
    PRINTLN("Sensor initialization error!");
    while (1)
      ;
  }

  uint32_t addr = samples.begin();
  PRINT("Last sample addr = ");
  PRINT(addr);
  PRINT(" --> number of existing samples = ");
  PRINTLN(samples.GetNumberOfSamples());
}

void collect_sample(DateTime &dt) {
  bme.PerformMeasurement();

  BaroSample sample(dt.unixtime(), bme.GetPressure() / 100,
                    bme.GetTemperature(), bme.GetHumidity() / 10);

  samples.AddSample(sample);

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
    Serial.print("Sample written at addr = ");
    Serial.print(samples.GetLastSampleAddr());
    Serial.print(" --> sample # ");
    Serial.println(samples.GetNumberOfSamples());
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
