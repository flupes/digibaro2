#ifndef DIGIBARO_PERMANENT_SAMPLES_H
#define DIGIBARO_PERMANENT_SAMPLES_H

#include "SPIMemory.h"
#include "baro_sample.h"
#include "flash_config.h"

const uint32_t kPermanentSamplesAddrStart =
    kPermanentSamplesSectorStart * KB(4);

class PermanentSamples {
 public:
  PermanentSamples(SPIFlash &flash);
  uint32_t begin();
  uint32_t AddSample(BaroSample &sample);
  uint32_t GetLastIndex();
  BaroSample GetLastSample();
  BaroSample GetSampleWithIndex(uint32_t index);
  uint32_t GetCurrentNumberOfSamples();
  uint32_t GetMaxNumberOfSamples();
  uint32_t GetFirstSampleAddr();
  uint32_t GetLastSampleAddr();

 private:
  BaroSample GetSampleAtAddr(uint32_t addr);
  uint32_t current_sample_addr_;
  uint32_t number_of_samples_;
  uint32_t max_samples_;
  SPIFlash &flash_;
};

#endif
