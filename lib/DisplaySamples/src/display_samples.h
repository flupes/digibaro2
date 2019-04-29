#ifndef DIGI_DISPLAY_SAMPLES_H
#define DIGI_DISPLAY_SAMPLES_H

#include <stdint.h>

const int16_t kGraphPxLength = 320;

class RotatingSamples;

enum SampleDataType { UNDEF, PRESSURE, TEMPERATURE, HUMIDITY };

class DisplaySamples {
 public:
  DisplaySamples(uint32_t period_in_seconds);

  uint32_t Begin(RotatingSamples *src, SampleDataType data_type = PRESSURE);

  uint32_t BufferStartIndex() { return index_; }

  uint32_t GetLastIndex() {
    uint32_t last = index_ + kGraphPxLength - 1;
    if (last >= kGraphPxLength) last -= kGraphPxLength;
    return last;
  }

  uint32_t IndexOffset(int32_t reference, int32_t offset) {
    // reference can be signed: we will never have such huge indices or offset!
    // We want to minimize checks: the caller should not provide and offset
    // larger thant he buffer!
    int32_t index = reference + offset;
    if (index >= kGraphPxLength) index -= kGraphPxLength;
    if (index < 0) index += kGraphPxLength;
    return index;
  }

  uint32_t IndexAfter(uint32_t current) {
    if (current >= length_) return 0x00FFFFFE;
    current++;
    if (current >= kGraphPxLength) return 0;
    return current;
  }

  int16_t DataAtIndex(uint32_t index) {
    if (index < length_) {
      return buffer_[index];
    }
    return 0xFFFF;
  }
  uint32_t AppendData(int16_t data);

 private:
  uint32_t period_seconds_;
  uint32_t index_;
  uint32_t length_;
  int16_t buffer_[kGraphPxLength];
};

#endif
