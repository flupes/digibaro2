#include <Arduino.h>

#if defined(ARDUINO_SAMD_ZERO) && defined(SERIAL_PORT_USBVIRTUAL)
#define Serial SERIAL_PORT_USBVIRTUAL
#endif

// compile with:
// pio ci .\test --board=zeroUSB -l src -l ..\BaroUtils -l ..\BaroSample
// -O "targets=upload"

// monitor with:
// pio device monitor --port COM5 --baud 115200

#include "baro_sample.h"
#include "rotating_samples.h"

const size_t kMaxSamples = 1024;

// #define SKIP_WRITE
#define SKIP_ERASE

SPIFlash flash(kMiniUltraProOnBoardChipSelectPin);
RotatingSamples samples_ring(flash);

void setup() {
  Serial.begin(115200);
  while (!Serial)
    ;

  Serial.println("Starting test_rotating_samples...");

  if (!flash.begin()) {
    Serial.println("Flash memory initialization error!");
    while (1)
      ;
  }

#if !defined(SKIP_ERASE)
  uint32_t end_ring_buffer = kPermanentSamplesSectorStart * KB(4) - 1; 
   Serial.print(
      "Erasing both robust indexes and ring buffer up to addr = ");
  Serial.print(end_ring_buffer);
  flash.eraseSection(0, end_ring_buffer);
  Serial.println(" | Done.");
#endif

  uint32_t pressure = 80000;
  int32_t temperature = -6000;
  uint32_t humidity = 0;
  uint32_t timestamp = k2019epoch;

  BaroSample samples_ref[kMaxSamples];

  Serial.println("Create RotatingSamples...");
  samples_ring.begin();

  Serial.println("Write samples to flash...");
  uint16_t error_count = 0;
  for (uint32_t i = 0; i < kMaxSamples; i++) {
    samples_ref[i].Set(timestamp, pressure, temperature, humidity);
    timestamp += 3600;
    pressure += 100;
    temperature += 20;
    humidity += 5;
#if !defined(SKIP_WRITE)
    // Serial.print("Adding sample #");
    // Serial.println(i);
    samples_ring.AddSample(samples_ref[i]);
#endif
  }

  Serial.println("Read samples from flash");
  uint32_t index = samples_ring.GetFirstIndexOfSerie(kMaxSamples);
  Serial.print("First index of serie with length = ");
  Serial.print(kMaxSamples);
  Serial.print(" --> ");
  Serial.println(index);
  for (uint32_t i = 0; i < kMaxSamples; i++) {
    BaroSample sample = samples_ring.GetSampleAtIndex(index);
    // Serial.println("Ref sample:");
    // samples_ref[i].Print();
    // Serial.println("Read sample:");
    // sample.Print();
    if (sample != samples_ref[i]) {
      Serial.print("Ref sample # ");
      Serial.print(i);
      Serial.print(" differs from sample stored at index ");
      Serial.println(index);
      error_count++;
    }
    index++;
    if (index == samples_ring.GetTotalNumberOfSamples()) {
      index = 0;
    }
  }
  Serial.print("last counter = ");
  Serial.println(samples_ring.GetIndexesCounter());
  Serial.println();
  Serial.print("Error Count = ");
  Serial.println(error_count);
}

void loop() {}
