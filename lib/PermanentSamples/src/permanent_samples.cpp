#include "permanent_samples.h"

#include "print_utils.h"

PermanentSamples::PermanentSamples(SPIFlash& flash) : flash_(flash) {
  max_samples_ =
      kPermanentSamplesSectorLength * KB(4) / kPermanentSampleBytesLength;
  min_pressure_ = BaroSample(k2019epoch, kMaxRecordablePressure, 0, 0);
  max_pressure_ = BaroSample(k2019epoch, 0, 0, 0);
}

uint32_t PermanentSamples::begin() {
  uint32_t start = millis();
  uint32_t addr = kPermanentSamplesAddrStart;
  bool reached_unitialized = false;
  for (number_of_samples_ = 0; number_of_samples_ < max_samples_;
       number_of_samples_++) {
    uint32_t word = flash_.readLong(addr);
    if (word == 0xFFFFFFFF) {
      reached_unitialized = true;
      PRINT("Uninitialized memory found at addr = ");
      PRINT(addr);
      PRINT(" --> index = ");
      PRINTLN(number_of_samples_);
      break;
    }
    PackedBaroSample data;
    flash_.readCharArray(addr, data, kBaroPackedSampleSize);
    current_sample_ = BaroSample(data);
    if (current_sample_.GetPressure() > max_pressure_.GetPressure()) {
      max_pressure_ = current_sample_;
    }
    if (current_sample_.GetPressure() < min_pressure_.GetPressure()) {
      // DEBUG("new min pressure", number_of_samples_);
      // current_sample_.PrettyPrint();
      min_pressure_ = current_sample_;
    }
    addr += kPermanentSampleBytesLength;
  }
  if (!reached_unitialized) {
    PRINTLN("Storage is already full!");
    number_of_samples_ = max_samples_;
  }
  if (number_of_samples_ == 0) {
    PRINTLN("No samples on storage yet.");
  }
  DEBUG("PermanentSamples::begin (ms)", millis() - start);
  // min_pressure_.PrettyPrint();
  // max_pressure_.PrettyPrint();
  return number_of_samples_;
}

uint32_t PermanentSamples::AddSample(BaroSample& sample) {
  PackedBaroSample data;
  sample.PackSample(data);
  uint32_t addr = number_of_samples_ * kPermanentSampleBytesLength +
                  kPermanentSamplesAddrStart;
  if (number_of_samples_ < max_samples_) {
    bool ret = flash_.writeCharArray(addr, data, kBaroPackedSampleSize);
    if (ret) {
      number_of_samples_++;
      // PRINT("Wrote successfuly sample # ");
      // PRINT(number_of_samples_);
      // PRINT(" at address ");
      // PRINTLN(addr);
      if (sample.GetPressure() > max_pressure_.GetPressure()) {
        max_pressure_ = sample;
      }
      if (sample.GetPressure() < min_pressure_.GetPressure()) {
        min_pressure_ = sample;
      }
    } else {
      PRINT("Cannot write sample # ");
      PRINT(number_of_samples_);
      PRINT(" at address ");
      PRINTLN(addr);
    }
  } else {
    PRINTLN("Storage already full: cannot add a new sample!");
  }
  return number_of_samples_;
}

BaroSample PermanentSamples::GetSampleAtAddr(uint32_t addr) {
  if (addr >
      kPermanentSamplesAddrStart + max_samples_ * kPermanentSampleBytesLength) {
    PRINT("GetSampleAddr with argument out of range: ");
    PRINTLN(addr);
    return BaroSample();
  }
  uint32_t word = flash_.readLong(addr);
  if (word == 0xFFFFFFFF) {
    PRINT("Unvalid sample read at addr = ");
    PRINT(addr);
    PRINTLN(" ! --> Return dummy sample :-(");
    return BaroSample();
  }
  PackedBaroSample data;
  flash_.readCharArray(addr, data, kBaroPackedSampleSize);
  return BaroSample(data);
}

BaroSample PermanentSamples::GetSampleWithIndex(uint32_t index) {
  if (index < max_samples_) {
    uint32_t addr =
        kPermanentSamplesAddrStart + index * kPermanentSampleBytesLength;
    return GetSampleAtAddr(addr);
  }
  // Index out of range --> return invalid sample
  PRINT("GetSampleWithIndex with argument out of range: ");
  PRINTLN(index);
  return BaroSample(k2019epoch, 0, 0, 0);
}

BaroSample PermanentSamples::GetLastSample() {
  return GetSampleAtAddr(GetLastSampleAddr());
}

uint32_t PermanentSamples::GetCurrentNumberOfSamples() {
  return number_of_samples_;
}

uint32_t PermanentSamples::GetMaxNumberOfSamples() { return max_samples_; }

uint32_t PermanentSamples::GetFirstSampleAddr() {
  return kPermanentSamplesAddrStart;
}

uint32_t PermanentSamples::GetLastSampleAddr() {
  if (number_of_samples_ > 0) {
    return kPermanentSamplesAddrStart +
           kPermanentSampleBytesLength * (number_of_samples_ - 1);
  }
  return 0xFFFFFFFF;
}
