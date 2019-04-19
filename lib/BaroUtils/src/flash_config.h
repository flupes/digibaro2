#ifndef DIGIBARO_FLASH_CONFIG_H
#define DIGIBARO_FLASH_CONFIG_H

#include <ctype.h>

// Flash memory types size (should be merged with BaroSample, etc.)
constexpr uint32_t kRobustIndexByteLength = 4;
constexpr uint32_t kRingSampleByteLength = 16;
constexpr uint32_t kPermanentSampleBytesLength = 8;

// Memory Map
constexpr uint32_t kRobustIndexesSectorStart = 4;
constexpr uint32_t kRobustIndexesSectorLength =
    6;  // 2*(2+1) sectors --> 3072 indices

constexpr uint32_t kRingSamplesSectorStart =
    kRobustIndexesSectorStart + kRobustIndexesSectorLength;
constexpr uint32_t kRingSamplesSectorLength = 12;  // 3072*16[sample_size] / 4096

constexpr uint32_t kPermanentSamplesSectorStart =
    kRingSamplesSectorStart + kRingSamplesSectorLength;
// constexpr uint32_t kPermanentSamplesSectorLength = 490;
constexpr uint32_t kPermanentSamplesSectorLength = 16;

constexpr uint8_t kMiniUltraProOnBoardChipSelectPin = 4;

#endif
