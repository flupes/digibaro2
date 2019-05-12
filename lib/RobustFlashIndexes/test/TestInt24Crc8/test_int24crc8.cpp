/*
compile with:
  pio ci .\test\TestInt24Crc8 -b zeroUSB -l src -l ..\FastCRC
    -l ..\SPIMemory -O "targets=upload"
*/

#include <Arduino.h>
// #include <unity.h>

#if defined(ARDUINO_SAMD_ZERO) && defined(SERIAL_PORT_USBVIRTUAL)
#define Serial SERIAL_PORT_USBVIRTUAL
#endif

#include "robust_flash_indexes.h"

void test_int24_crc8() {
  uint32_t errors = 0;

  for (uint32_t i = 0; i < 10000; i++) {
    uint32_t code = Int24Crc8::Create(i);
    bool crc_ok = Int24Crc8::Check(code);
    // TEST_ASSERT_TRUE(crc_ok);
    if (!crc_ok) {
      errors++;
      Serial.print("CRC Error for value = ");
      Serial.print(i);
      Serial.print(" | code = ");
      Serial.println(code, HEX);
    }
    uint32_t value = Int24Crc8::Data(code);
    // TEST_ASSERT_EQUAL_INT32(value, i);
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
  Serial.println(" errors.");
}

void setup() {
  Serial.begin(115200);
  uint8_t count = 0;
  while (!Serial && count < 8) {
    delay(1000);
  }

  Serial.println("Starting Int24Crc8 test...");
  test_int24_crc8();
  // UNITY_BEGIN();
  // RUN_TEST(test_int24_crc8);
  // UNITY_END();

  while (1)
    ;
}

void loop() {}
