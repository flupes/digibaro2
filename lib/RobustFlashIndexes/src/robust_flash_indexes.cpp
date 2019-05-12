#include "robust_flash_indexes.h"

#include <SPIMemory.h>

#ifdef RBI_DEBUG
#include "print_utils.h"
#else
#define PRINT(x)
#define PRINTLN(x)
#endif

FastCRC8 Int24Crc8::crc8_;

RobustFlashIndexes::RobustFlashIndexes(uint32_t sector_start,
                                       uint32_t total_sectors)
    : sector_start_(sector_start), empty_(false) {
  if (total_sectors % 2 != 0) {
    total_sectors--;
  }
  nb_sectors_ = total_sectors / 2;
  nb_indexes_ = nb_sectors_ * kSectorLength / kRobustIndexSize;
  indexes_start_[0] = sector_start * kSectorLength;
  indexes_start_[1] = indexes_start_[0] + nb_sectors_ * kSectorLength;
}

uint32_t RobustFlashIndexes::begin(SPIFlash *flash) {
  flash_ = flash;
  PRINTLN("RobustFlashIndexes::begin()");
  PRINT("nb_sector:          ");
  PRINTLN(nb_sectors_);
  PRINT("nb_indexes:         ");
  PRINTLN(nb_indexes_);
  PRINT("indexes_start[0]:   ");
  PRINTLN(indexes_start_[0]);
  PRINT("indexes_start[1]:   ");
  PRINTLN(indexes_start_[1]);

  current_index_ = RetrieveLastIndex();
  current_counter_ = GetCurrentCounter();

  PRINT("current index = ");
  PRINT(current_index_);
  PRINT(" / current_counter = ");
  PRINTLN(current_counter_);
  return current_index_;
}

uint32_t RobustFlashIndexes::GetCounterAt(uint32_t index) {
  // Serial.print("RobustFlashIndexes::GetCounterAt(");
  // Serial.print(index);
  // Serial.println(")"); 
  if (empty_) return kInvalidInt24;
  uint32_t offset = index * kRobustIndexSize;
  uint32_t counter =
      ReadCheckInt24(indexes_start_[0] + offset, indexes_start_[1] + offset);
  // Serial.print("--> [offset=");
  // Serial.print(offset);
  // Serial.print("] counter=");
  // Serial.println(counter);
  return counter;
}

uint32_t RobustFlashIndexes::RetrieveLastIndex() {
  uint32_t first[2] = {0, 0};
  PRINTLN("RobustFlashIndexes::RetrieveLastIndex()");
  for (int i = 0; i < 2; i++) {
    first[i] = flash_->readLong(indexes_start_[i]);
  }
  // Check if memory was correctly initialized
  if (first[0] == 0xFFFFFFFF || first[1] == 0xFFFFFFFF) {
    if (first[0] == first[1]) {
      PRINTLN("Indexes not initialized.");
    } else {
      PRINTLN("Indexes not correctly initialized!");
    }
    if (!InitializeMemory()) {
      PRINTLN("Error Initializing Memory!");
    }
    return 0;
  }

  PRINTLN("start searching for last index...");
  // Search for the last index
  uint32_t addr[2] = {indexes_start_[0], indexes_start_[1]};
  uint32_t last_counter = ReadCheckInt24(addr[0], addr[1]);
  if (last_counter == kInvalidInt24) {
    PRINTLN("Error reading first index!");
    PRINTLN("All things should collapse from here...");
    PRINTLN("Try to re-initialize the memory and start from fresh.");
    if (!InitializeMemory()) {
      PRINTLN("Error Initializing Memory!");
    }
    last_counter = 0;
  }
  for (uint32_t i = 0; i < nb_indexes_ - 1; i++) {
    addr[0] += kRobustIndexSize;
    addr[1] += kRobustIndexSize;
    // PRINT("checking index ");
    // PRINT(i, DEC);
    // PRINT(" at addr1=");
    // PRINT(addr[0]);
    // PRINT(" / addr2=");
    // PRINTLN(addr[1]);
    uint32_t first = flash_->readLong(addr[0]);
    uint32_t second = flash_->readLong(addr[1]);
    // Check for initialized memory
    if (first == 0xFFFFFFFF || second == 0xFFFFFFFF) {
      empty_ = false;
      PRINT("encountered unwritten memory for index ");
      PRINT(i + 1);
      PRINT(" at memory ");
      PRINT(addr[0]);
      PRINT(" / ");
      PRINTLN(addr[1]);
      if (first != second) {
        PRINTLN("Probable memory corruption: index termination do not match");
      }
      return i;
    }
    uint32_t counter = ReadCheckInt24(addr[0], addr[1]);
    // Check for previous counter: an increment different
    // from 1 or the wraparound indicates we reach the last index
    if ((counter - last_counter) != 1) {
      if ((last_counter - counter + 1) != kInvalidInt24) {
        PRINT("Older counter encountered for index = ");
        PRINT(i + 1);
        PRINT(" at memory ");
        PRINT(addr[0]);
        PRINT(" / ");
        PRINTLN(addr[1]);
        return i;
      }
    }
    last_counter = counter;
  }
  if (last_counter == kInvalidInt24) {
    PRINTLN("Last Index Not Found!");
    return kInvalidInt24;
  } else {
    PRINTLN("Return last index of the buffer");
    return nb_indexes_ - 1;
  }
}

uint32_t RobustFlashIndexes::Increment() {
  //   PRINT("current counter = ");
  //   PRINTLN(current_counter_);
  current_counter_++;
  if (current_counter_ == kInvalidInt24) {
    PRINTLN("Counter wrap around!");
    PRINTLN("Did you really wrote 16M indexes without wearing out the flash?");
    current_counter_ = 0;
    // At this point, we entered unchartered territory because
    // this condition was not fully tested.
  }

  if (!empty_) {
    current_index_++;
    PRINT("Increment : new index = ");
  } else {
    empty_ = false;
    PRINT("Increment : indexes not initialized yet, starting at index = ");
  }
  PRINTLN(current_index_);
  if (current_index_ >= nb_indexes_) {
    current_index_ = 0;
  }
  uint32_t offset = current_index_ * kRobustIndexSize;
  uint32_t addr[2] = {indexes_start_[0] + offset, indexes_start_[1] + offset};

  if (current_index_ % (kSectorLength / kRobustIndexSize) == 0) {
    // entering a new sector!
    PRINT("RobustFlashIndexes entering new sector for index = ");
    PRINTLN(current_index_);
    uint32_t code[2];
    code[0] = flash_->readLong(addr[0]);
    code[1] = flash_->readLong(addr[1]);
    if (code[0] != 0xFFFFFFFF || code[1] != 0xFFFFFFFF) {
      if (code[0] != code[1]) {
        PRINTLN("WARNING: sectors not in sync!");
      }
      PRINT("RobustFlashIndexes erasing sectors starting at: ");
      PRINT(addr[0]);
      PRINT(" / ");
      PRINTLN(addr[1]);
      flash_->eraseSector(addr[0]);
      flash_->eraseSector(addr[1]);
    }
  }

  DoubleWriteInt24(current_counter_, addr[0], addr[1]);
  return current_counter_;
}

uint32_t RobustFlashIndexes::GetCurrentCounter() {
  if (empty_) return 0;
  uint32_t offset = current_index_ * kRobustIndexSize;
  return ReadCheckInt24(indexes_start_[0] + offset, indexes_start_[1] + offset);
}

bool RobustFlashIndexes::InitializeMemory() {
  PRINTLN("RobustFlashIndexes::InitializeMemory()");
  uint32_t addr[2] = {indexes_start_[0], indexes_start_[1]};
  PRINT("Starting to erase sectors... ");
  uint8_t errors_count = 0;
  for (uint32_t i = 0; i < nb_sectors_; i++) {
    if (!flash_->eraseSector(addr[0])) {
      errors_count++;
      PRINTLN();
      PRINT("Error erasing sector starting at ");
      PRINTLN(addr[0]);
    }
    if (!flash_->eraseSector(addr[1])) {
      errors_count++;
      PRINTLN();
      PRINT("Error erasing sector starting at ");
      PRINTLN(addr[1]);
    }
    addr[0] += kSectorLength;
    addr[1] += kSectorLength;
  }
  PRINTLN("Done.");
  empty_ = true;
  current_index_ = 0;
  current_counter_ = 0;
  return (errors_count == 0);
}

uint32_t RobustFlashIndexes::ReadCheckInt24(uint32_t addr1, uint32_t addr2) {
  uint32_t first = flash_->readLong(addr1, false);
  uint32_t second = flash_->readLong(addr2, false);
  if ( first == 0xFFFFFFFF && second == 0xFFFFFFFF) {
    // Flash is uninitialized at this index
    return kInvalidInt24;
  }
  bool first_crc = Int24Crc8::Check(first);
  bool second_crc = Int24Crc8::Check(second);
  if (!first_crc && !second_crc) {
    PRINTLN("Both indexes CRC are wrong (cannot thrust either value)!");
    Serial.println("both crc wrong!");
    return kInvalidInt24;
  }
  if (!first_crc) {
    PRINTLN("CRC for first index is wrong!");
    return Int24Crc8::Data(second);
  }
  if (!second_crc) {
    PRINTLN("CRC for second index is wrong!");
    return Int24Crc8::Data(first);
  }
  if (first != second) {
    PRINTLN("Double double fault (CRC are correct but number do not match)!");
    return kInvalidInt24;
  }
  return Int24Crc8::Data(first);
}

bool RobustFlashIndexes::DoubleWriteInt24(uint32_t value, uint32_t addr1,
                                          uint32_t addr2) {
  uint32_t code = Int24Crc8::Create(value);
  PRINT("Double write(");
  PRINT(value);
  PRINT(") : addr1=");
  PRINT(addr1);
  PRINT(" / addr2=");
  PRINTLN(addr2);
  if (flash_->writeLong(addr1, code, true)) {
    if (flash_->writeLong(addr2, code, true)) {
      return true;
    }
  }
  return false;
}
