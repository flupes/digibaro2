#ifndef DIGI_GRAPH_SAMPLES_H
#define DIGI_GRAPH_SAMPLES_H

#include "display_samples.h"

class GFXcanvas1;

constexpr int16_t screen_width = 400;
constexpr int16_t screen_height = 300;
constexpr int16_t graph_width = 320;
constexpr int16_t graph_height = 240;
constexpr int16_t graph_xstart = 80;
constexpr int16_t graph_ystart = 280;

class GraphSamples : public DisplaySamples {
 public:
  GraphSamples(uint32_t period_in_seconds, uint32_t length = kGraphPxLength)
      : DisplaySamples(period_in_seconds, length) {}

  bool Draw(GFXcanvas1 &canvas);
};

#endif
