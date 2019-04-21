// Erase the all section of permanent samples!

// Compile with:
// pio ci .\test\ErasePermanentSamples --board=zeroUSB -l .\lib\SPIMemory -l
// .\lib\BaroUtils -O "targets=upload"

// Monitor with:
// pio device monitor-- port COM5-- baud 11520

#include <Arduino.h>

#if defined(ARDUINO_SAMD_ZERO) && defined(SERIAL_PORT_USBVIRTUAL)
#define Serial SERIAL_PORT_USBVIRTUAL
#endif

#include "SPIMemory.h"
#include "flash_config.h"

    SPIFlash flash(kMiniUltraProOnBoardChipSelectPin);

void setup() {
  Serial.begin(115200);
  while (!Serial)
    ;

  if (!flash.begin()) {
    Serial.println("Flash memory initialization error!");
    while (1)
      ;
  }

  uint32_t start_buffer = kPermanentSamplesSectorStart * KB(4);
  uint32_t end_buffer = start_buffer + kPermanentSamplesSectorStart * KB(4);

  Serial.print("Will erase all the permanent samples from ");
  Serial.print(start_buffer);
  Serial.print(" to ");
  Serial.println(end_buffer);
  Serial.println("Remove power in the next 10s to avoid this destruction!");
  delay(12 * 1000);

  Serial.print("Erasing now... ");
  flash.eraseSection(start_buffer, end_buffer);
  Serial.println("Done.");
  while (1)
    ;
}

void loop() {}  