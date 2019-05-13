#include "SPIMemory.h"
#include "baro_sample.h"
#include "display_samples.h"
#include "print_utils.h"
#include "rotating_samples.h"

// PIO stupidity again: does not resolve the library, even when specified on
// command line!
#include "Adafruit_GFX.h"

/* Compile with:
pio ci .\test\ShortTest --board=zeroUSB -l src -l ..\BaroUtils -l ..\BaroSample
  -l ..\SPIMemory -l ..\FastCRC -l ..\RobustFlashIndexes -l ..\RotatingSamples
  -l ..\Adafruit-GFX-Library -O "build_flags = -DDIGI_DEBUG" -O "targets=upload"
*/

const uint32_t kStartTestSector = 384;
const uint32_t kSamplesPeriod = 100;

// On board SPI Flash Memory
SPIFlash flash(kMiniUltraProOnBoardChipSelectPin);

// Use a memory map outside of the normal samples
RotatingSamples rotating_samples(flash, kStartTestSector);

// Two buffers with different periods
DisplaySamples daily_buffer(kSamplesPeriod, 6);
DisplaySamples weekly_buffer(3 * kSamplesPeriod, 6);

void setup() {
  Serial.begin(115200);
  while (!Serial)
    ;

  PRINTLN("short_display_samples starting...");

  if (!flash.begin()) {
    PRINTLN("Flash memory initialization error!");
    while (1)
      ;
  }

  // Wipe out flash for this test
  PRINTLN("Wipe out flash section...");
  flash.eraseSection(rotating_samples.GetSectorStart() * KB(4),
                     rotating_samples.GetSectorsLength() * KB(4));

  uint32_t last_index = rotating_samples.begin();
  PRINT("last rotating sample index = ");
  PRINTLN(last_index);

  // Populate with some controlled samples
  uint32_t start_ts = k2019epoch + 12 * 3600;

  PRINTLN("Writing test samples to flash...");
  uint32_t ts = start_ts;
  uint32_t last_ts = start_ts;
  uint32_t humidity = 0;
  for (size_t i = 0; i < 24; i++) {
    BaroSample s(ts, kPressureOffsetPa, 0, humidity);
    if (i == 19 || i == 20 || (i > 10 && i < 18)) {
      PRINT("Skip sample with ts=");
      PRINTLN(ts);
    } else {
      rotating_samples.AddSample(s);
    }
    last_ts = ts;
    ts += kSamplesPeriod;
    humidity += 10;
  }

  PRINTLN(
      "<======== Filling buffer with reference time older than the oldest "
      "sample on buffer")
  uint32_t daily_count =
      daily_buffer.Fill(&rotating_samples, k2019epoch + 10 * 3600);
  uint32_t weekly_count =
      weekly_buffer.Fill(&rotating_samples, k2019epoch + 10 * 3600);
  if (daily_count == 0 && weekly_count == 0) {
    PRINTLN("--------> PASS (samples added = 0)");
  } else {
    PRINT("********> FAILED : daily_count=");
    PRINT(daily_count);
    PRINT(" / weekly_count=");
    PRINTLN(weekly_count);
  }

  PRINTLN("<======== Filling buffer with reference time matching last sample");
  daily_count = daily_buffer.Fill(&rotating_samples, last_ts, HUMIDITY);
  daily_buffer.Print();
  weekly_count = weekly_buffer.Fill(&rotating_samples, last_ts, HUMIDITY);
  weekly_buffer.Print();
  if (daily_count == 4 && weekly_count == 4) {
    PRINTLN("--------> PASS (samples added == 4)");
  } else {
    PRINT("********> FAILED : daily_count=");
    PRINT(daily_count);
    PRINT(" / weekly_count=");
    PRINTLN(weekly_count);
  }

  PRINTLN("<======== Appending data to the display buffer");
  size_t errors = 0;
  for (size_t i = 0; i < 2; i++) {
    BaroSample s(ts, kPressureOffsetPa, 0, humidity);
    daily_buffer.AppendData(s);
    humidity += 10;
    ts += kSamplesPeriod;
  }
  int16_t result1[] = {-32768, 210, 220, 230, 240, 250};
  for (int i=0; i<6; i++) {
    if (daily_buffer.Data(i) != result1[i]) errors++;
  }
  daily_buffer.Print();
  ts += 2 * kSamplesPeriod;
  humidity += 20;
  BaroSample s(ts, kPressureOffsetPa, 0, humidity);
  daily_buffer.AppendData(s);
  int16_t result2[] = {230, 240, 250, -32768, -32768, 280};
  for (int i = 0; i < 6; i++) {
    if (daily_buffer.Data(i) != result2[i]) errors++;
  }
  daily_buffer.Print();
  if ( errors == 0 ) {
    PRINTLN("--------> PASS (errors == 0)");
  }
  else {
    PRINT("********> FAILED : errors=");
    PRINTLN(errors);
  }

  PRINTLN(
      "<======== Filling buffer with reference time more recent than last "
      "sample on flash");
  daily_count =
      daily_buffer.Fill(&rotating_samples, start_ts + 2 * kSamplesPeriod, HUMIDITY);
  daily_buffer.Print();
  weekly_count =
      weekly_buffer.Fill(&rotating_samples, start_ts + 6 * kSamplesPeriod, HUMIDITY);
  weekly_buffer.Print();
  if (daily_count == 3 && weekly_count == 3) {
    PRINTLN("--------> PASS (samples added == 3)");
  } else {
    PRINT("********> FAILED : daily_count=");
    PRINT(daily_count);
    PRINT(" / weekly_count=");
    PRINTLN(weekly_count);
  }
}

void loop() {}
