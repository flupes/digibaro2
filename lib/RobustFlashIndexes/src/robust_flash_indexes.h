#ifndef DIGIBARO_ROBUST_FLASH_INDEXES_H
#define DIGIBARO_ROBUST_FLASH_INDEXES_H

#include <FastCRC.h>

const uint32_t kInvalidInt24 = 0x00FFFFFE;
const uint32_t kSectorLength = 4096;
const uint32_t kRobustIndexSize = sizeof(uint32_t);

class SPIFlash;

/**
 * Int24Crc8 code an integer on 24 bits with an extra 8 bits for CRC.
 *
 * Int24Crc8 is extensively used by a lot of DigiBaro2 classes: it allows
 * to read/write a 32 bits words to flash with safety.
 *
 * The constant kInvalidInt24 is also used to indentify corrupted integers
 * or invalid indexes. It is different from 0xFFFFFF since we need to
 * differentiate from some flash unitialized memory.
 */
class Int24Crc8 {
 public:
  static uint32_t Create(uint32_t data) {
    // zero MSB
    data &= 0x00FFFFFF;
    // compute crc
    uint8_t *addr = (uint8_t *)(&data);
    uint8_t crc_write = crc8_.maxim(addr, 3);
    // assemble code word
    return data | ((uint32_t)crc_write << 24);
  }

  static bool Check(uint32_t code) {
    uint8_t *addr = (uint8_t *)(&code);
    uint8_t crc_check = crc8_.maxim(addr, 3);
    if (crc_check != (code >> 24)) {
      return false;
    }
    return true;
  }

  static uint32_t Data(uint32_t code) { return code & 0x00FFFFFF; }

 private:
  static FastCRC8 crc8_;
};

/**
 * RobustFlashIndexes is a double ring buffer with the goal to obtain
 * a very safe system to maintain indexes on flash memory.
 *
 * The class constructor sets the start address of the indexes and its
 * total length.
 *
 * The begin() method link the class to the flash abstraction.
 *
 * Besides some information methods, RobustFlashIndexes main method is:
 *   - Increment : increment the counter and write the result in the next slot
 *
 * Under the hood, the class actually creates two redundant ring buffer similar
 * to: http://ww1.microchip.com/downloads/en/appnotes/doc2526.pdf
 *
 * Every counter is written twice to flash, and when retrieved, the two counters
 * are 1) checksumed and 2) compared to each other.
 */
class RobustFlashIndexes {
 public:
  /**
   * sector_start : first sector for the indexes, counted in sector (not bytes)
   * total_sectors: number of sectors to reserve for the indexes.
   *     Since two copies of the indexes are stored, only half of the
   *     total_sector is actually useful information. An even number is required
   *     (otherwise the evennumber below the one given is used)
  */
  RobustFlashIndexes(uint32_t sector_start, uint32_t total_sectors);

  /**
   * Really start the object: retrieve the last index on flash and initialize
   * memory if not ready yet.
   */
  uint32_t begin(SPIFlash *flash);

  /**
   * Increment the counter and write it twice in the next ring buffer slot.
   */
  uint32_t Increment();

  uint32_t GetCurrentIndex() { return current_index_; }

  uint32_t GetCurrentCounter();

  uint32_t GetCounterAt(uint32_t index);

  uint32_t TotalNumberOfIndexes() { return nb_indexes_; }

  uint32_t NumberOfUsableIndexes() {
    return nb_indexes_ - kSectorLength / kRobustIndexSize;
  }

 protected:
  uint32_t RetrieveLastIndex();

 private:
  bool InitializeMemory();

  uint32_t ReadCheckInt24(uint32_t addr1, uint32_t addr2);

  bool DoubleWriteInt24(uint32_t value, uint32_t addr1, uint32_t addr2);

  uint32_t sector_start_;
  uint32_t nb_sectors_;
  uint32_t nb_indexes_;
  uint32_t current_index_;
  uint32_t current_counter_;
  uint32_t indexes_start_[2];
  SPIFlash *flash_;
  bool empty_;
};

#endif
