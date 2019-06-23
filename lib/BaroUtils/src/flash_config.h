#ifndef DIGIBARO_FLASH_CONFIG_H
#define DIGIBARO_FLASH_CONFIG_H

#include <stdint.h>

class SPIFlash;

// Flash memory types size 
// TODO : should be merged with BaroSample, etc.
constexpr uint32_t kRobustIndexByteLength = 4;
constexpr uint32_t kRingSampleByteLength = 16;
constexpr uint32_t kPermanentSampleBytesLength = 8;

// Memory Map
// Start of robust indexes
// (Leave 4 sectors for settings)
constexpr uint32_t kRobustIndexesSectorStart = 4;

// Robust indexes are stored on 6 sectors:
// Redundancy = 2 / Need to erase one full sector when rotating
// 6/2 * 4096 / 4 = 3072 total indices
// --> 6/2 - 1 = 2 usable sectors --> (2*4096/4)=2048 usable indices
constexpr uint32_t kRobustIndexesSectorLength = 6;

// RotatingSamples computes correctly number of sector required!
// This value is hard coded here only for convinience purpose, and will 
// only be valid if the RotatingSamples is constructed without optional
// arguments.
constexpr uint32_t kPermanentSamplesSectorStart =
    kRobustIndexesSectorStart + (1+2) * kRobustIndexesSectorLength;

// 25.5 years of hourly samples
// constexpr uint32_t kPermanentSamplesSectorLength = 436;

// For debug purpose: 86 * 4096 / 8 = 44032 --> 5 years
constexpr uint32_t kPermanentSamplesSectorLength = 86;

constexpr uint32_t kDebugSectorStart =
    kPermanentSamplesSectorStart + kPermanentSamplesSectorLength;

constexpr uint32_t KDebugSectorLength = 512 - kDebugSectorStart;

constexpr uint8_t kMiniUltraProOnBoardChipSelectPin = 4;

#endif
