// compile with:
// pio ci .\test\CollectBaroSamples --board=zeroUSB --lib lib\BME280_driver
// --lib lib\RTClib --lib lib\BaroSample --lib lib\SPIMemory -O "targets=upload"
// --keep-build-dir

// monitor with:
// pio device monitor --port COM5 --baud 115200

#include "print_utils.h"

#include "RTClib.h"
#include "SPIMemory.h"
#include "baro_sample.h"
#include "bme280_sensor.h"
#include "permanent_samples.h"
#include "rotating_samples.h"

// Global variables:

// On board SPI Flash Memory
SPIFlash flash(kMiniUltraProOnBoardChipSelectPin);

// Real Time Clock
RTC_DS3231 rtc;

// BME 280 Pressure/Temperature/Humidity sensor
Bme280Sensor bme(BME280_I2C_ADDR_SEC);

// Hourly log to flash
PermanentSamples permanent_samples(flash);

// Hour twelthes log to flash
RotatingSamples rotating_samples(flash);

const int8_t kTimeZone = -8;

void setup() {
  Serial.begin(115200);
  uint8_t count = 0;
  while (!Serial && count < 20) {
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

  PRINT("Usable number of rotating samples = ");
  PRINTLN(rotating_samples.GetUsableNumberOfSamples());
  uint32_t rotating_index = rotating_samples.begin();
  PRINT("Rotating sample current index = ");
  PRINTLN(rotating_index);

  PRINT("Max number of samples on permanent flash = ");
  PRINTLN(permanent_samples.GetMaxNumberOfSamples());
  uint32_t hourly_count = permanent_samples.begin();
  PRINT("Current number of samples on permanent flash = ");
  PRINTLN(hourly_count);
}

void collect_sample(DateTime &dt) {
  bme.PerformMeasurement();

  BaroSample sample(dt.unixtime(), bme.GetPressure() / 100,
                    bme.GetTemperature(), bme.GetHumidity() / 10);

  uint32_t index = rotating_samples.AddSample(sample);

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
    Serial.print("Sample written to rotating flash with index # ");
    Serial.println(index);
  }
}

void hourly_average(DateTime &dt) {
  uint32_t current_seconds = dt.unixtime();
  uint32_t count = 0;
  uint32_t samples_per_hour = kPermanentPeriodSeconds / kSecondsResolution;
  // Serial.print("hourly_average : samples_per_hour = ");
  // Serial.print(samples_per_hour);
  // Serial.print(" --> current_seconds = ");
  // Serial.println(current_seconds);
  uint32_t pressure = 0;
  uint32_t humidity = 0;
  uint32_t temperature = 0;
  BaroSample sample;
  uint32_t index = rotating_samples.GetIndexIterator(samples_per_hour);
  for (uint32_t i = 0; i < samples_per_hour; i++) {
    sample = rotating_samples.GetSampleAtIndex(index);
    uint32_t sample_seconds = sample.GetTimestamp();
    // Serial.print("GetSampleAtIndex with index = ");
    // Serial.print(index);
    // Serial.print(" --> sample_seconds = ");
    // Serial.println(sample_seconds);
    if ((current_seconds - sample_seconds) <= kPermanentPeriodSeconds) {
      // Serial.print("pressure Pa = ");
      // Serial.print(sample.GetPressure());
      // Serial.print(" / pressure mbar = ");
      // Serial.println(sample.PressureMilliBar());
      pressure += sample.GetPressure();
      humidity += sample.GetHumidity();
      temperature += sample.GetTemperature();
      count++;
    }
    index = rotating_samples.GetNextIndex();
  }
  pressure /= count;
  humidity /= count;
  temperature /= count;
  sample.Set(current_seconds, pressure, temperature, humidity, kTimeZone);
  if (Serial) {
    char buffer[64];
    DateTime local = dt.getLocalTime(kTimeZone);
    local.toString(buffer);
    Serial.print("hourly_average performed at time = ");
    Serial.print(buffer);
    Serial.print(" over # of samples = ");
    Serial.println(count);
    sample.PrettyPrint();
  }
  uint32_t nbs = permanent_samples.AddSample(sample);
  PRINT("Saved hourly sample # ");
  PRINTLN(nbs);
}

void loop() {
  static uint8_t minute = 0;
  static uint32_t seconds = k2019epoch;

  DateTime utc = rtc.now();

  if (utc.second() == 0 && utc.minute() != minute) {
    collect_sample(utc);

    if ( utc.minute() % (kPermanentPeriodSeconds/kSecondsResolution) == 0) {
      hourly_average(utc);
    }

    minute = utc.minute();
  }

  delay(100);
}
