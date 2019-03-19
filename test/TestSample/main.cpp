#include <Arduino.h>


// We use a Zero compatible board
#define Serial SerialUSB

#include "baro_sample.h"

// compile with:
// pio ci .\test\TestSample --board=zeroUSB --lib src --lib lib/FastCRC --project-option="targets=upload" --keep-build-dir

// monitor with:
// pio device monitor --port COM5 --baud 115200


void setup() {
  Serial.begin(115200);
  while (!Serial)
    ;

  Serial.println("Starting...");

  BaroSample in(1558569600, 110000, 8000, 10000);
  in.Print();
  in.Inspect();

  Serial.println();
  
  BaroSample out(in.data_);
  out.Print();
  out.Inspect();
}

void loop() {}
