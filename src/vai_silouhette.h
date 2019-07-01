#include <stdint.h>

typedef struct {
  const uint8_t *data;
  uint16_t width;
  uint16_t height;
  uint8_t dataSize;
} tImage;

extern const tImage vai_silouhette;
