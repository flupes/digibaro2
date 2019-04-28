#include "SPIMemory.h"
#include "baro_sample.h"
#include "generated.h"
#include "print_utils.h"
#include "rotating_samples.h"


/* Compile with:
pio ci .\test\SetSampleData --board=zeroUSB -l .\lib\BaroUtils
  -l .\lib\BaroSample -l .\lib\SPIMemory -l .\lib\FastCRC
  -l .\lib\RobustFlashIndexes -l .\lib\RotatingSamples
  -O "build_flags = -DDIGI_TESTING -DDIGI_DEBUG" -O "targets=upload"
*/

/*
SetDataSample starting...
flash initialized...
Usable number of rotating samples = 2816
RotatingSamples::begin()
rotating sample addr start = 1073152
max number of rotating samples = 3072
RobustFlashIndexes::begin()
nb_sector:          3
nb_indexes:         3072
indexes_start[0]:   1048576
indexes_start[1]:   1060864
RobustFlashIndexes::RetrieveLastIndex()
Indexes not initialized.
RobustFlashIndexes::InitializeMemory()
Starting to erase sectors... Done.
current index = 0
---- END ---- RobustFlashIndexes::begin()
Rotating sample current index = 0
kRobustIndexesSectorStart =256
kRingSamplesSectorStart = 262
Increment : indexes not initialized yet, starting at index = 0
RobustFlashIndexes entering new sector for index = 0
Double write(1) : addr1=1048576 / addr2=1060864
RotatingSamples::AddSample entering new sector : Sector seems already
initialized.
Increment : new index = 1
Double write(2) : addr1=1048580 / addr2=1060868

Increment : new index = 1727
Double write(1728) : addr1=1055484 / addr2=1067772
Rotating sample new index = 1727
*/

// On board SPI Flash Memory
SPIFlash flash(kMiniUltraProOnBoardChipSelectPin);

// Hour twelthes log to flash
RotatingSamples rotating_samples(flash);

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