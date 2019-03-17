#include <FastCRC.h>

const uint32_t kInvalidInt24 = 0x00FFFFFE;
const uint32_t kSectorLength = 4096;

class SPIFlash;

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

class RobustFlashIndexes {
 public:
  /**
*sector_start : first sector for the indexes, counted in sector (not bytes)
*total_sectors: number of sectors to reserve for the indexes
*Since two copies of the indexes are stored, only half of the total_sector
*is
*actually useful information. An even number is required (otherwise the even
*nubmer below the one given is used)
*/
  RobustFlashIndexes(uint32_t sector_start, uint32_t total_sectors);

  void begin(SPIFlash *flash);

  uint32_t Increment();

  uint32_t GetCurrentIndex() { return current_index_; }

  uint32_t GetCurrentCounter();

 private:
  bool InitializeMemory();

  uint32_t RetrieveLastIndex();

  uint32_t ReadCheckInt24(uint32_t addr1, uint32_t addr2);

  bool DoubleWriteInt24(uint32_t value, uint32_t addr1, uint32_t addr2);

  uint32_t sector_start_;
  uint32_t nb_sectors_;
  uint32_t nb_indexes_;
  uint32_t current_index_;
  uint32_t current_counter_;
  uint32_t indexes_start_[2];
  SPIFlash *flash_;
};
