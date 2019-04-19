#include "rotating_samples.h"

#ifdef RS_DEBUG
#define Serial SERIAL_PORT_USBVIRTUAL
#endif

RotatingSamples::RotatingSamples(SPIFlash& flash)
    : flash_(flash),
      indexes_(kRobustIndexesSectorStart, kRobustIndexesSectorLength) {
  max_samples_ = kRingSamplesSectorLength * KB(4) / kRingSampleByteLength;
  usable_samples_ =
      (kRingSamplesSectorLength - 1) * KB(4) / kRingSampleByteLength;
}

void RotatingSamples::begin() {
  indexes_.begin(&flash_);
#ifdef RS_DEBUG
  Serial.print("Usable samples = ");
  Serial.print(usable_samples_);
  Serial.print(" / Total samples = ");
  Serial.println(max_samples_);
#endif
}

uint32_t RotatingSamples::GetTotalNumberOfSamples() { return max_samples_; }

uint32_t RotatingSamples::GetUsableNumberOfSamples() { return usable_samples_; }

uint32_t RotatingSamples::GetLastSampleIndex() {
  return indexes_.GetCurrentIndex();
}

uint32_t RotatingSamples::GetFirstIndexOfSerie(uint32_t length) {
  uint32_t last = indexes_.GetCurrentIndex();
  // Serial.print("GetFirstIndexOfSerie(");
  // Serial.print(length);
  // Serial.print(") : last=");
  // Serial.println(last);
  uint32_t first = kInvalidInt24;
  if (length > usable_samples_) return first;
  if (last < length - 1) {
    // Serial.print("last<length-1");
    first = max_samples_ + last + 1 - length;
    // make sure we fall on a initialized value (before a full buffer)
    uint32_t counter = indexes_.GetCounterAt(first);
    if (counter == kInvalidInt24) first = 0;
  } else {
    // Serial.print("last>=length-1");
    first = last + 1 - length;
  }
  // Serial.print(" --> first=");
  // Serial.println(first);
  return first;
}

uint32_t RotatingSamples::AddSample(BaroSample& sample) {
  indexes_.Increment();
  uint32_t index = indexes_.GetCurrentIndex();
  uint32_t addr = kRotatingSamplesAddrStart + index * kRingSampleByteLength;
  SerializedBaroSample data;
  sample.SerializeSample(data);
#ifdef RS_DEBUG
  Serial.print("Write Sample to addr = ");
  Serial.println(addr);
  Serial.print("Data to flash =  ");
  for (size_t u = 0; u < kRingSampleByteLength; u++) {
    Serial.print(data[u], HEX);
  }
  Serial.println();
#endif
  if (index % (kSectorLength / kRingSampleByteLength) == 0) {
    // entering a new sector!
    // Serial.print("RotatingSamples entering new sector for index = ");
    // Serial.println(index);
    uint32_t code;
    code = flash_.readLong(addr);
    if (code != 0xFFFFFFFF) {
#ifdef RS_DEBUG
      Serial.print("RotatingSample erasing sector starting at: ");
      Serial.println(addr);
#endif
      flash_.eraseSector(addr);
    }
  }

  flash_.writeCharArray(addr, data, kRingSampleByteLength);
  return index;
}

BaroSample RotatingSamples::GetSampleAtIndex(uint32_t index) {
  if (index < max_samples_) {
    uint32_t addr = kRotatingSamplesAddrStart + index * kRingSampleByteLength;
    SerializedBaroSample data;
    flash_.readCharArray(addr, data, kRingSampleByteLength);
    // Serial.print("Read Sample from addr = ");
    // Serial.println(addr);
    // Serial.print("Data from flash = ");
    // for (size_t u = 0; u < kRingSampleByteLength; u++) {
    //   Serial.print(data[u], HEX);
    // }
    // Serial.println();
    BaroSample sample(data, false);
    return sample;
  }
  return BaroSample();
}

uint32_t RotatingSamples::GetIndexesCounter() {
  return indexes_.GetCurrentCounter();
}
