#include "display_samples.h"

#include "baro_sample.h"
#include "print_utils.h"
#include "rotating_samples.h"

DisplaySamples::DisplaySamples(uint32_t p, uint32_t length) : period_(p) {
  if (length < kGraphPxLength) {
    size_ = length;
  } else {
    size_ = length;
  }
}

uint32_t DisplaySamples::Fill(RotatingSamples *src, uint32_t now,
                              SampleDataType dt) {
  type_ = dt;
  // Set the all buffer to zero
  for (size_t i = 0; i < size_; i++) {
    buffer_[i] = INT16_MIN;
  }
  min_ = INT16_MAX - 1;
  max_ = INT16_MIN + 1;
  uint32_t count = 0;
  last_ = now;
  first_ = now - size_ * period_;

  uint32_t flash_index = src->GetReverseIndexIterator();
  if (flash_index == kInvalidInt24) return count;

  BaroSample sample = src->GetSampleAtIndex(flash_index);
  uint32_t last_ts = sample.GetTimestamp();
  uint32_t sample_ts = last_ts;
  int16_t buffer_index = size_ - 1;

  // DEBUG("DisplaySamples::Fill");
  // DEBUG("buffer size", size_);
  // DEBUG("flash_index", flash_index);

  if (last_ts > now) {
    // we need to skip the sample in flash that are considered in the future!
    // DEBUG("Flash timestamp is in the future!");
    while (sample_ts > now) {
      flash_index = src->GetPreviousIndex();
      if (flash_index == kInvalidInt24) {
        DEBUG(
            "Reached end of buffer before finding a sample more recent than "
            "the given date!");
        return count;
      }
      sample = src->GetSampleAtIndex(flash_index);
      sample_ts = sample.GetTimestamp();
    }
  } else {
    // reference time is larger or equal to last sample in flash (nominal case):
    // we may need to move buffer to the left to accomodate the time gap.
    int32_t skip = (now - sample_ts) / period_;
    if (skip > buffer_index) {
      DEBUG("Entire display buffer is more recent than samples in flash!");
      return count;
    }
    if (skip > 0) {
      buffer_index -= skip;
      DEBUG("Display buffer has to be shifted to the left by skip", skip);
      DEBUG("Starting to process indexes at buffer_index", buffer_index);
    }
  }

  while (buffer_index >= 0) {
    int16_t data = 0;
    switch (type_) {
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
    // PRINT("buffer_index=");
    // PRINT(buffer_index);
    // PRINT(" <-- data=");
    // PRINT(data);
    // PRINT(" | ts=");
    // PRINTLN(sample_ts);
    buffer_[buffer_index] = data;
    count++;
    if (data > max_) max_ = data;
    if (data < min_) min_ = data;

    uint32_t last_sample_ts = sample_ts;
    uint32_t timespan = 0;
    while (timespan < period_) {
      // search for the next sample satifying the required period
      // (will skip samples with timespan smaller than period)
      flash_index = src->GetPreviousIndex();
      if (flash_index == kInvalidInt24) {
        DEBUG(
            "Last valid flash index reached while reverse iterating samples "
            "--> last ts",
            sample_ts);
        return count;
      }
      sample = src->GetSampleAtIndex(flash_index);
      timespan = last_sample_ts - sample.GetTimestamp();
      sample_ts = sample.GetTimestamp();
      buffer_index -= timespan / period_;
      // PRINT("Reverse iterating: new sample_ts=");
      // PRINT(sample_ts);
      // PRINT(" / timespan=");
      // PRINT(timespan);
      // PRINT(" / new buffer_index=");
      // PRINTLN(buffer_index);
    }
  }

  return count;

  // WORK NEED TO RESUME HERE
  // 1) probably remove the end filling (since buffer already initialized)
  // 2) need to define how we get out of this loop
  // 3) need to set properly:
  //   a) first_ (may have to expand all the return count;)
  //   b) last_
  //   c) last_ts

  /*
    uint32_t increment = timespan / period_;
    // fill buffer with 0 in case there was missing samples in flash
    // (case where timespan between two flash samples is larger than period)

    for (uint16_t j = 0; j < increment; j++) {
      buffer_index--;
      if (buffer_index >= 0) {
        buffer_[buffer_index] = 0;
        if (j > 0) length_++;
      } else {
        filled = true;
      }
    }
  }
  index_ = 0;
  return length_;
  */
}

void DisplaySamples::Print() {
  if (Serial) {
    Serial.print("DisplaySamples: size=");
    Serial.print(size_);
    Serial.print(", first=");
    Serial.print(first_);
    Serial.print(", last=");
    Serial.print(last_);
    Serial.print(", min=");
    Serial.print(min_);
    Serial.print(", max=");
    Serial.println(max_);
    for (size_t i = 0; i < size_; i++) {
      Serial.print("  ");
      Serial.print(i);
      Serial.print(" | ");
      Serial.println(buffer_[i]);
    }
  }
}

/*
uint32_t DisplaySamples::AppendData(int16_t data) {
  uint32_t new_index = index_ + length_;
  // PRINT("BEFORE index_="); PRINT(index_); PRINT(" / length_=");
  // PRINT(length_);
  // PRINT(" --> new temp index="); PRINT(new_index);
  if (new_index >= kGraphPxLength) new_index -= kGraphPxLength;
  buffer_[new_index] = data;
  if (data > max_) max_ = data;
  if (data < min_) min_ = data;
  index_++;
  if (index_ >= kGraphPxLength) index_ -= kGraphPxLength;
  // PRINT(" / new index="); PRINT(new_index);
  // PRINT(" ==== AFTER index_="); PRINT(index_);
  // PRINT(" / last_index="); PRINTLN(GetLastIndex());
  return new_index;
}
*/