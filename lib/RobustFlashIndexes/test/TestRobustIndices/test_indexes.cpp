/*
    Test the RobustIndexes system
*/

// We use a Zero compatible board
#define Serial SERIAL_PORT_USBVIRTUAL

#include <SPIFlash.h>

// compile with:
// pio ci .\test\TestRobustIndices --board=zeroUSB -l src -l ..\FastCRC -l ..\SPIMemory -O "targets=upload"

// monitor with:
// pio device monitor --port COM5 --baud 115200

#include "robust_flash_indexes.h"

                                     SPIFlash flash(4);

// Just a derived class to allow access to RetrieveLastIndex...
class FlashIndices : public RobustFlashIndexes {
 public:
  FlashIndices() : RobustFlashIndexes(2, 4) {}

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
    Serial.println(last);
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

  Serial.println("Staring Flash");
  flash.begin();

  Serial.print("Erasing first 32K.. .");
  flash.eraseBlock32K(0);

  Serial.println("Starting indices...");
  indices.begin(&flash);

  indices.PrintStatus();

  indices.IncrementAndPrint(4, true);

  indices.IncrementAndPrint(2040);
  indices.PrintLast();

  indices.IncrementAndPrint(1027);
  indices.PrintLast();

  Serial.println("Done with this test.");
  while (1)
    ;
}

void loop() {}