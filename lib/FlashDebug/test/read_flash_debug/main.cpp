#include <Arduino.h>
#include "SPIFlash.h"
#include "RTClib.h"

#include "flash_config.h"
#include "flash_debug.h"

// pio ci test\read_flash_debug -b zeroUSB -l .\src -l ..\RTCZero
//   -l ..\RTClib -l ..\SPIMemory -O "targets=upload"

#if defined(ARDUINO_SAMD_ZERO) && defined(SERIAL_PORT_USBVIRTUAL)
#define Serial SERIAL_PORT_USBVIRTUAL
#endif

SPIFlash spi_flash(kMiniUltraProOnBoardChipSelectPin);

FlashDebug debug;

void setup() {
  Serial.begin(115200);
  while (!Serial)
    ;

  spi_flash.begin();

  uint32_t addr = kDebugSectorStart * KB(4);
  while (addr < 512 * KB(4)) {
    uint32_t seconds = spi_flash.readLong(addr);
    if (seconds == 0xFFFFFFFF) break;
    DateTime dt(seconds);
    addr += 4;
    uint8_t msg_type = spi_flash.readByte(addr++);
    uint8_t msg_value = spi_flash.readByte(addr++);
    int16_t msg_payload = spi_flash.readShort(addr);
    addr += 2;
    char buffer[24];
    dt.toString(buffer);
    Serial.print(buffer);
    Serial.print(" ");
    Serial.print(msg_type);
    Serial.print(" ");
    Serial.print(msg_value);
    Serial.print(" ");
    Serial.print(msg_payload);
    Serial.println();
  }

}

void loop() {}
