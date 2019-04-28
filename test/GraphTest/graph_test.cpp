#include "RTClib.h"
#include "SPIMemory.h"
#include "baro_sample.h"
#include "print_utils.h"
#include "rotating_samples.h"

/* Compile with:
pio ci .\test\GraphTest --board=zeroUSB -l .\lib\BaroUtils
  -l .\lib\BaroSample -l .\lib\SPIMemory -l .\lib\FastCRC
  -l .\lib\RobustFlashIndexes -l .\lib\RotatingSamples  -l .\lib\RTClib
  -O "build_flags = -DDIGI_TESTING -DDIGI_DEBUG" -O "targets=upload"
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

  PRINTLN("SetDataSample stating...");

  if (!flash.begin()) {
    PRINTLN("Flash memory initialization error!");
    while (1)
      ;
  }

  PRINT("kRobustIndexesSectorStart = ");
  PRINTLN(kRobustIndexesSectorStart);

  uint32_t last_index = rotating_samples.begin();
  PRINT("last rotating sample index = ");
  PRINTLN(last_index);
  BaroSample last_sample = rotating_samples.GetSampleAtIndex(last_index);
  DateTime last_ts = DateTime(last_sample.GetTimestamp());
  char buffer[64];
  last_ts.toString(buffer);
  PRINT("Last sample collected at time = ");
  PRINTLN(buffer);

  uint32_t iter = rotating_samples.GetIndexIterator(12);
  while (iter != kInvalidInt24) {
    BaroSample s = rotating_samples.GetSampleAtIndex(iter);
    s.PrettyPrint();
    iter = rotating_samples.GetNextIndex();
  }

}

void loop() {}
