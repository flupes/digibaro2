// compile with:
// pio ci .\test\CollectBaroSamples --board=zeroUSB --lib lib\BME280_driver
// --lib lib\RTClib --lib lib\BaroSample --lib lib\SPIMemory -O "targets=upload"
// --keep-build-dir

// monitor with:
// pio device monitor --port COM5 --baud 115200


#include <Arduino.h>

#if defined(ARDUINO_SAMD_ZERO) && defined(SERIAL_PORT_USBVIRTUAL)
#define Serial SERIAL_PORT_USBVIRTUAL
#endif

#include "RTClib.h"
#include "SPIMemory.h"
#include "baro_sample.h"
#include "bme280_sensor.h"

// Global variables:

// On board SPI Flash Memory
SPIFlash flash(4);

// Real Time Clock
RTC_DS3231 rtc;

// BME 280 Pressure/Temperature/Humidity sensor
Bme280Sensor bme(BME280_I2C_ADDR_SEC);

// address of the next addr where to write sample on flash memory
uint32_t next_sample_addr = 0;

const uint32_t kSampleStart = 16 * KB(4);
const int8_t kTimeZone = -8;

#define PRINT(x) if (Serial) Serial.print(x);

#define PRINTLN(x) if (Serial) Serial.println(x);


uint32_t locate_last_sample() {
  uint32_t addr = kSampleStart;
  uint32_t word = flash.readLong(addr);
  if ( word == 0xFFFFFFFF ) {
    PRINTLN("Memory at start address is ready for write --> start fresh");
    return addr;
  }

  for (uint16_t i=1; i<32767; i++) {
    addr += 8;
    word = flash.readLong(addr);
    if ( word == 0xFFFFFFFF) {
      if (Serial) {
        Serial.print("Last sample found at addr = ");
        Serial.print(addr);
        Serial.print(" --> index = ");
        Serial.println(i);
      }
      return addr;
    }

  }
  return 0;
}

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

  next_sample_addr = locate_last_sample();

  if (!bme.Begin()) {
    PRINTLN("Sensor initialization error!");
    while (1)
      ;
  }
}

void collect_sample(DateTime &dt) {
  bme.PerformMeasurement();

  BaroSample sample(dt.unixtime(), bme.GetPressure() / 100,
                    bme.GetTemperature(), bme.GetHumidity() / 10);

  PackedBaroSample data;
  sample.PackSample(data);

  flash.writeCharArray(next_sample_addr, data, kBaroSampleSize);
  next_sample_addr += 8;

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
    Serial.print(next_sample_addr-8);
    Serial.print(" --> sample # ");
    Serial.println((next_sample_addr-kSampleStart)/8);
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