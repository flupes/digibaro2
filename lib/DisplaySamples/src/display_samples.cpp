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
  first_ = 0;
}

int divRoundClosest(const int n, const int d) {
  return ((n < 0) ^ (d < 0)) ? ((n - d / 2) / d) : ((n + d / 2) / d);
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
  last_ts_ = now;
  first_ = 0;

  uint32_t flash_index = src->GetReverseIndexIterator();
  if (flash_index == kInvalidInt24) return count;

  int32_t accumulator = 0;
  size_t bucket_count = 0;
  size_t current_index = kInvalidInt24;
  size_t last_index = size_ - 1;
  uint32_t first_ts = last_ts_ - (size_ - 1) * period_;
  uint32_t sample_ts = 0;
  bool done = false;
  // Serial.print("Start filling: flash_index=");
  // Serial.print(flash_index);
  // Serial.print(" | first_ts=");
  // Serial.println(first_ts);

  while (!done) {
    BaroSample sample = src->GetSampleAtIndex(flash_index);
    sample_ts = sample.GetTimestamp();

    if (first_ts <= sample_ts && sample_ts <= last_ts_) {
      // Get data to insert in buffer
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
      if (data < min_) min_ = data;
      if (data > max_) max_ = data;

      // Bucket to place the data
      current_index = size_ - (last_ts_ - sample_ts) / period_ - 1;
      // Serial.print("flash_index=");
      // Serial.print(flash_index);
      // Serial.print(" : ts=");
      // Serial.print(sample_ts);
      // Serial.print(" -> data=");
      // Serial.print(data);
      // Serial.print(" | current_index=");
      // Serial.print(current_index);
      // Serial.print(" / last_index=");
      // Serial.print(last_index);

      if (current_index < last_index) {
        // add previous sample
        buffer_[last_index] = accumulator / bucket_count;
        count++;
        // Serial.print(" | added sample in ");
        // Serial.print(last_index);
        // Serial.print(" <- ");
        // Serial.println(buffer_[last_index]);
        // Start to accumulate again
        last_index = current_index;
        accumulator = data;
        bucket_count = 1;
      } else {
        // compute the mean
        accumulator += data;
        bucket_count++;
        // Serial.print(" | accumulate -> bucket_count=");
        // Serial.println(bucket_count);
      }
    }
    flash_index = src->GetPreviousIndex();
    if (flash_index == kInvalidInt24) done = true;

    if (sample_ts < first_ts) done = true;
  }
  if (current_index < size_) {
    // Need to take care of the last sample
    buffer_[current_index] = accumulator / bucket_count;
    count++;
    // Serial.print("last sample added in ");
    // Serial.print(current_index);
    // Serial.print(" <- ");
    // Serial.println(buffer_[current_index]);
  }

  return count;
}

void DisplaySamples::AppendData(BaroSample &sample) {
  int16_t data;
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
  AppendData(data, sample.GetTimestamp());
}

void DisplaySamples::AppendData(int16_t data, uint32_t ts) {
  size_t skip = (ts - last_ts_) / period_ - 1;
  if (skip > size_) skip = size_;
  size_t index = first_ + size_;
  for (size_t i = 0; i < skip; i++) {
    if (index >= size_) index -= size_;
    buffer_[index++] = INT16_MIN;
    first_++;
  }
  if (index >= size_) index -= size_;
  buffer_[index] = data;
  first_++;
  if ((uint16_t)first_ >= size_) first_ -= size_;
  last_ts_ = ts;

  // Search again for limits (since we discarded a bunch of values, we have
  // to parse the full buffer again).
  min_ = INT16_MAX - 1;
  max_ = INT16_MIN + 1;
  for (size_t i = 0; i < size_; i++) {
    data = buffer_[i];
    if (data != INT16_MIN) {
      if (data < min_) min_ = data;
      if (data > max_) max_ = data;
    }
  }
}

int16_t DisplaySamples::Data(int16_t index) {
  if (index < 0 || (uint16_t)index >= size_) return INT16_MIN;
  index += first_;
  if ((uint16_t)index >= size_) index -= size_;
  return buffer_[index];
}

void DisplaySamples::Print() {
  if (Serial) {
    Serial.print("DisplaySamples: size=");
    Serial.print(size_);
    Serial.print(", last_ts=");
    Serial.print(last_ts_);
    Serial.print(", first=");
    Serial.print(first_);
    Serial.print(", min=");
    Serial.print(min_);
    Serial.print(", max=");
    Serial.println(max_);
    for (size_t i = 0; i < size_; i++) {
      Serial.print("  ");
      Serial.print(i);
      // Serial.print(" | ");
      // Serial.print(buffer_[i]);
      Serial.print(" | ");
      Serial.println(Data(i));
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