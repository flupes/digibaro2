#ifndef DIGIBARO_INT24_CRC8
#define DIGIBARO_INT24_CRC8

#include <FastCRC.h>

const uint32_t kInvalidInt24 = 0x00FFFFFE;

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

#endif
