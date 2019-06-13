#include <Arduino.h>
#include "RTCZero.h"
#include "SPIFlash.h"


#include "flash_config.h"
#include "flash_debug.h"

// pio ci test\write_debug2flash -b zeroUSB -l .\src
//   -l ..\RTCZero -l ..\SPIMemory -O "targets=upload"

#if defined(ARDUINO_SAMD_ZERO) && defined(SERIAL_PORT_USBVIRTUAL)
#define Serial SERIAL_PORT_USBVIRTUAL
#endif

SPIFlash spi_flash(kMiniUltraProOnBoardChipSelectPin);

RTCZero onboard_rtc;

FlashDebug debug;

static uint8_t conv2d(const char* p) {
  uint8_t v = 0;
  if ('0' <= *p && *p <= '9') v = *p - '0';
  return 10 * v + *++p - '0';
}

void SetClock(const char* date, const char* time) {
  // sample input: date = "Dec 26 2009", time = "12:34:56"
  uint8_t yOff = conv2d(date + 9);
  // Jan Feb Mar Apr May Jun Jul Aug Sep Oct Nov Dec
  uint8_t m;
  switch (date[0]) {
    case 'J':
      m = (date[1] == 'a') ? 1 : ((date[2] == 'n') ? 6 : 7);
      break;
    case 'F':
      m = 2;
      break;
    case 'A':
      m = date[2] == 'r' ? 4 : 8;
      break;
    case 'M':
      m = date[2] == 'r' ? 3 : 5;
      break;
    case 'S':
      m = 9;
      break;
    case 'O':
      m = 10;
      break;
    case 'N':
      m = 11;
      break;
    case 'D':
      m = 12;
      break;
  }
  uint8_t d = conv2d(date + 4);
  uint8_t hh = conv2d(time);
  uint8_t mm = conv2d(time + 3);
  uint8_t ss = conv2d(time + 6);
  Serial.print(yOff);
  Serial.print("-");
  Serial.print(m);
  Serial.print("-");
  Serial.print(d);
  Serial.print(" ");
  Serial.print(hh);
  Serial.print(":");
  Serial.print(mm);
  Serial.print(":");
  Serial.print(ss);
  Serial.println();
  onboard_rtc.setDate(d, m, yOff);
  onboard_rtc.setTime(hh, mm, ss);
}

void setup() {
  Serial.begin(115200);
  while (!Serial)
    ;

  spi_flash.begin();

  uint32_t start = debug.SetFlash(&spi_flash);
  Serial.print("debug addr start = ");
  Serial.println(start);
  
  onboard_rtc.begin();
  SetClock(__DATE__, __TIME__);

  debug.SetRTC(&onboard_rtc);

  Serial.println("writting some debug messages to flash");
  debug.Message(FlashDebug::BOOT);

  for (uint8_t i = 0; i < 4; i++) {
    debug.Message(FlashDebug::STEP, i, (int16_t)i * 1000 - 500);
  }

  Serial.println("done.");
}

void loop() {}
