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
  PackedBaroSample buffer;

  BaroSample inp(1558569600, 60000, 8191, 5000);
  inp.Print();
  inp.Inspect();

  Serial.println();
  inp.GetRawData(buffer);

  BaroSample outp(buffer);
  outp.Print();
  outp.Inspect();

  Serial.println();
  Serial.println();

  BaroSample inn(1558569600, 125000, -8192, 10000);
  inn.Print();
  inn.Inspect();

  Serial.println();
  inn.GetRawData(buffer);

  BaroSample outn(buffer);
  outn.Print();
  outn.Inspect();
}

void loop() {}

/*
  int16 --> uint16 --> int16 conversions

  Serial.println("int16");
  int16_t i0 = 32766;
  Serial.print(i0, HEX);
  Serial.print(" -> ");
  Serial.println(i0, DEC);

  int16_t i1 = 32767;
  Serial.print(i1, HEX);
  Serial.print(" -> ");
  Serial.println(i1, DEC);
  
  int16_t i2 = 32768;
  Serial.print(i2, HEX);
  Serial.print(" -> ");
  Serial.println(i2, DEC);

  int16_t i3 = -32767;
  Serial.print(i3, HEX);
  Serial.print(" -> ");
  Serial.println(i3, DEC);
  
  int16_t i4 = -32768;
  Serial.print(i4, HEX);
  Serial.print(" -> ");
  Serial.println(i4, DEC);

  Serial.println("uint16");

  uint16_t u0 = (uint16_t)(i0);
  Serial.print(u0, HEX);
  Serial.print(" -> ");
  Serial.println(u0, DEC);
 
  uint16_t u1 = (uint16_t)(i1);
  Serial.print(u1, HEX);
  Serial.print(" -> ");
  Serial.println(u1, DEC);
 
  uint16_t u3 = (uint16_t)(i3);
  Serial.print(u3, HEX);
  Serial.print(" -> ");
  Serial.println(u3, DEC);
 
  uint16_t u4 = (uint16_t)(i4);
  Serial.print(u4, HEX);
  Serial.print(" -> ");
  Serial.println(u4, DEC);
 
  Serial.println("back to int16");
  int16_t b1 = (int16_t)(u1);
  Serial.print(b1, HEX);
  Serial.print(" -> ");
  Serial.println(b1, DEC);
  
  int16_t b4 = (int16_t)(u4);
  Serial.print(b4, HEX);
  Serial.print(" -> ");
  Serial.println(b4, DEC);

int16
7FFE -> 32766
7FFF -> 32767
FFFF8000 -> -32768
FFFF8001 -> -32767
FFFF8000 -> -32768
uint16
7FFE -> 32766
7FFF -> 32767
8001 -> 32769
8000 -> 32768
back to int16
7FFF -> 32767
FFFF8000 -> -32768

  */