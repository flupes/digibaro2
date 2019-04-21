#include "rotating_samples.h"

RotatingSamples::RotatingSamples(SPIFlash& flash)
    : flash_(flash),
      indexes_(kRobustIndexesSectorStart, kRobustIndexesSectorLength) {
  max_samples_ = kRingSamplesSectorLength * KB(4) / kRingSampleByteLength;
  usable_samples_ =
      (kRingSamplesSectorLength - 1) * KB(4) / kRingSampleByteLength;
}

uint32_t RotatingSamples::begin() {
  return indexes_.begin(&flash_);
}

uint32_t RotatingSamples::GetTotalNumberOfSamples() { return max_samples_; }

uint32_t RotatingSamples::GetUsableNumberOfSamples() { return usable_samples_; }

uint32_t RotatingSamples::GetLastSampleIndex() {
  return indexes_.GetCurrentIndex();
}

uint32_t RotatingSamples::GetIndexIterator(uint32_t length) {
  iterator_end_ = true;
  
  last_index_iterator_ = GetLastSampleIndex();
  // Serial.print("GetFirstIndexOfSerie(");
  // Serial.print(length);
  // Serial.print(") : last=");
  // Serial.println(last_index_iterator_);

  current_index_iterator_ = kInvalidInt24;
  if (length > usable_samples_) return current_index_iterator_;
  if (last_index_iterator_ < length - 1) {
    // Serial.print("last<length-1");
    current_index_iterator_ = max_samples_ + last_index_iterator_ + 1 - length;
    // make sure we fall on a initialized value (before a full buffer)
    uint32_t counter = indexes_.GetCounterAt(current_index_iterator_);
    if (counter == kInvalidInt24) current_index_iterator_ = 0;
  } else {
    // Serial.print("last>=length-1");
    current_index_iterator_ = last_index_iterator_ + 1 - length;
  }
  // Serial.print(" --> current_index_iterator_=");
  Serial.println(current_index_iterator_);
  iterator_end_ = false;
  return current_index_iterator_;
}

uint32_t RotatingSamples::GetNextIndex() {
  if (iterator_end_) return kInvalidInt24;
  current_index_iterator_++;
  if (current_index_iterator_ >= max_samples_) {
    current_index_iterator_ = 0;
  }
  if (current_index_iterator_ == last_index_iterator_) {
    iterator_end_ = true;
  }
  return current_index_iterator_;
}

uint32_t RotatingSamples::AddSample(BaroSample& sample) {
  indexes_.Increment();
  uint32_t index = indexes_.GetCurrentIndex();
  uint32_t addr = kRotatingSamplesAddrStart + index * kRingSampleByteLength;
  SerializedBaroSample data;
  sample.SerializeSample(data);

  // Serial.print("Write Sample to addr = ");
  // Serial.println(addr);
  // Serial.print("Data to flash =  ");
  // for (size_t u = 0; u < kRingSampleByteLength; u++) {
  //   Serial.print(data[u], HEX);
  // }
  // Serial.println();

  if (index % (kSectorLength / kRingSampleByteLength) == 0) {
    // entering a new sector!
    uint32_t code;
    code = flash_.readLong(addr);
    if (code != 0xFFFFFFFF) {
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
