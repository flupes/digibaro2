#ifndef DIGI_DISPLAY_SAMPLES_H
#define DIGI_DISPLAY_SAMPLES_H

#include <stdint.h>

const int16_t kGraphPxLength = 320;

class RotatingSamples;
class BaroSample;

enum SampleDataType { UNDEF, PRESSURE, TEMPERATURE, HUMIDITY };

class DisplaySamples {
 public:
  /**
   * Creates a ring buffer to store one data value in RAM.
   */
  DisplaySamples(uint32_t period_in_seconds, uint32_t length = kGraphPxLength);

  /**
   * Fill the buffer from rotating samples on flash.
   * Pushes the samples to the left if now is higher than the last sample.
   * Add unvalid data for samples that are skipped on flash.
   */
  uint32_t Fill(RotatingSamples *src, uint32_t now,
                SampleDataType data_type = PRESSURE);

  /*
    uint32_t IndexOffset(int32_t reference, int32_t offset) {
      // reference can be signed (we do not need the last bit for indexes)
      // We want to minimize checks: the caller should not provide and offset
      // larger than the buffer!
      // TODO does not handle non-full buffers !!!
      int32_t index = reference + offset;
      if (index >= (int32_t)size_) index -= size_;
      if (index < 0) index += size_;
      return index;
    }

    uint32_t IndexAfter(uint32_t current) {
      if (current >= size_) return 0x00FFFFFE;
      if (current == size_) return 0;
      return current++;
    }

    int16_t DataAtIndex(uint32_t index) {
      if (index < size_) {
        return buffer_[index];
      }
      return 0xFFFF;
    }
  */
  void AppendData(BaroSample &sample);

  int16_t SerieMin() { return min_; }

  int16_t SerieMax() { return max_; }

  uint32_t size() { return size_; }

  int16_t Data(int16_t index);

  void Print();

 protected:
  void AppendData(int16_t data, uint32_t ts);

  uint32_t period_;  // in second
  uint32_t size_;
  uint32_t last_ts_;
  SampleDataType type_;
  int16_t buffer_[kGraphPxLength];
  int16_t min_;
  int16_t max_;
  int16_t first_;
};

#endif
