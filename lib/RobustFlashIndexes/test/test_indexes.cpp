/*
    Test the RobustIndexes system
*/

// We use a Zero compatible board
#define Serial SERIAL_PORT_USBVIRTUAL

#include <SPIFlash.h>

// compile with:
// pio ci .\test --board=zeroUSB -l src -l ..\FastCRC -l ..\SPIMemory -O "targets=upload"
// monitor with:
// pio device monitor --port COM5 --baud 115200

#include "robust_flash_indexes.h"

SPIFlash flash(4);

RobustFlashIndexes indices(1, 4);

void setup() {
  Serial.begin(115200);
  while (!Serial)
    ;

  Serial.println("Staring Flash");
  flash.begin();

  Serial.print("Erasing first 32K.. .");
  flash.eraseBlock32K(0);

  Serial.println("Starting indices...");
  indices.begin(&flash);

  for (int x = 0; x < 2; x++) {
    Serial.print("Current Index = ");
    Serial.println(indices.GetCurrentIndex());

    Serial.println("Incrementing 6x ...");
    for (int i = 0; i < 6; i++) {
      uint32_t counter = indices.Increment();
      Serial.print("new counter = ");
      Serial.println(counter);
    }

    Serial.println("Incrementing 2040x ...");
    for (int i = 0; i < 2040; i++) {
      indices.Increment();
    }
    Serial.print("Current Index = ");
    Serial.println(indices.GetCurrentIndex());

    Serial.println("Incrementing 6x ...");
    for (int i = 0; i < 6; i++) {
      uint32_t counter = indices.Increment();
      Serial.print("new counter = ");
      Serial.println(counter);
    }

    Serial.print("Current Index = ");
    Serial.println(indices.GetCurrentIndex());
    Serial.print("Current Counter = ");
    Serial.println(indices.GetCurrentCounter());

    Serial.println("Done with this test.");
    while (1);
  }
}

void loop() {  }