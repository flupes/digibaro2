#include <Arduino.h>

#if defined(ARDUINO_SAMD_ZERO) && defined(SERIAL_PORT_USBVIRTUAL)
#define Serial SERIAL_PORT_USBVIRTUAL
#endif

/*
compile with:
  pio ci .\test\TestIterators --board=zeroUSB -l src -l ..\BaroUtils
    -l ..\BaroSample -l ..\RobustFlashIndexes -l ..\FastCRC -l ..\SPIMemory
    -O "targets=upload"

monitor with:
  pio device monitor --port COM5 --baud 115200
*/

#include "baro_sample.h"
#include "rotating_samples.h"

const size_t kMaxSamples = 6;

// #define SKIP_WRITE
// #define SKIP_ERASE

SPIFlash flash(kMiniUltraProOnBoardChipSelectPin);
RotatingSamples buffer(flash, 256);

BaroSample samples_ref[kMaxSamples];

size_t test_forward(size_t length) {
  Serial.println("Read samples from flash with forward iterator");
  size_t errors = 0;
  uint32_t index = buffer.GetIndexIterator(length);
  Serial.print("First index of serie with length = ");
  Serial.print(length);
  Serial.print(" --> ");
  Serial.println(index);
  size_t i = kMaxSamples - length;
  while (index != kInvalidInt24) {
    BaroSample sample = buffer.GetSampleAtIndex(index);
    if (sample != samples_ref[i]) {
      Serial.print("Ref sample # ");
      Serial.print(i);
      Serial.print(" differs from sample stored at index ");
      Serial.println(index);
      errors++;
    }
    index = buffer.GetNextIndex();
    i++;
  }
  return errors;
}

size_t test_reverse() {
  Serial.println("Read samples from flash with reverse iterator");
  size_t errors = 0;
  uint32_t index = buffer.GetReverseIndexIterator();
  uint32_t last = index;
  size_t i = kMaxSamples - 1;
  while (index != kInvalidInt24) {
    if (i != kInvalidInt24) {
      Serial.print("reverse index = ");
      Serial.println(index);
      BaroSample sample = buffer.GetSampleAtIndex(index);
      if (sample != samples_ref[i]) {
        Serial.print("Ref sample # ");
        Serial.print(i);
        Serial.print(" differs from sample stored at index ");
        Serial.println(index);
        errors++;
      }
      if (i > 0 && i < kMaxSamples) {
        i--;
      } else {
        i = kInvalidInt24;
      }
    }
    last = index;
    index = buffer.GetPreviousIndex();
  }
  Serial.print("reverse iterator ended with index = ");
  Serial.print(last);
  return errors;
}

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
  uint32_t start_ring_buffer = buffer.GetSectorStart();
  uint32_t length_ring_buffer = buffer.GetSectorsLength();
  Serial.print("Erasing both robust indexes and ring buffer from addr = ");
  Serial.print(start_ring_buffer);
  Serial.print(" with data size = ");
  Serial.print(length_ring_buffer);
  flash.eraseSection(KB(4) * start_ring_buffer, KB(4) * length_ring_buffer);
  Serial.println(" | Done.");
#endif

  uint32_t pressure = 100000;
  int32_t temperature = -6000;
  uint32_t humidity = 0;
  uint32_t timestamp = k2019epoch;

  Serial.println("Create RotatingSamples...");
  uint32_t current = buffer.begin();

  Serial.print("Current index from flash = ");
  Serial.println(current);

  Serial.println("Write samples to flash...");
  uint16_t error_count = 0;
  for (uint32_t i = 0; i < kMaxSamples; i++) {
    samples_ref[i].Set(timestamp, pressure, temperature, humidity);
    timestamp += 3600;
    pressure += 20;
    temperature += 12;
    humidity += 10;
#if !defined(SKIP_WRITE)
    buffer.AddSample(samples_ref[i]);
#endif
  }

  Serial.print("Last sample written to flash is at index = ");
  Serial.println(buffer.GetLastSampleIndex());
  Serial.println();

  uint32_t i = buffer.GetIndexIterator();
  if (i != 0) {
    Serial.println("index returned should have zero!");
    error_count++;
  }
  error_count += test_forward(kMaxSamples);
  error_count += test_forward(3);
  error_count += test_reverse();

#if !defined(SKIP_WRITE)
  for (uint32_t i = 0; i < buffer.GetTotalNumberOfSamples(); i++) {
    buffer.AddSample(samples_ref[0]);
  }
  for (uint32_t i = 0; i < kMaxSamples; i++) {
    buffer.AddSample(samples_ref[i]);
  }
  error_count += test_reverse();
#endif

  Serial.println();
  Serial.print("Error Count = ");
  Serial.println(error_count);
}

void loop() {}
