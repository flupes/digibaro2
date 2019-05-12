/*
    Test the RobustIndexes system
*/

/*
compile with:
  pio ci .\test\TestRobustIndices -b zeroUSB -l src -l ..\FastCRC
    -l ..\SPIMemory -l ..\BaroUtils -O "targets=upload"
    -O "build_flags = -DRBI_DEBUG"
monitor with:
  pio device monitor --port COM5 --baud 115200
*/

#include <Arduino.h>

#if defined(ARDUINO_SAMD_ZERO) && defined(SERIAL_PORT_USBVIRTUAL)
#define Serial SERIAL_PORT_USBVIRTUAL
#endif

#include <SPIFlash.h>

#include "robust_flash_indexes.h"

SPIFlash flash(4);

// Just a derived class to allow access to RetrieveLastIndex...
class FlashIndices : public RobustFlashIndexes {
 public:
  FlashIndices() : RobustFlashIndexes(2, 6) {}

  uint32_t LastIndexOutsideBegin() { return RetrieveLastIndex(); }

  void PrintStatus() {
    Serial.print("Current Index = ");
    Serial.println(GetCurrentIndex());
    Serial.print("Current Counter = ");
    Serial.println(GetCurrentCounter());
  }

  void PrintLast() {
    uint32_t last = RetrieveLastIndex();
    Serial.print("Last Index Retrieved = ");
    Serial.print(last);
    Serial.print(" (counter at this location = ");
    Serial.print(GetCounterAt(last));
    Serial.println(")");
  }

  void IncrementAndPrint(uint32_t increment, bool steps = false) {
    Serial.print("Incrementing ");
    Serial.print(increment);
    Serial.println("x...");
    for (uint32_t i = 0; i < increment; i++) {
      uint32_t counter = Increment();
      if (steps) {
        Serial.print("new counter = ");
        Serial.println(counter);
      }
    }
    PrintStatus();
  }
};

FlashIndices indices;

void setup() {
  Serial.begin(115200);
  while (!Serial)
    ;

  Serial.println("Starting Flash");
  flash.begin();

  Serial.print("Erasing first 32K...");
  flash.eraseBlock32K(0);
  Serial.println("Done.");

  Serial.print("Starting indices...");
  indices.begin(&flash);
  Serial.println("Done.");

  Serial.print("Number of usable indexes = ");
  Serial.println(indices.NumberOfUsableIndexes());

  indices.PrintStatus();

  indices.IncrementAndPrint(4, true);

  indices.IncrementAndPrint(1024*3-4);
  indices.PrintLast();

  indices.IncrementAndPrint(4, true);
  indices.PrintLast();

  indices.IncrementAndPrint(1020);
  indices.PrintLast();

  indices.IncrementAndPrint(4, true);
  indices.PrintLast();

  Serial.println("Done with this test.");
  while (1)
    ;
}

void loop() {}