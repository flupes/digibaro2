
#include "Adafruit_GFX.h"
#include "epd4in2.h"

#include "RTClib.h"
#include "SPIMemory.h"

#include "baro_sample.h"
#include "flash_config.h"
#include "graph_samples.h"
#include "print_utils.h"
#include "rotating_samples.h"

/* Compile with:
pio ci .\test --board=zeroUSB -l src -l ..\BaroUtils -l ..\BaroSample
  -l ..\RotatingSamples -l ..\RobustFlashIndexes -l ..\DisplaySamples
  -l ..\RTClib  -l ..\SPIMemory -l ..\FastCRC -l ..\Labels
  -l ..\Adafruit-GFX-Library -l ..\epd42
  -O "build_flags = -DDIGI_TESTING -DDIGI_DEBUG" -O "targets=upload"
*/

// On board SPI Flash Memory
SPIFlash flash(kMiniUltraProOnBoardChipSelectPin);

// Hour twelthes log to flash
RotatingSamples rotating_samples(flash, 492);

// Two buffers with different periods
GraphSamples daily_buffer(5 * 60);
GraphSamples weekly_buffer(20 * 60);

// e-paper display
Epd epd;

// canvas to draw on
GFXcanvas1 canvas(400, 300);

#define COLORED 0
#define UNCOLORED 1

const uint8_t kRtcPowerPin = 6;

void setup() {
  pinMode(kRtcPowerPin, OUTPUT);
  digitalWrite(kRtcPowerPin, HIGH);
  delay(500);

  Serial.begin(115200);
  uint8_t count = 0;
  while (!Serial && count < 20) {
    delay(1000);
    count++;
  }

  PRINTLN("graph_test starting...");

  if (!flash.begin()) {
    PRINTLN("Flash memory initialization error!");
    while (1)
      ;
  }

  PRINTLN("Init e-Paper...");

  if (epd.Init(false) != 0) {
    PRINTLN("e-Paper init failed");
    return;
  }

  uint32_t last_index = rotating_samples.begin();
  PRINT("last rotating sample index = ");
  PRINTLN(last_index);

  daily_buffer.Fill(&rotating_samples, 1556427000);
  weekly_buffer.Fill(&rotating_samples, 1556427000);

  PRINTLN("Buffers filled!");
}

void loop() {
  uint32_t start = millis();
  uint32_t begining = start;
  epd.Init(false);
  uint32_t init_ms = millis();
  epd.ConfigAndSendOldBuffer(canvas.getBuffer());
  uint32_t oldbuf_ms = millis();
  daily_buffer.Draw(canvas);
  uint32_t draw_ms = millis();
  // epd.SetPartialWindow(canvas.getBuffer(), 0, 0, 400, 300);
  // epd.DisplayFrame();
  epd.SendNewBufferAndRefresh(canvas.getBuffer());
  uint32_t newbuf_ms = millis();
  epd.Sleep();
  uint32_t sleep_ms = millis();
  PRINTLN("TIMINGS (ms)");
  PRINT("  reset and init     = ");
  PRINTLN(init_ms-start);
  PRINT("  old data transfer  = ");
  PRINTLN(oldbuf_ms-init_ms);
  PRINT("  graph drawing      = ");
  PRINTLN(draw_ms-oldbuf_ms);
  PRINT("  new data + refresh = ");
  PRINTLN(newbuf_ms-draw_ms);
  PRINT("  display to sleep   = ");
  PRINTLN(sleep_ms-newbuf_ms);
  PRINT("  total              = ");
  PRINTLN(sleep_ms-start);


  delay(1000 * 20);
  epd.Init(false);
  epd.ConfigAndSendOldBuffer(canvas.getBuffer());
  weekly_buffer.Draw(canvas);
  // epd.SetPartialWindow(canvas.getBuffer(), 0, 0, 400, 300);
  epd.SendNewBufferAndRefresh(canvas.getBuffer());
  // epd.DisplayFrame();
  epd.Sleep();
  delay(1000 * 20);
}
