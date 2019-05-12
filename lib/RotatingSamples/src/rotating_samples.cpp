#include "rotating_samples.h"

#include "baro_sample.h"

#ifdef DIGI_DEBUG
#include "print_utils.h"
#else
#define PRINT(x)
#define PRINTLN(x)
#endif

RotatingSamples::RotatingSamples(SPIFlash& flash, uint32_t indexes_start,
                                 uint32_t robust_indices_sector_length)
    : flash_(flash), indexes_(indexes_start, robust_indices_sector_length) {
  samples_addr_start_ =
      KB(4) * (indexes_start + indexes_.GetAllocatedSectors());
  ring_sectors_length_ = (kRingSampleByteLength / kRobustIndexByteLength) *
                         (indexes_.GetAllocatedSectors() / 2);
  max_samples_ = ring_sectors_length_ * KB(4) / kRingSampleByteLength;
  usable_samples_ = (ring_sectors_length_ - 1) * KB(4) / kRingSampleByteLength;
}

uint32_t RotatingSamples::begin() {
  PRINTLN("RotatingSamples::begin()");
  PRINT("rotating sample addr start = ");
  PRINTLN(samples_addr_start_);
  PRINT("rotating samples sector length = ");
  PRINTLN(ring_sectors_length_);
  PRINT("max number of rotating samples = ");
  PRINTLN(max_samples_);
  PRINT("usable number of rotating samples = ");
  PRINTLN(usable_samples_);
  return indexes_.begin(&flash_);
}

uint32_t RotatingSamples::GetSectorStart() { return indexes_.GetFirstSector(); }

uint32_t RotatingSamples::GetSectorsLength() {
  return indexes_.GetAllocatedSectors() + ring_sectors_length_;
}

uint32_t RotatingSamples::GetTotalNumberOfSamples() { return max_samples_; }

uint32_t RotatingSamples::GetUsableNumberOfSamples() { return usable_samples_; }

uint32_t RotatingSamples::GetLastSampleIndex() {
  return indexes_.GetCurrentIndex();
}

uint32_t RotatingSamples::GetReverseIndexIterator() {
  rev_index_iterator_ = GetLastSampleIndex();
  return rev_index_iterator_;
}

uint32_t RotatingSamples::GetPreviousIndex() {
  uint32_t current_counter = indexes_.GetCounterAt(rev_index_iterator_);
  // Serial.print("current_counter=");
  // Serial.print(current_counter);
  // Serial.print(" / rev_index_iterator_=");
  // Serial.println(rev_index_iterator_);
  if (rev_index_iterator_ > 0) {
    rev_index_iterator_--;
  } else {
    rev_index_iterator_ = max_samples_ - 1;
  }
  uint32_t prev_counter = indexes_.GetCounterAt(rev_index_iterator_);
  // Serial.print("new rev_index_iterator_=");
  // Serial.print(rev_index_iterator_);
  // Serial.print(" / prev_counter=");
  // Serial.println(prev_counter);
  if (prev_counter == kInvalidInt24 || prev_counter == 0xFFFFFFFF) {
    rev_index_iterator_ = kInvalidInt24;
  } else {
    if (prev_counter > current_counter) {
      rev_index_iterator_ = kInvalidInt24;
    }
  }
  return rev_index_iterator_;
}

uint32_t RotatingSamples::GetIndexIterator(uint32_t length) {
  iterator_end_ = true;

  last_index_iterator_ = GetLastSampleIndex();
  if ( length == 0) {
    length = usable_samples_;
  }
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
  // Serial.println(current_index_iterator_);
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
  uint32_t addr = samples_addr_start_ + index * kRingSampleByteLength;
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
    // PRINT("RotatingSamples::AddSample entering new sector : ");
    if (code != 0xFFFFFFFF) {
      // PRINTLN("Need to erase first.");
      if (!flash_.eraseSector(addr)) {
        // PRINT("Error erasing sector starting at addr = ");
        // PRINTLN(addr);
      }
    } else {
      // PRINTLN("Sector seems already initialized.");
    }
  }

  flash_.writeCharArray(addr, data, kRingSampleByteLength);
  return index;
}

BaroSample RotatingSamples::GetSampleAtIndex(uint32_t index) {
  if (index < max_samples_) {
    uint32_t addr = samples_addr_start_ + index * kRingSampleByteLength;
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
