#include <Arduino.h>

#if defined(ARDUINO_SAMD_ZERO) && defined(SERIAL_PORT_USBVIRTUAL)
#define Serial SERIAL_PORT_USBVIRTUAL
#endif

// compile with:
// pio ci .\test --board=zeroUSB -l src -l ..\BaroUtils -l ..\BaroSample
//  -l ..\RobustFlashIndexes -l ..\FastCRC -l ../SPIMemory -O "targets=upload"

// monitor with:
// pio device monitor --port COM5 --baud 115200

#include "baro_sample.h"
#include "rotating_samples.h"

const size_t kMaxSamples = 1000;

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
  Serial.print("Erasing both robust indexes and ring buffer up to addr = ");
  Serial.print(end_ring_buffer);
  flash.eraseSection(0, end_ring_buffer);
  Serial.println(" | Done.");
#endif

  // make sure we are not reading known values
  randomSeed(analogRead(0));
  uint32_t pressure = random(90000, 100000);
  int32_t temperature = -6000;
  uint32_t humidity = 0;
  uint32_t timestamp = k2019epoch;

  BaroSample samples_ref[kMaxSamples];

  Serial.println("Create RotatingSamples...");
  uint32_t current = samples_ring.begin();

  Serial.print("Current index from flash = ");
  Serial.println(current);

  Serial.println("Write samples to flash...");
  uint16_t error_count = 0;
  for (uint32_t i = 0; i < kMaxSamples; i++) {
    // Serial.print("pressure = ");
    // Serial.println(pressure);
    samples_ref[i].Set(timestamp, pressure, temperature, humidity);
    timestamp += 3600;
    pressure += 20;
    temperature += 12;
    humidity += 10;
#if !defined(SKIP_WRITE)
    // Serial.print("Adding sample #");
    // Serial.println(i);
    samples_ring.AddSample(samples_ref[i]);
#endif
  }

  Serial.print("Last sample written to flash is at index = ");
  Serial.println(samples_ring.GetLastSampleIndex());
  Serial.println();

  Serial.println("Read samples from flash");
  uint32_t index = samples_ring.GetIndexIterator(kMaxSamples);
  Serial.print("First index of serie with length = ");
  Serial.print(kMaxSamples);
  Serial.print(" --> ");
  Serial.println(index);
  for (uint32_t i = 0; i < kMaxSamples; i++) {
    BaroSample sample = samples_ring.GetSampleAtIndex(index);
    // Serial.println("Ref sample:");
    // samples_ref[i].PrettyPrint();
    // Serial.println("Read sample:");
    // sample.PrettyPrint();
    if (sample != samples_ref[i]) {
      Serial.print("Ref sample # ");
      Serial.print(i);
      Serial.print(" differs from sample stored at index ");
      Serial.println(index);
      error_count++;
    }
    index = samples_ring.GetNextIndex();
    if (i < kMaxSamples - 1 && index == kInvalidInt24) {
      Serial.println("Interator Invalid returned!");
      break;
    }
  }
  Serial.print("Last index = ");
  Serial.println(samples_ring.GetLastSampleIndex());
  Serial.print("last counter = ");
  Serial.println(samples_ring.GetIndexesCounter());
  Serial.println();
  Serial.print("Error Count = ");
  Serial.println(error_count);
}

void loop() {}
