#include "SPIMemory.h"
#include "baro_sample.h"
#include "generated.h"
#include "print_utils.h"
#include "rotating_samples.h"


/* Compile with:
pio ci .\test\SetSampleData --board=zeroUSB -l ..\BaroUtils -l ..\BaroSample
  -l ..\SPIMemory -l ..\FastCRC -l ..\RobustFlashIndexes -l ..\RotatingSamples
  -O "build_flags = -DDIGI_DEBUG" -O "targets=upload"
*/

/*
SetDataSample starting...
flash initialized...
Usable number of rotating samples = 2816
RotatingSamples::begin()
rotating sample addr start = 1671168
rotating samples sector length = 12
max number of rotating samples = 3072
usable number of rotating samples = 2816
Rotating sample current index = 0
kRobustIndexesSectorStart =4
kRingSamplesSectorStart = 10
Rotating sample new index = 1627
*/

// On board SPI Flash Memory
SPIFlash flash(kMiniUltraProOnBoardChipSelectPin);

// Hour twelthes log to flash
RotatingSamples rotating_samples(flash, 384+18);

void setup() {
  Serial.begin(115200);
  uint8_t count = 0;
  while (!Serial && count < 20) {
    delay(1000);
    count++;
  }

  PRINTLN("SetDataSample starting...");

  if (!flash.begin()) {
    PRINTLN("Flash memory initialization error!");
    while (1)
      ;
  }

  PRINTLN("flash initialized...");

  PRINT("Usable number of rotating samples = ");
  PRINTLN(rotating_samples.GetUsableNumberOfSamples());
  uint32_t rotating_index = rotating_samples.begin();
  PRINT("Rotating sample current index = ");
  PRINTLN(rotating_index);

  PRINT("kRobustIndexesSectorStart =");
  PRINTLN(kRobustIndexesSectorStart);
  PRINT("kRingSamplesSectorStart = ");
  PRINTLN(kRingSamplesSectorStart);

  BaroSample s;
  for (uint32_t i = 0; i < kNumberOfSamples; i++) {
    s.Set(seconds_and_pressure[i][0], seconds_and_pressure[i][1], 25 * 100,
          50 * 100);
    // s.PrettyPrint();
    rotating_samples.AddSample(s);
  }

  PRINT("Rotating sample new index = ");
  PRINTLN(rotating_samples.GetLastSampleIndex());
}

void loop() {}