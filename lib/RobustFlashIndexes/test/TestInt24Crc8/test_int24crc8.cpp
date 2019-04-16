// Compile with:
// pio ci .\test\TestInt24Crc8 --board=zeroUSB -l src -l ..\FastCRC -l ..\SPIMemory -O "targets=upload"

#include <Arduino.h>

#if defined(ARDUINO_SAMD_ZERO) && defined(SERIAL_PORT_USBVIRTUAL)
#define Serial SERIAL_PORT_USBVIRTUAL
#endif

#include "robust_flash_indexes.h"

void setup() {
  Serial.begin(115200);
  while (!Serial)
    ;
  Serial.println("Starting Int24Crc8 test...");

  uint32_t errors = 0;

  for (uint32_t i = 0; i < 10000; i++) {
    uint32_t code = Int24Crc8::Create(i);
    bool crc_ok = Int24Crc8::Check(code);
    if (!crc_ok) {
      errors++;
      Serial.print("CRC Error for value = ");
      Serial.print(i);
      Serial.print(" | code = ");
      Serial.println(code, HEX);
    }
    uint32_t value = Int24Crc8::Data(code);
    if (value != i) {
      errors++;
      Serial.print("Int24 Decoding Error for value = ");
      Serial.print(i);
      Serial.print(" | code = ");
      Serial.println(code, HEX);
    }
  }
  Serial.print("Test Completed with ");
  Serial.print(errors);
  Serial.println(" erros.");

  while (1);
}

void loop() {}
