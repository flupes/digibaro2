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
  BaroSample GetSampleWithIndex(uint32_t index);
  uint32_t GetNumberOfSamples();
  BaroSample GetSampleAtAddr(uint32_t addr);
  uint32_t GetLastSampleAddr();

 private:
  uint32_t LocateLastSample();
  uint32_t current_sample_addr_;
  uint32_t number_of_samples_;
  SPIFlash &flash_;
  bool empty_;
  bool full_;
};

#endif
