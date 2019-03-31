#include <Arduino.h>

#if defined(ARDUINO_SAMD_ZERO) && defined(SERIAL_PORT_USBVIRTUAL)
#define Serial SERIAL_PORT_USBVIRTUAL
#endif

#include "baro_sample.h"

// compile with:
// pio ci .\test --board=zeroUSB -l src -O "targets=upload"

// monitor with:
// pio device monitor --port COM5 --baud 115200

void setup() {
  Serial.begin(115200);
  while (!Serial)
    ;

  Serial.println("Starting...");

  Serial.print("BaroSample size = ");
  Serial.println(sizeof(BaroSample));

  // two buffers are not necessary for this test, but demonstrate the inteded usage
  // with flash memory read / write.
  PackedBaroSample buffer_write, buffer_read;

  BaroSample inp(1558569600, 60000, 8191, 5000);
  inp.Print();
  inp.PackSample(buffer_write);
  inp.Inspect(buffer_write);
  memcpy(buffer_read, buffer_write, kBaroSampleSize);
  Serial.println();

  BaroSample outp(buffer_read);
  outp.Print();
  outp.Inspect(buffer_read);

  Serial.println();
  Serial.println();

  BaroSample inn(1558569600, 125000, -8192, 10000);
  inn.Print();
  inn.PackSample(buffer_write);
  inn.Inspect(buffer_write);
  memcpy(buffer_read, buffer_write, kBaroSampleSize);
  Serial.println();
 
  BaroSample outn(buffer_read);
  outn.Print();
  outn.Inspect(buffer_read);
  if ( (int32_t)(-8192) != outn.GetTemperature() ) {
    Serial.println("not happy already!");
  }

  Serial.println();
  Serial.println("Some more tests now...");
  Serial.println();

  uint32_t pressure = 60000;
  int32_t temperature = -8000;
  uint32_t humidity = 0;
  uint32_t timestamp = k2019epoch;

  uint16_t error_count = 0;
  for (int i = 0; i < 1000; i++) {
    bool error = false;
    BaroSample in(timestamp, pressure, temperature, humidity);
    in.PackSample(buffer_write);
    memcpy(buffer_read, buffer_write, kBaroSampleSize);
    BaroSample out(buffer_read);
    if (timestamp != out.GetTimestamp()) {
      Serial.println("Timestamp conversions failed!");
      error = true;
    }
    if (pressure != out.GetPressure()) {
      Serial.println("Pressure conversions failed!");
      error = true;
    }
    if (temperature != out.GetTemperature()) {
      Serial.println("Temperature conversions failed!");
      error = true;
    }
    if (in.GetHumidity() != out.GetHumidity()) {
      Serial.println("Humidity conversions failed!");
      error = true;
    }
    if (error) {
      error_count++;
      Serial.print("Error for test # ");
      Serial.println(i);
      in.Print();
      in.Inspect(buffer_write);
      out.Print();
      out.Inspect(buffer_read);
      Serial.println();
      Serial.println();
    }
    timestamp += 3600;
    pressure += 65;
    temperature += 16;
    humidity += 1;
  }
  Serial.println();
  Serial.print("Error Count = ");
  Serial.println(error_count);
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