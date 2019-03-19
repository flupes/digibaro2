#include <Arduino.h>
#include <stdint.h>

/**
 * Illustrate how the compiler packs the data structure for the SAMD architecture.
 * 
 * Unfortunately, not smart enough for this application: 12 bytes are used rather than 8.
 * It is probably due to the 32 bit architecture + little endian , and the packing would
 * actually work correctly on a 8 bit microcontroller.
 */

// We use a Zero compatible board
#define Serial SerialUSB

// compile with:
// pio ci .\test\PackedBitField --board=zeroUSB --project-option="targets=upload" --keep-build-dir

// monitor with:
// pio device monitor --port COM5 --baud 115200

struct DataSample {
  uint32_t minutes : 24;
  int16_t temperature : 14;
  uint16_t humidity : 10;
  uint16_t pressure;
};

void print(const char *name, const DataSample &data) {
  Serial.println(name);
  Serial.print("  size : ");
  Serial.println(sizeof(data));
  Serial.print("  minutes     = ");
  Serial.println(data.minutes);
  Serial.print("  pressure    = ");
  Serial.println(data.pressure);
  Serial.print("  temperature = ");
  Serial.println(data.temperature);
  Serial.print("  humidity    = ");
  Serial.println(data.humidity);
}


void inspect(const char* name, const DataSample &data) {
  Serial.println(name);
  Serial.print("  size : ");
  Serial.println(sizeof(data));
  Serial.print("  minutes     = ");
  Serial.println(data.minutes, HEX);
  Serial.print("  pressure    = ");
  Serial.println(data.pressure, HEX);
  Serial.print("  temperature = ");
  Serial.println(data.temperature, HEX);
  Serial.print("  humidity    = ");
  Serial.println(data.humidity, HEX);
  Serial.print("       memory = ");
  uint8_t *ptr = (uint8_t *)(&data);
  for (uint8_t i=0; i<sizeof(data); i++) {
    Serial.print(*ptr++, HEX);
    Serial.print(" ");
  }
  Serial.println();
}

void setup() {
  Serial.begin(115200);
  while (!Serial)
    ;

  DataSample d1;
  d1.minutes = 16E6;
  d1.pressure = 65000;
  d1.temperature = -8190;
  d1.humidity = 1020;
  print("d1 (in range)", d1);

  DataSample d2;
  d2.minutes = 16777218;
  d2.pressure = 65538;
  d2.temperature = -8194;
  d2.humidity = 1028;
  print("d2 (overflow)", d2);

  DataSample d3;
  d3.minutes = 0xABCDEF;
  d3.pressure = 0x7777;
  d3.temperature = 0x1888;
  d3.humidity = 0x299;
  inspect("d3", d3);

}

void loop() {}

/*

Results of different packing order (SAMD12):

  uint32_t minutes: 24;
  uint16_t pressure;
  int16_t temperature : 14;
  uint16_t humidity : 10;

d1 (in range)
  size : 12
  minutes     = 16000000
  pressure    = 65000
  temperature = -8190
  humidity    = 1020
d2 (overflow)
  size : 12
  minutes     = 2
  pressure    = 2
  temperature = 8190
  humidity    = 4
d3
  size : 12
  minutes     = ABCDEF
  pressure    = 7777
  temperature = 1888
  humidity    = 299
       memory = EF CD AB 0 77 77 88 18 99 6 0 20

-------
  uint16_t pressure;
  uint32_t minutes: 24;
  int16_t temperature : 14;
  uint16_t humidity : 10;
...
d3
  size : 12
  minutes     = ABCDEF
  pressure    = 7777
  temperature = 1888
  humidity    = 299
       memory = 77 77 0 0 EF CD AB 0 88 18 99 22

-------
  uint16_t pressure;
  int16_t temperature : 14;
  uint16_t humidity : 10;
  uint32_t minutes: 24;
...
d3
  size : 12
  minutes     = ABCDEF
  pressure    = 7777
  temperature = 1888
  humidity    = 299
       memory = 77 77 88 18 99 2 0 0 EF CD AB 20

--------
  uint16_t pressure;
  int16_t temperature : 14;
  uint16_t humidity : 10;
  uint32_t minutes;
...
d3
  size : 12
  minutes     = ABCDEF
  pressure    = 7777
  temperature = 1888
  humidity    = 299
       memory = 77 77 88 18 99 2 0 0 EF CD AB 0

-------
  uint32_t minutes : 24;
  int16_t temperature : 14;
  uint16_t humidity : 10;
  uint16_t pressure;
...
d3
  size : 12
  minutes     = ABCDEF
  pressure    = 7777
  temperature = 1888
  humidity    = 299
       memory = EF CD AB 0 88 18 99 2 77 77 0 20


*/