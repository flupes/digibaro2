#include "SPIMemory.h"
#include "baro_sample.h"
#include "rotating_samples.h"
#include "display_samples.h"
#include "print_utils.h"

/* Compile with:
pio ci .\test --board=zeroUSB -l src -l ..\BaroUtils -l ..\BaroSample -l
  ..\SPIMemory -l ..\FastCRC -l ..\RobustFlashIndexes -l ..\RotatingSamples
  -O "build_flags = -DDIGI_TESTING -DDIGI_DEBUG" -O "targets=upload"
*/

// On board SPI Flash Memory
SPIFlash flash(kMiniUltraProOnBoardChipSelectPin);

// Hour twelthes samples on flash
RotatingSamples rotating_samples(flash);

// Two buffers with different periods
DisplaySamples daily_buffer(5 * 60);
DisplaySamples weekly_buffer(20 * 60);

void setup() {
  Serial.begin(115200);
  while (!Serial)
    ;

  PRINTLN("test_display_samples starting...");

  if (!flash.begin()) {
    PRINTLN("Flash memory initialization error!");
    while (1)
      ;
  }

  uint32_t last_index = rotating_samples.begin();
  PRINT("last rotating sample index = ");
  PRINTLN(last_index);
  
  uint32_t start = millis();

  uint32_t daily_length = daily_buffer.Begin(&rotating_samples);
  uint32_t weekly_length = weekly_buffer.Begin(&rotating_samples);

  uint32_t stop = millis();
  PRINT("Reading from flash time (ms) = ");
  PRINTLN(stop - start);

  PRINT("daily_buffer length = ");
  PRINTLN(daily_length);
  PRINT("weekly_buffer length = ");
  PRINTLN(weekly_length);

  uint32_t index = daily_buffer.BufferStartIndex();
  PRINT("start of buffer = ");
  PRINTLN(index);

  for (size_t s=0; s<8; s++) {
    daily_buffer.AppendData((s+1)*10);
  }

  index = daily_buffer.GetLastIndex();
  index = daily_buffer.IndexOffset(index, -50);

  for (size_t i=0; i<50; i++) {
    PRINTLN(daily_buffer.DataAtIndex(index));
    index = daily_buffer.IndexAfter(index);
  }  

}

void loop() {}
