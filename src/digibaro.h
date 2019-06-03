#ifndef DIGIBARO_DEFS_H
#define DIGIBARO_DEFS_H

#include <stdint.h>

// unused pins list
uint8_t unused_pins[] = {0,  1,  2,  3,  5,  7,  8,  9,  10, 11,
                         12, 14, 15, 16, 17, 18, 19, 22, 25, 26,
                         34, 35, 36, 37, 38, 39, 40, 41};

const uint8_t kRtcPowerPin = 6;

const uint8_t kSwitchesPin[2] = {2, 3};

#endif
