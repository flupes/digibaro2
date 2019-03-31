// compile with:
// pio ci .\test\ReadFlashSamples --board=zeroUSB --lib lib\RTClib --lib lib\BaroSample --lib lib\SPIMemory -O "targets=upload"

// monitor with:
// pio device monitor --port COM5 --baud 115200


#include <Arduino.h>

#if defined(ARDUINO_SAMD_ZERO) && defined(SERIAL_PORT_USBVIRTUAL)
#define Serial SERIAL_PORT_USBVIRTUAL
#endif

#include "SPIMemory.h"
#include "baro_sample.h"

// Global variables:

// On board SPI Flash Memory
SPIFlash flash(4);

const uint32_t kSampleStart = 16 * KB(4);
const int8_t kTimeZone = -8;

void setup() {
  Serial.begin(115200);
  while (!Serial) ;

 Serial.println("ReadFlashSamples Starting...");

  if (!flash.begin()) {
    Serial.println("Flash memory initialization error!");
    while (1);
  }

  PackedBaroSample data;

  Serial.println("unix_seconds, hour_twelfth, pressure, temperature, humidity");

  uint32_t addr = kSampleStart;
  uint32_t word = 0;
  for (uint16_t i=0; i<32767; i++) {
    word = flash.readLong(addr);
    if ( word == 0xFFFFFFFF) {
        Serial.print("Last sample reached at = ");
        Serial.print(addr);
        Serial.print(" --> index = ");
        Serial.println(i);
        break;
    }
    flash.readCharArray(addr, data, kBaroSampleSize);
    BaroSample sample(data);
    Serial.print(sample.GetTimestamp());
    Serial.print(", ");
    Serial.print(sample.GetTimecount());
    Serial.print(", ");
    Serial.print(sample.PressureMilliBar());
    Serial.print(", ");
    Serial.print(sample.TemperatureDegCelcius());
    Serial.print(", ");
    Serial.print(sample.HumidityPercent());
    Serial.println();

    addr += 8;
  }

 Serial.println("Done: spinning for ever...");
  while(1);
 }

void loop() {

}