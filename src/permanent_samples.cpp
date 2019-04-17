#include "permanent_samples.h"

#include "print_utils.h"

PermanentSamples::PermanentSamples(SPIFlash& flash)
    : flash_(flash), empty_(false), full_(false) {}

uint32_t PermanentSamples::begin() {
  current_sample_addr_ = LocateLastSample();
  if (current_sample_addr_ == 0xFFFFFFFF) {
    PRINTLN("PermanentSamples storage space is full!");
    full_ = true;
  }
  if (current_sample_addr_ == kPermanentSamplesAddrStart) {
    PRINTLN("No samples yet in flash...");
    empty_ = true;
  }
  else {
    current_sample_addr_ -= kPermanentSampleBytesLength;
  }
  return current_sample_addr_;
}

uint32_t PermanentSamples::AddSample(BaroSample& sample) {
  PackedBaroSample data;
  sample.PackSample(data);
  if (!empty_ && !full_) {
    current_sample_addr_ += kPermanentSampleBytesLength;
    if (current_sample_addr_ > kPermanentSamplesAddrStart +
                                   kPermanentSamplesSectorLength * KB(4) - 1) {
      full_ = true;
    }
  }
  if (!full_) {
    flash_.writeCharArray(current_sample_addr_, data, kBaroSampleSize);
    empty_ = false;
  }
  return current_sample_addr_;
}

BaroSample PermanentSamples::GetSampleAtAddr(uint32_t addr) {
  uint32_t word = flash_.readLong(addr);
  if (word == 0xFFFFFFFF) {
    PRINT("Unvalid sample read at addr = ");
    PRINT(addr);
    PRINTLN(" ! --> Return dummy sample :-(");
    return BaroSample(k2019epoch, 0, 0, 0);
  }
  PackedBaroSample data;
  flash_.readCharArray(addr, data, kBaroSampleSize);
  return BaroSample(data);
}

BaroSample PermanentSamples::GetSampleWithIndex(uint32_t index) {
  uint32_t addr =
      kPermanentSamplesAddrStart + index * kPermanentSampleBytesLength;
  return GetSampleAtAddr(addr);
}

uint32_t PermanentSamples::GetNumberOfSamples() {
  if (empty_) return 0;
  return (current_sample_addr_ - kPermanentSamplesAddrStart) /
             kPermanentSampleBytesLength +
         1;
}

uint32_t PermanentSamples::GetLastSampleAddr() { return current_sample_addr_; }

uint32_t PermanentSamples::LocateLastSample() {
  uint32_t addr = kPermanentSamplesAddrStart;
  for (uint16_t i = 0;
       i < kPermanentSamplesSectorLength * KB(4) / kPermanentSampleBytesLength;
       i++) {
    uint32_t word = flash_.readLong(addr);
    if (word == 0xFFFFFFFF) {
      PRINT("Uninitialized memory found at addr = ");
      PRINT(addr);
      PRINT(" --> index = ");
      PRINTLN(i);
      return addr;
    }
    addr += kPermanentSampleBytesLength;
  }
  return 0xFFFFFFFF;
}