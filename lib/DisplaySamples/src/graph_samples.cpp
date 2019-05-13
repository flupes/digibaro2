#include "graph_samples.h"

#include "Adafruit_GFX.h"

#define COLORED 0
#define UNCOLORED 1

bool GraphSamples::Draw(GFXcanvas1 &canvas)
{
  canvas.fillScreen(UNCOLORED);
  int16_t y1 = 0;
  for (size_t i = 0; i < size_; i++) {
    int16_t data = Data(i);
    int16_t y2 = 300 - 300 * (data - min_) / (max_ - min_);
    if (y1 != 0 && data != INT16_MIN) {
      if (y1 < y2) {
        canvas.drawFastVLine(i + 10, y1 - 1, 2 + (y2 - y1), COLORED);
      } else {
        canvas.drawFastVLine(i + 10, y2 - 1, 2 + (y1 - y2), COLORED);
      }
    }
    if (data == INT16_MIN) {
      y1 = 0;
    } else {
      y1 = y2;
    }
  }
}
