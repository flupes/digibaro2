#include "robust_flash_indexes.h"

#include <SPIMemory.h>

FastCRC8 Int24Crc8::crc8_;

#define Serial SERIAL_PORT_USBVIRTUAL

RobustFlashIndexes::RobustFlashIndexes(uint32_t sector_start,
                                       uint32_t total_sectors)
    : sector_start_(sector_start) {
  if (total_sectors % 2 != 0) {
    total_sectors--;
  }
  //   Serial.println("RobustFlashIndexes constructor");
  nb_sectors_ = total_sectors / 2;
  nb_indexes_ = nb_sectors_ * kSectorLength / 4;
  indexes_start_[0] = sector_start * kSectorLength;
  indexes_start_[1] = indexes_start_[0] + nb_sectors_ * kSectorLength;
}

void RobustFlashIndexes::begin(SPIFlash *flash) {
  flash_ = flash;
  Serial.println("---- BEGIN ---- RobustFlashIndexes::begin()");
  Serial.print("nb_sector:          ");
  Serial.println(nb_sectors_);
  Serial.print("nb_indexes:         ");
  Serial.println(nb_indexes_);
  Serial.print("indexes_start[0]:   ");
  Serial.println(indexes_start_[0]);
  Serial.print("indexes_start[1]:   ");
  Serial.println(indexes_start_[1]);
  current_index_ = RetrieveLastIndex();
  current_counter_ = GetCurrentCounter();
  Serial.print("current index = ");
  Serial.println(current_index_);
  Serial.println("---- END ---- RobustFlashIndexes::begin()");
}

uint32_t RobustFlashIndexes::GetCounter(uint32_t index) {
  uint32_t first =
      ReadCheckInt24(indexes_start_[0] + index, indexes_start_[1] + index);
  uint32_t second =
      ReadCheckInt24(indexes_start_[1] + index, indexes_start_[1] + index);

  if (first != second) {
    if (first != kInvalidInt24) {
      return first;
    }
    if ( second != kInvalidInt24) {
      return second;
    }
  }
  return first;
}

uint32_t RobustFlashIndexes::RetrieveLastIndex() {
  uint32_t first[2] = {0, 0};

  Serial.println("RobustFlashIndexes::RetrieveLastIndex()");
  for (int i = 0; i < 2; i++) {
    first[i] = flash_->readLong(indexes_start_[i]);
  }
//   Serial.print("first[0] = ");
//   Serial.println(first[0], HEX);
//   Serial.print("first[0] = ");
//   Serial.println(first[0], HEX);
  // Check if memory was correctly initialized
  if (first[0] == 0xFFFFFFFF || first[1] == 0xFFFFFFFF) {
    if (first[0] == first[1]) {
      Serial.println("Indexes not initialized.");
    } else {
      Serial.println("Indexes not correctly initialized!");
    }
    InitializeMemory();
    return 0;
  }

  uint32_t last_counter = GetCounter(0);

  Serial.println("start searching for last index...");
  // Search for the last index
  uint32_t addr[2] = {indexes_start_[0], indexes_start_[1]};
  for (uint32_t i = 0; i < nb_indexes_ - 1; i++) {
    addr[0] += 4;
    addr[1] += 4;
    // Serial.print("checking index ");
    // Serial.print(i, DEC);
    // Serial.print(" at addr1=");
    // Serial.print(addr[0]);
    // Serial.print(" / addr2=");
    // Serial.println(addr[1]);
    uint32_t first = flash_->readLong(addr[0]);
    uint32_t second = flash_->readLong(addr[1]);
    if (first == 0xFFFFFFFF || second == 0xFFFFFFFF) {
      Serial.print("encountered unwritten memory for index ");
      Serial.print(i + 1);
      Serial.print(" at memory ");
      Serial.print(addr[0]);
      Serial.print(" / ");
      Serial.println(addr[1]);
      if ( first != second ) {
          Serial.println("Probable memory corruption: index termination do not match");
      }
      return i;
    }
    uint32_t counter = GetCounter(i+1);
    if (counter > last_counter) {
      Serial.print("Older counter encountered for index = ");
      Serial.print(i+1);
      Serial.print(" at memory ");
      Serial.print(addr[0]);
      Serial.print(" / ");
      Serial.println(addr[1]);
      return i;
    }
  }
  Serial.println("Last Index Not Found!");
  return kInvalidInt24;
}

uint32_t RobustFlashIndexes::Increment() {
  //   Serial.print("current counter = ");
  //   Serial.println(current_counter_);
  current_counter_++;
  current_index_++;
  //   Serial.print("next index = ");
  //   Serial.println(current_index_);
  if (current_index_ >= nb_indexes_ ) {
    current_index_ = 0;
  }
  uint32_t offset = current_index_ * sizeof(uint32_t);
  uint32_t addr[2] = {indexes_start_[0] + offset, indexes_start_[1] + offset};

  if (current_index_ % (kSectorLength/sizeof(uint32_t)) == 0) {
    // entering a new sector!
    Serial.print("Entering new sector for index = ");
    Serial.println(current_index_);
    uint32_t code[2];
    code[0] = flash_->readLong(addr[0]);
    code[1] = flash_->readLong(addr[1]);
    if (code[0] != 0xFFFFFFFF || code[1] != 0xFFFFFFFF) {
      if (code[0] != code[1]) {
        Serial.println("WARNING: sectors not in sync!");
      }
      Serial.print("Erasing sectors starting at: ");
      Serial.print(addr[0]);
      Serial.print(" / ");
      Serial.println(addr[1]);
      flash_->eraseSector(addr[0]);
      flash_->eraseSector(addr[1]);
    }
  }

  DoubleWriteInt24(current_counter_, addr[0], addr[1]);
  return current_counter_;
}

uint32_t RobustFlashIndexes::GetCurrentCounter() {
  uint32_t offset = current_index_ * sizeof(uint32_t);
  return ReadCheckInt24(indexes_start_[0] + offset, indexes_start_[1] + offset);
}

bool RobustFlashIndexes::InitializeMemory() {
  Serial.println("RobustFlashIndexes::InitializeMemory()");
  uint32_t addr[2] = {indexes_start_[0], indexes_start_[1]};
  for (uint32_t i = 0; i < nb_sectors_; i++) {
    if (!flash_->eraseSector(addr[0])) {
      Serial.print("Error erasing sector starting at ");
      Serial.println(addr[0]);
    }
    if (!flash_->eraseSector(addr[1])) {
      Serial.print("Error erasing sector starting at ");
      Serial.println(addr[1]);
    }
    addr[0] += kSectorLength;
    addr[1] += kSectorLength;
  }
  current_index_ = 0;
  current_counter_ = 0;
  return DoubleWriteInt24(current_counter_, indexes_start_[0], indexes_start_[1]);
}

uint32_t RobustFlashIndexes::ReadCheckInt24(uint32_t addr1, uint32_t addr2) {
  uint32_t first = flash_->readLong(addr1, false);
  uint32_t second = flash_->readLong(addr2, false);
  bool first_crc = Int24Crc8::Check(first);
  bool second_crc = Int24Crc8::Check(second);
  if (!first_crc && !second_crc) {
    Serial.println("Both indexes CRC are wrong (cannot thrust either value)!");
    return kInvalidInt24;
  }
  if (!first_crc) {
    Serial.println("CRC for first index is wrong!");
    return Int24Crc8::Data(second);
  }
  if (!second_crc) {
    Serial.println("CRC for second index is wrong!");
    return Int24Crc8::Data(first);
  }
  if (first != second) {
    Serial.println(
        "Double double fault (CRC are correct but number do not match)!");
    return kInvalidInt24;
  }
  return Int24Crc8::Data(first);
}

bool RobustFlashIndexes::DoubleWriteInt24(uint32_t value, uint32_t addr1,
                                          uint32_t addr2) {
  uint32_t code = Int24Crc8::Create(value);
  //   Serial.print("Double write(");
  //   Serial.print(value);
  //   Serial.print(") : addr1=");
  //   Serial.print(addr1);
  //   Serial.print(" / addr2=");
  //   Serial.println(addr2);
  if (flash_->writeLong(addr1, code, true)) {
    if (flash_->writeLong(addr2, code, true)) {
      return true;
    }
  }
  return false;
}
