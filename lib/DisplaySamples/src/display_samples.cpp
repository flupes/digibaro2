#include "display_samples.h"

#include "print_utils.h"
#include "rotating_samples.h"

DisplaySamples::DisplaySamples(uint32_t p) : period_seconds_(p) {}

uint32_t DisplaySamples::Begin(RotatingSamples *src, SampleDataType dt) {
  length_ = 0;
  index_ = 0;
  uint32_t flash_index = src->GetReverseIndexIterator();
  if (flash_index == kInvalidInt24) {
    for (size_t i = 0; i < kGraphPxLength; i++) {
      // zero buffer since it is empty
      buffer_[i] = 0;
    }
    return 0;
  }

  BaroSample sample = src->GetSampleAtIndex(flash_index);
  uint32_t last_ts = sample.GetTimestamp();
  bool filled = false;
  int16_t buffer_index = kGraphPxLength - 1;
  while (!filled) {
    int16_t data = 0;
    switch (dt) {
      case TEMPERATURE:
        data = (int16_t)(sample.GetTemperature());
        break;
      case HUMIDITY:
        data = (int16_t)(sample.GetHumidity());
        break;
      default:
        // Warning: pressure numbers are too high to fit on a int16:
        // the buffer will thus contain deciPa (rather than Pa)!
        data = (int16_t)(sample.GetPressure() / 10);
    }
    buffer_[buffer_index] = data;
    length_++;
    uint32_t timespan = 0;
    while (timespan < period_seconds_) {
      // search for the next sample satifying the required period
      // (will skip samples with timespan smaller than period)
      flash_index = src->GetPreviousIndex();
      if (flash_index == kInvalidInt24) {
        // we run out of flash samples before filling the display buffer!
        index_ = buffer_index;
        for (size_t i = buffer_index - 1; i >= 0; i--) {
          buffer_[i] = 0;
        }
        return length_;
      }
      sample = src->GetSampleAtIndex(flash_index);
      timespan = last_ts - sample.GetTimestamp();
    }
    last_ts = sample.GetTimestamp();
    uint32_t increment = timespan / period_seconds_;
    // fill buffer with 0 in case there was missing samples in flash
    // (case where timespan between two flash samples is larger than period)
    for (uint16_t j = 0; j < increment; j++) {
      buffer_index--;
      if (buffer_index >= 0) {
        buffer_[buffer_index] = 0;
        if (j>0) length_++;
      } else {
        filled = true;
      }
    }
  }
  index_ = buffer_index;
  return length_;
}

uint32_t DisplaySamples::AppendData(int16_t data) {
  uint32_t new_index = index_ + length_;
  if (new_index >= kGraphPxLength) new_index -= kGraphPxLength;
  buffer_[new_index] = data;
  index_++;
  if (index_ >= kGraphPxLength) index_ -= kGraphPxLength;
  return new_index;
}
