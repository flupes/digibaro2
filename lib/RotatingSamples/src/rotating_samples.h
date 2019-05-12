#ifndef DIGIBARO_ROTATING_SAMPLES_H
#define DIGIBARO_ROTATING_SAMPLES_H

#include "SPIMemory.h"
#include "flash_config.h"
#include "robust_flash_indexes.h"

class BaroSample;

class RotatingSamples {
 public:
  /**
   * Declare the rotating samples.
   * Rotating samples relies on Ro  bustFlashIndexes to maintain the indexes.
   * Warning: the optional argument should be used for debug purpose only. It
   * will break the memory map layout in flash_config.h (more speficically
   * the constant kPermanentSamplesSectorStart)!
   */
  RotatingSamples(
      SPIFlash &flash, uint32_t indexes_start = kRobustIndexesSectorStart,
      uint32_t robust_indices_sector_length = kRobustIndexesSectorLength);
  
  uint32_t begin();

  /** Returns the start of the data structure (in sectors) : this is the
   * start of the RobustFlashIndexes. */
  uint32_t GetSectorStart();

  /** Return the total number of sectors used by the data structure : this is
   * the size of the underlying RobustFlashIndexes + the size of the of the
   * RotatingSamples.
  */
  uint32_t GetSectorsLength();

  /** 
   * The total number of samples is defined by the the number of sectors
   * allocated divided by the data size of a sample. The number of sectors
   * allocated for this structure is a function of the number of sectors
   * that have been specified in the constructor for the RobustIndexes.
   */
  uint32_t GetTotalNumberOfSamples();
 
  /**
   * The usable number of samples is the total number of samples minus how 
   * samples are contained in one sector. This is due to the fact that when
   * some flash memory with data already written to it is encountered, it 
   * is necessary to re-initialize the whole sector.
   */
  uint32_t GetUsableNumberOfSamples();

  /**
   * Get the index of the last sample in the ring buffer.
   */
  uint32_t GetLastSampleIndex();
 
  /**
   * Initialize a forward iterator on a Serie of samples.
   * If no argument is provided, then the Serie is considered of 
   * length GetUsableNumberOfSamples.
   * If an argument is provided, it is the length of the desired
   * Serie to iterate over. The first element of the Serie will be
   * the last element in the ring buffer offseted by length-1.
   * Returns the index of the first element of the Serie.
   */
  uint32_t GetIndexIterator(uint32_t length = 0);

  /**
   * Returns the index of the next element in the forward Serie.
   * Once the last element of the buffer is reached, the methods returns 
   * kInvalidInt24.
   */
  uint32_t GetNextIndex();
 
  uint32_t GetReverseIndexIterator();
  uint32_t GetPreviousIndex();
 
  /** Add a sample to the ring buffer.
   * This operation both add the sample to the ring buffer, and 
   * increment accordingly the RobustFlashIndexes in order to maintain 
   * the location of the last sample after a power loss.
   */
  uint32_t AddSample(BaroSample &sample);
  
  /**
   * Query the underlying RobustFlashIndexes to return the last counter 
   * stored on flash.
   */
  uint32_t GetIndexesCounter();
  
  /** 
   * Returns the sample stored at the specified address.
   */
  BaroSample GetSampleAtIndex(uint32_t index);

 private:
  SPIFlash &flash_;
  RobustFlashIndexes indexes_;
  uint32_t ring_sectors_length_;
  uint32_t samples_addr_start_;
  uint32_t max_samples_;
  uint32_t usable_samples_;
  uint32_t last_index_iterator_;
  uint32_t current_index_iterator_;
  uint32_t rev_index_iterator_;
  bool iterator_end_;
};

#endif
