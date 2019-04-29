#ifndef DIGIBARO_ROTATING_SAMPLES_H
#define DIGIBARO_ROTATING_SAMPLES_H

#include "SPIMemory.h"
#include "baro_sample.h"
#include "flash_config.h"
#include "robust_flash_indexes.h"


const uint32_t kRotatingSamplesAddrStart = kRingSamplesSectorStart * KB(4);

class RotatingSamples {
 public:
  RotatingSamples(SPIFlash &flash);
  uint32_t begin();
  uint32_t GetTotalNumberOfSamples();
  uint32_t GetUsableNumberOfSamples();
  uint32_t GetLastSampleIndex();
  uint32_t GetIndexIterator(uint32_t length);
  uint32_t GetNextIndex();
  uint32_t GetReverseIndexIterator();
  uint32_t GetPreviousIndex();
  uint32_t AddSample(BaroSample &sample);
  uint32_t GetIndexesCounter();
  BaroSample GetSampleAtIndex(uint32_t index);

 private:
  SPIFlash &flash_;
  RobustFlashIndexes indexes_;
  uint32_t max_samples_;
  uint32_t usable_samples_;
  uint32_t last_index_iterator_;
  uint32_t current_index_iterator_;
  uint32_t rev_index_iterator_;
  bool iterator_end_;
};

#endif
