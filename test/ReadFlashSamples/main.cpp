// compile with:
// pio ci .\test\ReadFlashSamples --board=zeroUSB
// -l lib\RTClib -l lib\BaroSample -l lib\SPIMemory -l src -O "targets=upload"

// monitor with:
// pio device monitor --port COM5 --baud 115200

#include "print_utils.h"

#include "SPIMemory.h"
#include "baro_sample.h"
#include "permanent_samples.h"

// Global variables:

// On board SPI Flash Memory
SPIFlash flash(kMiniUltraProOnBoardChipSelectPin);

PermanentSamples samples(flash);

const int8_t kTimeZone = -8;

void setup() {
  Serial.begin(115200);
  while (!Serial)
    ;

  Serial.println("ReadFlashSamples Starting...");

  if (!flash.begin()) {
    Serial.println("Flash memory initialization error!");
    while (1)
      ;
  }

  uint32_t nb_samples = samples.begin();

  Serial.println("unix_seconds,hour_twelfth,pressure,temperature,humidity");

  for (uint32_t i = 0; i < nb_samples; i++) {
    BaroSample sample = samples.GetSampleWithIndex(i);
    Serial.print(sample.GetTimestamp());
    Serial.print(",");
    Serial.print(sample.GetTimecount());
    Serial.print(",");
    Serial.print(sample.PressureMilliBar());
    Serial.print(",");
    Serial.print(sample.TemperatureDegCelcius());
    Serial.print(",");
    Serial.print(sample.HumidityPercent());
    Serial.println();
  }

  Serial.println("Done.");
  while (1)
    ;
}

void loop() {}